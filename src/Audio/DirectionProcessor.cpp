#include "DirectionProcessor.h"
#include "../Common/Logger.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// 方向映射表：方向 -> (方位角, 仰角)
const std::array<std::pair<CardinalDirection, std::pair<float, float>>, 10> DirectionProcessor::s_cardinalDirections = {{
    {CardinalDirection::Front,      {0.0f,    0.0f}},
    {CardinalDirection::Back,       {180.0f,  0.0f}},
    {CardinalDirection::Left,       {-90.0f,  0.0f}},
    {CardinalDirection::Right,      {90.0f,   0.0f}},
    {CardinalDirection::Up,         {0.0f,    90.0f}},
    {CardinalDirection::Down,       {0.0f,   -90.0f}},
    {CardinalDirection::FrontLeft,  {-45.0f,  0.0f}},
    {CardinalDirection::FrontRight, {45.0f,   0.0f}},
    {CardinalDirection::BackLeft,   {-135.0f, 0.0f}},
    {CardinalDirection::BackRight,  {135.0f,  0.0f}}
}};

DirectionProcessor::DirectionProcessor()
    : m_hasLastValues(false)
    , m_averageIntensity(0.0f)
    , m_directionStability(0.0f)
    , m_processedFrames(0)
    , m_lastIntensity(0.0f)
{
    m_history = std::make_unique<DirectionHistory>(m_config.historyBufferSize);
    Logger::Info("DirectionProcessor created");
}

DirectionProcessor::~DirectionProcessor()
{
    Logger::Info("DirectionProcessor destroyed");
}

ProcessedDirection DirectionProcessor::ProcessAudioData(const SpatialAudioData& data)
{
    ProcessedDirection result;

    // 计算主要方向
    DirectionVector primaryDirection = CalculatePrimaryDirection(data);
    
    // 计算强度和置信度
    float intensity = CalculateIntensity(data);
    float confidence = CalculateConfidence(data);

    // 验证方向有效性
    if (!ValidateDirection(primaryDirection) || intensity < m_config.sensitivityThreshold)
    {
        result.primary = CardinalDirection::None;
        result.intensity = 0.0f;
        return result;
    }

    // 应用噪声过滤
    primaryDirection = FilterNoise(primaryDirection, intensity);

    // 应用平滑滤波
    if (m_config.enableDirectionFiltering)
    {
        primaryDirection = ApplyDirectionSmoothing(primaryDirection);
        intensity = ApplyIntensitySmoothing(intensity);
    }

    // 更新历史记录
    UpdateHistory(primaryDirection, intensity, confidence);

    // 转换为基本方向
    result.primary = GetPrimaryCardinalDirection(primaryDirection);
    result.intensity = intensity;
    result.secondary = GetSecondaryDirections(primaryDirection);

    // 更新统计信息
    m_processedFrames++;
    m_averageIntensity = (m_averageIntensity * (m_processedFrames - 1) + intensity) / m_processedFrames;

    return result;
}

void DirectionProcessor::SetProcessingParameters(const ProcessingConfig& config)
{
    m_config = config;
    
    // 重新调整历史缓冲区大小
    if (m_history->maxSize != config.historyBufferSize)
    {
        m_history = std::make_unique<DirectionHistory>(config.historyBufferSize);
    }
    
    Logger::Debug("Processing parameters updated");
}

void DirectionProcessor::UpdateConfig(const AudioConfig& audioConfig)
{
    m_audioConfig = audioConfig;
    
    // 更新处理配置
    m_config.sensitivityThreshold = audioConfig.noiseThreshold;
    
    Logger::Debug("Audio configuration updated in DirectionProcessor");
}

