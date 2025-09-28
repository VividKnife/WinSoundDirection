#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <vector>
#include <array>
#include <memory>

// 处理配置
struct ProcessingConfig
{
    float sensitivityThreshold = 0.1f;
    float noiseFloor = 0.05f;
    float directionSmoothingFactor = 0.3f;
    float intensitySmoothingFactor = 0.5f;
    bool enableDirectionFiltering = true;
    bool enableIntensityNormalization = true;
    int historyBufferSize = 10;
};

// 方向历史记录
struct DirectionHistory
{
    std::vector<DirectionVector> directions;
    std::vector<float> intensities;
    std::vector<float> confidences;
    size_t maxSize;
    size_t currentIndex;

    DirectionHistory(size_t size = 10) : maxSize(size), currentIndex(0)
    {
        directions.resize(size);
        intensities.resize(size);
        confidences.resize(size);
    }
};

class DirectionProcessor
{
public:
    DirectionProcessor();
    ~DirectionProcessor();

    // 主要处理方法
    ProcessedDirection ProcessAudioData(const SpatialAudioData& data);
    
    // 配置管理
    void SetProcessingParameters(const ProcessingConfig& config);
    void UpdateConfig(const AudioConfig& audioConfig);

    // 方向分析
    CardinalDirection GetPrimaryCardinalDirection(const DirectionVector& direction);
    std::vector<CardinalDirection> GetSecondaryDirections(const DirectionVector& direction, float threshold = 0.3f);

    // 统计信息
    float GetAverageIntensity() const;
    float GetDirectionStability() const;
    void ResetHistory();

private:
    // 核心计算方法
    DirectionVector CalculatePrimaryDirection(const SpatialAudioData& data);
    float CalculateIntensity(const SpatialAudioData& data);
    float CalculateConfidence(const SpatialAudioData& data);
    bool ValidateDirection(const DirectionVector& direction);

    // 方向转换
    DirectionVector CartesianToSpherical(float x, float y, float z);
    DirectionVector SphericalToCartesian(float azimuth, float elevation, float distance);
    CardinalDirection SphericalToCardinal(float azimuth, float elevation);

    // 信号处理
    DirectionVector ApplyDirectionSmoothing(const DirectionVector& newDirection);
    float ApplyIntensitySmoothing(float newIntensity);
    DirectionVector FilterNoise(const DirectionVector& direction, float intensity);

    // 多音源处理
    std::vector<DirectionVector> SeparateMultipleSources(const SpatialAudioData& data);
    DirectionVector FindDominantDirection(const std::vector<DirectionVector>& directions);

    // 历史数据管理
    void UpdateHistory(const DirectionVector& direction, float intensity, float confidence);
    DirectionVector GetSmoothedDirection() const;
    float GetSmoothedIntensity() const;

    // 方向分类
    float CalculateDirectionWeight(const DirectionVector& direction, CardinalDirection cardinal);
    bool IsDirectionInRange(const DirectionVector& direction, CardinalDirection cardinal, float tolerance = 45.0f);

    // 数学工具
    float NormalizeAngle(float angle);
    float AngleDifference(float angle1, float angle2);
    float VectorMagnitude(float x, float y, float z);
    DirectionVector NormalizeVector(const DirectionVector& vector);

    // 成员变量
    ProcessingConfig m_config;
    AudioConfig m_audioConfig;
    
    // 历史数据
    std::unique_ptr<DirectionHistory> m_history;
    
    // 平滑滤波器状态
    DirectionVector m_lastDirection;
    float m_lastIntensity;
    bool m_hasLastValues;

    // 统计数据
    float m_averageIntensity;
    float m_directionStability;
    int m_processedFrames;

    // 方向映射表
    static const std::array<std::pair<CardinalDirection, std::pair<float, float>>, 10> s_cardinalDirections;
};