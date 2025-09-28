#!/usr/bin/env python3
"""
3D音效测试文件生成器
生成不同方向的测试音频文件，用于测试空间音效可视化工具
"""

import math
import wave
import struct
import os

def generate_tone(frequency, duration, sample_rate=44100, amplitude=0.5):
    """生成纯音调"""
    frames = int(duration * sample_rate)
    samples = []
    
    for i in range(frames):
        t = i / sample_rate
        sample = amplitude * math.sin(2 * math.pi * frequency * t)
        samples.append(sample)
    
    return samples

def apply_3d_effect(samples, azimuth, elevation=0, distance=1.0):
    """
    应用3D音效效果
    azimuth: 方位角 (-180 to 180, 0=前方, -90=左, 90=右, 180=后方)
    elevation: 仰角 (-90 to 90, 0=水平, 90=上方, -90=下方)  
    distance: 距离 (0.1 to 2.0, 1.0=标准距离)
    """
    # 转换为弧度
    azimuth_rad = math.radians(azimuth)
    elevation_rad = math.radians(elevation)
    
    # 计算左右声道的增益
    # 简化的HRTF模拟
    left_gain = 0.5 + 0.5 * math.cos(azimuth_rad + math.pi/2)
    right_gain = 0.5 + 0.5 * math.cos(azimuth_rad - math.pi/2)
    
    # 距离衰减
    distance_attenuation = 1.0 / max(0.1, distance)
    left_gain *= distance_attenuation
    right_gain *= distance_attenuation
    
    # 仰角影响（简化）
    elevation_factor = 1.0 - abs(elevation) / 180.0
    left_gain *= elevation_factor
    right_gain *= elevation_factor
    
    # 应用到样本
    stereo_samples = []
    for sample in samples:
        left_sample = sample * left_gain
        right_sample = sample * right_gain
        stereo_samples.extend([left_sample, right_sample])
    
    return stereo_samples

def save_wav_file(samples, filename, sample_rate=44100, channels=2):
    """保存WAV文件"""
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(channels)
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)
        
        # 转换为16位整数
        for sample in samples:
            wav_file.writeframes(struct.pack('<h', int(sample * 32767)))

def generate_test_files():
    """生成所有测试文件"""
    print("Generating 3D audio test files...")
    
    # 创建输出目录
    output_dir = "test_audio"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 测试配置
    test_configs = [
        {"name": "front", "azimuth": 0, "elevation": 0, "freq": 440, "desc": "Front - Enemy ahead"},
        {"name": "back", "azimuth": 180, "elevation": 0, "freq": 440, "desc": "Back - Enemy behind"},
        {"name": "left", "azimuth": -90, "elevation": 0, "freq": 440, "desc": "Left - Enemy on left"},
        {"name": "right", "azimuth": 90, "elevation": 0, "freq": 440, "desc": "Right - Enemy on right"},
        {"name": "front_left", "azimuth": -45, "elevation": 0, "freq": 440, "desc": "Front-Left - Enemy at 10 o'clock"},
        {"name": "front_right", "azimuth": 45, "elevation": 0, "freq": 440, "desc": "Front-Right - Enemy at 2 o'clock"},
        {"name": "back_left", "azimuth": -135, "elevation": 0, "freq": 440, "desc": "Back-Left - Enemy at 8 o'clock"},
        {"name": "back_right", "azimuth": 135, "elevation": 0, "freq": 440, "desc": "Back-Right - Enemy at 4 o'clock"},
        {"name": "above", "azimuth": 0, "elevation": 45, "freq": 880, "desc": "Above - Helicopter overhead"},
        {"name": "below", "azimuth": 0, "elevation": -45, "freq": 220, "desc": "Below - Enemy in basement"},
    ]
    
    duration = 2.0  # 2秒
    
    for config in test_configs:
        print(f"  Generating: {config['desc']}")
        
        # 生成基础音调
        mono_samples = generate_tone(config['freq'], duration)
        
        # 应用3D效果
        stereo_samples = apply_3d_effect(
            mono_samples, 
            config['azimuth'], 
            config['elevation']
        )
        
        # 保存文件
        filename = os.path.join(output_dir, f"test_{config['name']}.wav")
        save_wav_file(stereo_samples, filename)
        
        print(f"    ✓ Saved: {filename}")
    
    # 生成循环测试（敌人绕圈移动）
    print("  Generating: Circular movement test")
    generate_circular_test(output_dir)
    
    print(f"\n✓ Generated {len(test_configs) + 1} test files in {output_dir}/")
    print("\nTest files created:")
    for config in test_configs:
        print(f"  • test_{config['name']}.wav - {config['desc']}")
    print("  • test_circular.wav - Enemy moving in circle")