CardinalDirection DirectionProcessor::GetPrimaryCardinalDirection(const DirectionVector& direction)
{
    if (VectorMagnitude(direction.x, direction.y, direction.z) < 0.1f)
        return CardinalDirection::None;

    float bestScore = -1.0f;
    CardinalDirection bestDirection = CardinalDirection::None;

    for (const auto& mapping : s_cardinalDirections)
    {
        float score = CalculateDirectionWeight(direction, mapping.first);
        if (score > bestScore)
        {
            bestScore = score;
            bestDirection = mapping.first;
        }
    }

    return bestDirection;
}

std::vector<CardinalDirection> DirectionProcessor::GetSecondaryDirections(const DirectionVector& direction, float threshold)
{
    std::vector<CardinalDirection> secondaryDirections;

    for (const auto& mapping : s_cardinalDirections)
    {
        float weight = CalculateDirectionWeight(direction, mapping.first);
        if (weight > threshold && weight < 0.8f) // 不包括主方向
        {
            secondaryDirections.push_back(mapping.first);
        }
    }

    // 按权重排序
    std::sort(secondaryDirections.begin(), secondaryDirections.end(),
        [this, &direction](CardinalDirection a, CardinalDirection b) {
            return CalculateDirectionWeight(direction, a) > CalculateDirectionWeight(direction, b);
        });

    return secondaryDirections;
}

float DirectionProcessor::GetAverageIntensity() const
{
    return m_averageIntensity;
}

float DirectionProcessor::GetDirectionStability() const
{
    return m_directionStability;
}

void DirectionProcessor::ResetHistory()
{
    m_history = std::make_unique<DirectionHistory>(m_config.historyBufferSize);
    m_hasLastValues = false;
    m_averageIntensity = 0.0f;
    m_directionStability = 0.0f;
    m_processedFrames = 0;
    
    Logger::Debug("Direction history reset");
}

DirectionVector DirectionProcessor::CalculatePrimaryDirection(const SpatialAudioData& data)
{
    DirectionVector result = data.primaryDirection;

    // 如果有多个方向，找到最强的
    if (!data.secondaryDirections.empty())
    {
        std::vector<DirectionVector> allDirections = data.secondaryDirections;
        allDirections.push_back(data.primaryDirection);
        
        result = FindDominantDirection(allDirections);
    }

    // 转换为球坐标
    if (result.azimuth == 0.0f && result.elevation == 0.0f)
    {
        result = CartesianToSpherical(result.x, result.y, result.z);
    }

    return result;
}

float DirectionProcessor::CalculateIntensity(const SpatialAudioData& data)
{
    float intensity = data.intensity;

    // 应用敏感度调整
    intensity *= m_audioConfig.sensitivity;

    // 强度归一化
    if (m_config.enableIntensityNormalization)
    {
        intensity = std::clamp(intensity, 0.0f, 1.0f);
    }

    return intensity;
}

float DirectionProcessor::CalculateConfidence(const SpatialAudioData& data)
{
    float confidence = data.confidence;

    // 基于信号强度调整置信度
    if (data.intensity < m_config.noiseFloor)
    {
        confidence *= 0.5f;
    }

    // 基于方向一致性调整置信度
    if (m_hasLastValues)
    {
        float directionDiff = AngleDifference(data.primaryDirection.azimuth, m_lastDirection.azimuth);
        if (directionDiff > 45.0f)
        {
            confidence *= 0.7f;
        }
    }

    return std::clamp(confidence, 0.0f, 1.0f);
}

bool DirectionProcessor::ValidateDirection(const DirectionVector& direction)
{
    // 检查方向向量是否有效
    if (std::isnan(direction.x) || std::isnan(direction.y) || std::isnan(direction.z))
        return false;

    if (std::isnan(direction.azimuth) || std::isnan(direction.elevation))
        return false;

    // 检查角度范围
    if (direction.azimuth < -180.0f || direction.azimuth > 180.0f)
        return false;

    if (direction.elevation < -90.0f || direction.elevation > 90.0f)
        return false;

    // 检查向量长度
    float magnitude = VectorMagnitude(direction.x, direction.y, direction.z);
    if (magnitude < 0.001f || magnitude > 10.0f)
        return false;

    return true;
}

