#!/usr/bin/env python3
"""
图标生成脚本
将SVG图标转换为不同尺寸的PNG和ICO文件
"""

import os
import subprocess
from PIL import Image, ImageDraw, ImageFont
import io

def create_simple_icon(size):
    """创建简单的图标（不依赖SVG）"""
    # 创建图像
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 背景圆形
    margin = size // 16
    bg_color = (26, 26, 46, 255)  # #1a1a2e
    draw.ellipse([margin, margin, size-margin, size-margin], fill=bg_color)
    
    # 外圈
    circle_margin = size // 8
    circle_color = (15, 52, 96, 180)  # #0f3460 with opacity
    draw.ellipse([circle_margin, circle_margin, size-circle_margin, size-circle_margin], 
                outline=circle_color, width=max(1, size//64))
    
    # 中心点
    center = size // 2
    
    # 主要方向线
    line_length = size // 6
    line_width = max(2, size // 32)
    main_color = (233, 69, 96, 255)  # #e94560
    
    # 北
    draw.line([center, margin*2, center, margin*2 + line_length], 
              fill=main_color, width=line_width)
    # 东
    draw.line([size-margin*2, center, size-margin*2-line_length, center], 
              fill=main_color, width=line_width)
    # 南
    draw.line([center, size-margin*2, center, size-margin*2-line_length], 
              fill=main_color, width=line_width)
    # 西
    draw.line([margin*2, center, margin*2+line_length, center], 
              fill=main_color, width=line_width)
    
    # 中心音频波纹
    wave_color = (0, 212, 170, 200)  # #00d4aa
    wave_sizes = [size//6, size//8, size//10]
    for i, wave_size in enumerate(wave_sizes):
        opacity = 200 - i * 50
        draw.ellipse([center-wave_size, center-wave_size, center+wave_size, center+wave_size], 
                    outline=(*wave_color[:3], opacity), width=max(1, size//64))
    
    # 中心扬声器
    speaker_size = size // 16
    speaker_color = (255, 255, 255, 255)
    
    # 扬声器主体
    draw.rectangle([center-speaker_size//2, center-speaker_size, 
                   center+speaker_size//2, center+speaker_size], fill=speaker_color)
    
    # 扬声器锥形
    cone_points = [
        (center-speaker_size//2, center-speaker_size//2),
        (center-speaker_size, center-speaker_size),
        (center-speaker_size, center+speaker_size),
        (center-speaker_size//2, center+speaker_size//2)
    ]
    draw.polygon(cone_points, fill=speaker_color)
    
    # 音波弧线（简化为圆弧）
    wave_start = center + speaker_size
    for i in range(2):
        wave_radius = speaker_size + (i+1) * speaker_size//2
        draw.arc([wave_start-wave_radius, center-wave_radius, 
                 wave_start+wave_radius, center+wave_radius], 
                start=-45, end=45, fill=wave_color, width=max(1, size//64))
    
    return img

def generate_ico_file(sizes, output_path):
    """生成ICO文件"""
    images = []
    for size in sizes:
        img = create_simple_icon(size)
        images.append(img)
    
    # 保存为ICO文件
    images[0].save(output_path, format='ICO', sizes=[(img.width, img.height) for img in images])
    print(f"Generated ICO: {output_path}")

def generate_png_files():
    """生成不同尺寸的PNG文件"""
    sizes = [16, 24, 32, 48, 64, 96, 128, 256, 512]
    
    for size in sizes:
        img = create_simple_icon(size)
        output_path = f"resources/icon_{size}x{size}.png"
        img.save(output_path, format='PNG')
        print(f"Generated PNG: {output_path}")

def main():
    """主函数"""
    # 确保resources目录存在
    os.makedirs('resources', exist_ok=True)
    
    print("Generating application icons...")
    
    # 生成PNG文件
    generate_png_files()
    
    # 生成ICO文件（包含多个尺寸）
    ico_sizes = [16, 24, 32, 48, 64, 128, 256]
    generate_ico_file(ico_sizes, 'resources/app_icon.ico')
    
    # 生成小尺寸ICO（用于托盘）
    tray_sizes = [16, 24, 32]
    generate_ico_file(tray_sizes, 'resources/tray_icon.ico')
    
    print("Icon generation completed!")

if __name__ == "__main__":
    main()