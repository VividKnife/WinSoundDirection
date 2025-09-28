#!/usr/bin/env python3
"""
创建更好看的应用程序图标
"""

def create_enhanced_icon_data(size):
    """创建增强的图标像素数据"""
    pixels = []
    center = size // 2
    
    for y in range(size-1, -1, -1):  # BMP从下到上
        row = []
        for x in range(size):
            dx = x - center
            dy = y - center
            distance = (dx*dx + dy*dy) ** 0.5
            angle = 0
            if dx != 0 or dy != 0:
                import math
                angle = math.atan2(dy, dx)
            
            # 默认背景色（深蓝色）
            r, g, b = 26, 26, 46
            
            # 外圈边框
            if distance > center - 1:
                r, g, b = 16, 33, 62  # 更深的蓝色边框
            # 指南针圆环
            elif center - 8 < distance < center - 4:
                r, g, b = 15, 52, 96  # 指南针环颜色
            else:
                # 主要方向指示器
                import math
                
                # 计算是否在主要方向线上
                is_main_direction = False
                
                # 北方 (上)
                if abs(dx) < 2 and dy > center//3:
                    is_main_direction = True
                # 南方 (下)  
                elif abs(dx) < 2 and dy < -center//3:
                    is_main_direction = True
                # 东方 (右)
                elif abs(dy) < 2 and dx > center//3:
                    is_main_direction = True
                # 西方 (左)
                elif abs(dy) < 2 and dx < -center//3:
                    is_main_direction = True
                
                if is_main_direction:
                    r, g, b = 233, 69, 96  # 红色主方向
                
                # 次要方向指示器（对角线）
                elif abs(abs(dx) - abs(dy)) < 2 and distance > center//3:
                    r, g, b = 243, 156, 18  # 橙色次方向
                
                # 中心音频波纹
                elif distance < center//3:
                    wave_intensity = 1.0 - (distance / (center//3))
                    
                    # 创建波纹效果
                    wave_rings = [center//6, center//8, center//10]
                    for i, ring_radius in enumerate(wave_rings):
                        if abs(distance - ring_radius) < 1.5:
                            intensity = wave_intensity * (0.8 - i * 0.2)
                            r = int(0 * intensity + r * (1 - intensity))
                            g = int(212 * intensity + g * (1 - intensity))
                            b = int(170 * intensity + b * (1 - intensity))
                            break
                    
                    # 中心扬声器区域
                    if distance < center//8:
                        # 扬声器主体
                        if abs(dx) < center//16 and abs(dy) < center//12:
                            r, g, b = 255, 255, 255  # 白色扬声器
                        # 扬声器锥形
                        elif dx < -center//16 and abs(dy) < center//8:
                            r, g, b = 255, 255, 255  # 白色扬声器
                        # 音波
                        elif dx > center//16:
                            wave_distance = abs(distance - center//12)
                            if wave_distance < 1:
                                r, g, b = 0, 212, 170  # 青色音波
            
            # 添加一些光晕效果
            if distance < center - 2:
                glow_factor = 1.0 - (distance / center)
                glow_intensity = glow_factor * 0.1
                r = min(255, int(r + glow_intensity * 50))
                g = min(255, int(g + glow_intensity * 50))
                b = min(255, int(b + glow_intensity * 50))
            
            # BMP使用BGR顺序
            row.extend([b, g, r])
        
        # BMP行需要4字节对齐
        while len(row) % 4 != 0:
            row.append(0)
        
        pixels.extend(row)
    
    return bytearray(pixels)

def create_bmp_data(width, height, color_data):
    """创建BMP格式的图像数据"""
    file_size = 54 + len(color_data)
    bmp_header = bytearray([
        0x42, 0x4D,  # BM
        file_size & 0xFF, (file_size >> 8) & 0xFF, (file_size >> 16) & 0xFF, (file_size >> 24) & 0xFF,
        0x00, 0x00, 0x00, 0x00,  # 保留
        0x36, 0x00, 0x00, 0x00,  # 数据偏移
        0x28, 0x00, 0x00, 0x00,  # 信息头大小
        width & 0xFF, (width >> 8) & 0xFF, (width >> 16) & 0xFF, (width >> 24) & 0xFF,
        height & 0xFF, (height >> 8) & 0xFF, (height >> 16) & 0xFF, (height >> 24) & 0xFF,
        0x01, 0x00,  # 平面数
        0x18, 0x00,  # 位深度
        0x00, 0x00, 0x00, 0x00,  # 压缩方式
        len(color_data) & 0xFF, (len(color_data) >> 8) & 0xFF, (len(color_data) >> 16) & 0xFF, (len(color_data) >> 24) & 0xFF,
        0x00, 0x00, 0x00, 0x00,  # X分辨率
        0x00, 0x00, 0x00, 0x00,  # Y分辨率
        0x00, 0x00, 0x00, 0x00,  # 颜色数
        0x00, 0x00, 0x00, 0x00,  # 重要颜色数
    ])
    
    return bmp_header + color_data

def create_ico_file():
    """创建ICO文件"""
    sizes = [16, 24, 32, 48, 64]
    ico_data = bytearray()
    
    # ICO文件头
    ico_header = bytearray([
        0x00, 0x00,  # 保留
        0x01, 0x00,  # 类型
        len(sizes), 0x00,  # 图像数量
    ])
    
    ico_data.extend(ico_header)
    
    # 图像目录和数据
    image_data_offset = 6 + len(sizes) * 16
    image_data_list = []
    
    for size in sizes:
        pixel_data = create_enhanced_icon_data(size)
        bmp_data = create_bmp_data(size, size, pixel_data)
        image_data_list.append(bmp_data)
        
        # 目录项
        directory_entry = bytearray([
            size if size < 256 else 0,
            size if size < 256 else 0,
            0x00, 0x00,  # 颜色数和保留
            0x01, 0x00,  # 颜色平面数
            0x18, 0x00,  # 位深度
            len(bmp_data) & 0xFF, (len(bmp_data) >> 8) & 0xFF, (len(bmp_data) >> 16) & 0xFF, (len(bmp_data) >> 24) & 0xFF,
            image_data_offset & 0xFF, (image_data_offset >> 8) & 0xFF, (image_data_offset >> 16) & 0xFF, (image_data_offset >> 24) & 0xFF,
        ])
        
        ico_data.extend(directory_entry)
        image_data_offset += len(bmp_data)
    
    # 添加图像数据
    for image_data in image_data_list:
        ico_data.extend(image_data)
    
    return ico_data

def main():
    """主函数"""
    print("Creating enhanced application icon...")
    
    # 创建增强的ICO文件
    ico_data = create_ico_file()
    
    # 保存文件
    with open('resources/app_icon.ico', 'wb') as f:
        f.write(ico_data)
    
    print("Enhanced icon created: resources/app_icon.ico")
    print("Icon includes multiple sizes: 16x16, 24x24, 32x32, 48x48, 64x64")

if __name__ == "__main__":
    main()