DirectionVector DirectionProcessor::CartesianToSpherical(float x, float y, float z)
{
    DirectionVector result;
    result.x = x;
    result.y = y;
    result.z = z;

    float magnitude = VectorMagnitude(x, y, z);
    result.distance = magnitude;

    if (magnitude > 0.001f)
    {
        // 计算方位角 (azimuth)
        result.azimuth = std::atan2(x, z) * 180.0f / M_PI;
        
        // 计算仰角 (elevation)
        result.elevation = std::asin(y / magnitude) * 180.0f / M_PI;
    }
    else
    {
        result.azimuth = 0.0f;
        result.elevation = 0.0f;
    }

    return result;
}

DirectionVector DirectionProcessor::SphericalToCartesian(float azimuth, float elevation, float distance)
{
    DirectionVector result;
    
    float azimuthRad = azimuth * M_PI / 180.0f;
    float elevationRad = elevation * M_PI / 180.0f;
    
    result.x = distance * std::sin(azimuthRad) * std::cos(elevationRad);
    result.y = distance * std::sin(elevationRad);
    result.z = distance * std::cos(azimuthRad) * std::cos(elevationRad);
    
    result.azimuth = azimuth;
    result.elevation = elevation;
    result.distance = distance;

    return result;
}

CardinalDirection DirectionProcessor::SphericalToCardinal(float azimuth, float elevation)
{
    // 首先检查垂直方向
    if (std::abs(elevation) > 60.0f)
    {
        return (elevation > 0) ? CardinalDirection::Up : CardinalDirection::Down;
    }

    // 然后检查水平方向
    float normalizedAzimuth = NormalizeAngle(azimuth);
    
    if (normalizedAzimuth >= -22.5f && normalizedAzimuth < 22.5f)
        return CardinalDirection::Front;
    else if (normalizedAzimuth >= 22.5f && normalizedAzimuth < 67.5f)
        return CardinalDirection::FrontRight;
    else if (normalizedAzimuth >= 67.5f && normalizedAzimuth < 112.5f)
        return CardinalDirection::Right;
    else if (normalizedAzimuth >= 112.5f && normalizedAzimuth < 157.5f)
        return CardinalDirection::BackRight;
    else if (normalizedAzimuth >= 157.5f || normalizedAzimuth < -157.5f)
        return CardinalDirection::Back;
    else if (normalizedAzimuth >= -157.5f && normalizedAzimuth < -112.5f)
        return CardinalDirection::BackLeft;
    else if (normalizedAzimuth >= -112.5f && normalizedAzimuth < -67.5f)
        return CardinalDirection::Left;
    else if (normalizedAzimuth >= -67.5f && normalizedAzimuth < -22.5f)
        return CardinalDirection::FrontLeft;

    return CardinalDirection::Front;
}

DirectionVector DirectionProcessor::ApplyDirectionSmoothing(const DirectionVector& newDirection)
{
    if (!m_hasLastValues)
    {
        m_lastDirection = newDirection;
        m_hasLastValues = true;
        return newDirection;
    }

    DirectionVector smoothed;
    float factor = m_config.directionSmoothingFactor;

    // 平滑笛卡尔坐标
    smoothed.x = m_lastDirection.x * (1.0f - factor) + newDirection.x * factor;
    smoothed.y = m_lastDirection.y * (1.0f - factor) + newDirection.y * factor;
    smoothed.z = m_lastDirection.z * (1.0f - factor) + newDirection.z * factor;

    // 重新计算球坐标
    smoothed = CartesianToSpherical(smoothed.x, smoothed.y, smoothed.z);

    m_lastDirection = smoothed;
    return smoothed;
}