def generate_circular_test(output_dir):
    """生成敌人绕圈移动的测试文件"""
    duration = 8.0  # 8秒
    sample_rate = 44100
    frequency = 440
    
    frames = int(duration * sample_rate)
    stereo_samples = []
    
    for i in range(frames):
        t = i / sample_rate
        
        # 计算当前角度（完整一圈）
        angle = (t / duration) * 360 - 180  # -180 to 180
        
        # 生成音调
        sample = 0.3 * math.sin(2 * math.pi * frequency * t)
        
        # 应用3D效果
        mono_samples = [sample]
        stereo_frame = apply_3d_effect(mono_samples, angle, 0, 1.0)
        stereo_samples.extend(stereo_frame)
    
    filename = os.path.join(output_dir, "test_circular.wav")
    save_wav_file(stereo_samples, filename)

def create_test_playlist():
    """创建测试播放列表"""
    playlist_content = """# 3D Audio Test Playlist
# 
# Instructions:
# 1. Start SpatialAudioVisualizer.exe
# 2. Enable Windows Sonic for Headphones
# 3. Wear headphones
# 4. Play each file and watch the visualizer
#
# Test Files:

test_front.wav          # Should show indicator pointing UP (12 o'clock)
test_back.wav           # Should show indicator pointing DOWN (6 o'clock)  
test_left.wav           # Should show indicator pointing LEFT (9 o'clock)
test_right.wav          # Should show indicator pointing RIGHT (3 o'clock)
test_front_left.wav     # Should show indicator at 10-11 o'clock
test_front_right.wav    # Should show indicator at 1-2 o'clock
test_back_left.wav      # Should show indicator at 7-8 o'clock
test_back_right.wav     # Should show indicator at 4-5 o'clock
test_above.wav          # Should show UP indicator (higher pitch)
test_below.wav          # Should show DOWN indicator (lower pitch)
test_circular.wav       # Should show indicator rotating around center

# If the visualizer shows the correct directions, your setup is working!
"""
    
    with open("test_audio/PLAYLIST.txt", "w", encoding="utf-8") as f:
        f.write(playlist_content)
    
    print("✓ Created test playlist: test_audio/PLAYLIST.txt")

def main():
    """主函数"""
    print("========================================")
    print("3D Audio Test File Generator")
    print("========================================")
    print()
    
    try:
        generate_test_files()
        create_test_playlist()
        
        print("\n========================================")
        print("Generation Complete!")
        print("========================================")
        print()
        print("Next steps:")
        print("1. Start SpatialAudioVisualizer.exe")
        print("2. Enable Windows Sonic for Headphones")
        print("3. Play the test files in test_audio/")
        print("4. Watch the visualizer for direction indicators")
        print()
        print("Expected results:")
        print("• Front/Back sounds should trigger vertical indicators")
        print("• Left/Right sounds should trigger horizontal indicators") 
        print("• Diagonal sounds should trigger corner indicators")
        print("• Above/Below sounds should trigger up/down indicators")
        print("• Circular test should show rotating indicator")
        
    except Exception as e:
        print(f"Error generating test files: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())