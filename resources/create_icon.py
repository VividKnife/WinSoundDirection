#!/usr/bin/env python3
"""
简单的图标创建脚本
创建应用程序图标，不依赖复杂的库
"""

def create_bmp_data(width, height, color_data):
    """创建BMP格式的图像数据"""
    # BMP文件头
    file_size = 54 + len(color_data)
    bmp_header = bytearray([
        0x42, 0x4D,  # BM
        file_size & 0xFF, (file_size >> 8) & 0xFF, (file_size >> 16) & 0xFF, (file_size >> 24) & 0xFF,  # 文件大小
        0x00, 0x00, 0x00, 0x00,  # 保留
        0x36, 0x00, 0x00, 0x00,  # 数据偏移
        0x28, 0x00, 0x00, 0x00,  # 信息头大小
        width & 0xFF, (width >> 8) & 0xFF, (width >> 16) & 0xFF, (width >> 24) & 0xFF,  # 宽度
        height & 0xFF, (height >> 8) & 0xFF, (height >> 16) & 0xFF, (height >> 24) & 0xFF,  # 高度
        0x01, 0x00,  # 平面数
        0x18, 0x00,  # 位深度 (24位)
        0x00, 0x00, 0x00, 0x00,  # 压缩方式
        len(color_data) & 0xFF, (len(color_data) >> 8) & 0xFF, (len(color_data) >> 16) & 0xFF, (len(color_data) >> 24) & 0xFF,  # 图像大小
        0x00, 0x00, 0x00, 0x00,  # X分辨率
        0x00, 0x00, 0x00, 0x00,  # Y分辨率
        0x00, 0x00, 0x00, 0x00,  # 颜色数
        0x00, 0x00, 0x00, 0x00,  # 重要颜色数
    ])
    
    return bmp_header + color_data

def create_simple_icon_data(size):
    """创建简单的图标像素数据"""
    # 创建像素数据 (BGR格式，从下到上)
    pixels = []
    center = size // 2
    
    for y in range(size-1, -1, -1):  # BMP从下到上
        row = []
        for x in range(size):
            # 计算到中心的距离
            dx = x - center
            dy = y - center
            distance = (dx*dx + dy*dy) ** 0.5
            
            # 背景色
            if distance > center - 2:
                # 边框
                r, g, b = 22, 22, 46  # 深蓝色边框
            elif distance > center - 8:
                # 外圈
                r, g, b = 26, 26, 46  # 深蓝色背景
            else:
                # 内部区域
                r, g, b = 26, 26, 46
                
                # 添加方向指示线
                if abs(dx) < 2 and abs(dy) > center//3:  # 垂直线
                    r, g, b = 233, 69, 96  # 红色
                elif abs(dy) < 2 and abs(dx) > center//3:  # 水平线
                    r, g, b = 233, 69, 96  # 红色
                elif distance < center//4:  # 中心区域
                    r, g, b = 0, 212, 170  # 青色
            
            # BMP使用BGR顺序
            row.extend([b, g, r])
        
        # BMP行需要4字节对齐
        while len(row) % 4 != 0:
            row.append(0)
        
        pixels.extend(row)
    
    return bytearray(pixels)

def create_ico_file():
    """创建ICO文件"""
    sizes = [16, 32, 48]
    ico_data = bytearray()
    
    # ICO文件头
    ico_header = bytearray([
        0x00, 0x00,  # 保留
        0x01, 0x00,  # 类型 (1 = ICO)
        len(sizes), 0x00,  # 图像数量
    ])
    
    ico_data.extend(ico_header)
    
    # 图像目录
    image_data_offset = 6 + len(sizes) * 16  # 头部 + 目录项
    image_data_list = []
    
    for size in sizes:
        # 创建图像数据
        pixel_data = create_simple_icon_data(size)
        bmp_data = create_bmp_data(size, size, pixel_data)
        image_data_list.append(bmp_data)
        
        # 目录项
        directory_entry = bytearray([
            size if size < 256 else 0,  # 宽度
            size if size < 256 else 0,  # 高度
            0x00,  # 颜色数 (0 = 不使用调色板)
            0x00,  # 保留
            0x01, 0x00,  # 颜色平面数
            0x18, 0x00,  # 位深度
            len(bmp_data) & 0xFF, (len(bmp_data) >> 8) & 0xFF, (len(bmp_data) >> 16) & 0xFF, (len(bmp_data) >> 24) & 0xFF,  # 数据大小
            image_data_offset & 0xFF, (image_data_offset >> 8) & 0xFF, (image_data_offset >> 16) & 0xFF, (image_data_offset >> 24) & 0xFF,  # 数据偏移
        ])
        
        ico_data.extend(directory_entry)
        image_data_offset += len(bmp_data)
    
    # 添加图像数据
    for image_data in image_data_list:
        ico_data.extend(image_data)
    
    return ico_data

def main():
    """主函数"""
    print("Creating application icon...")
    
    # 创建ICO文件
    ico_data = create_ico_file()
    
    # 保存文件
    with open('resources/app_icon.ico', 'wb') as f:
        f.write(ico_data)
    
    print("Icon created: resources/app_icon.ico")

if __name__ == "__main__":
    main()