float DirectionProcessor::ApplyIntensitySmoothing(float newIntensity)
{
    if (!m_hasLastValues)
    {
        m_lastIntensity = newIntensity;
        return newIntensity;
    }

    float factor = m_config.intensitySmoothingFactor;
    float smoothed = m_lastIntensity * (1.0f - factor) + newIntensity * factor;
    
    m_lastIntensity = smoothed;
    return smoothed;
}

DirectionVector DirectionProcessor::FilterNoise(const DirectionVector& direction, float intensity)
{
    if (intensity < m_config.noiseFloor)
    {
        // 强度太低，可能是噪声
        DirectionVector filtered = direction;
        filtered.x *= 0.5f;
        filtered.y *= 0.5f;
        filtered.z *= 0.5f;
        return filtered;
    }

    return direction;
}

DirectionVector DirectionProcessor::FindDominantDirection(const std::vector<DirectionVector>& directions)
{
    if (directions.empty())
        return DirectionVector();

    // 找到距离最大的方向作为主导方向
    auto maxElement = std::max_element(directions.begin(), directions.end(),
        [](const DirectionVector& a, const DirectionVector& b) {
            return a.distance < b.distance;
        });

    return *maxElement;
}

void DirectionProcessor::UpdateHistory(const DirectionVector& direction, float intensity, float confidence)
{
    size_t index = m_history->currentIndex % m_history->maxSize;
    
    m_history->directions[index] = direction;
    m_history->intensities[index] = intensity;
    m_history->confidences[index] = confidence;
    
    m_history->currentIndex++;

    // 计算方向稳定性
    if (m_history->currentIndex >= m_history->maxSize)
    {
        float totalVariation = 0.0f;
        size_t count = std::min(m_history->currentIndex, m_history->maxSize);
        
        for (size_t i = 1; i < count; i++)
        {
            float diff = AngleDifference(m_history->directions[i].azimuth, 
                                       m_history->directions[i-1].azimuth);
            totalVariation += diff;
        }
        
        m_directionStability = 1.0f - (totalVariation / (count * 180.0f));
        m_directionStability = std::clamp(m_directionStability, 0.0f, 1.0f);
    }
}

float DirectionProcessor::CalculateDirectionWeight(const DirectionVector& direction, CardinalDirection cardinal)
{
    // 找到对应的目标角度
    auto it = std::find_if(s_cardinalDirections.begin(), s_cardinalDirections.end(),
        [cardinal](const auto& pair) { return pair.first == cardinal; });
    
    if (it == s_cardinalDirections.end())
        return 0.0f;

    float targetAzimuth = it->second.first;
    float targetElevation = it->second.second;

    // 计算角度差异
    float azimuthDiff = AngleDifference(direction.azimuth, targetAzimuth);
    float elevationDiff = std::abs(direction.elevation - targetElevation);

    // 转换为权重 (角度差异越小，权重越高)
    float azimuthWeight = std::max(0.0f, 1.0f - azimuthDiff / 180.0f);
    float elevationWeight = std::max(0.0f, 1.0f - elevationDiff / 90.0f);

    return (azimuthWeight + elevationWeight) * 0.5f;
}

float DirectionProcessor::NormalizeAngle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float DirectionProcessor::AngleDifference(float angle1, float angle2)
{
    float diff = std::abs(NormalizeAngle(angle1) - NormalizeAngle(angle2));
    return std::min(diff, 360.0f - diff);
}

float DirectionProcessor::VectorMagnitude(float x, float y, float z)
{
    return std::sqrt(x * x + y * y + z * z);
}

DirectionVector DirectionProcessor::NormalizeVector(const DirectionVector& vector)
{
    float magnitude = VectorMagnitude(vector.x, vector.y, vector.z);
    
    if (magnitude < 0.001f)
        return vector;

    DirectionVector normalized = vector;
    normalized.x /= magnitude;
    normalized.y /= magnitude;
    normalized.z /= magnitude;
    
    return normalized;
}