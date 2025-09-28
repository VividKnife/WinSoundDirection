#!/usr/bin/env python3
"""
图标预览工具
显示创建的图标的ASCII艺术预览
"""

def read_ico_file(filename):
    """读取ICO文件并返回图像信息"""
    try:
        with open(filename, 'rb') as f:
            data = f.read()
        
        # 读取ICO头部
        if len(data) < 6:
            return None
        
        # 检查ICO签名
        if data[0:2] != b'\x00\x00' or data[2:4] != b'\x01\x00':
            return None
        
        # 读取图像数量
        num_images = int.from_bytes(data[4:6], 'little')
        
        print(f"ICO文件包含 {num_images} 个图像:")
        
        # 读取每个图像的信息
        for i in range(num_images):
            offset = 6 + i * 16
            if offset + 16 > len(data):
                break
            
            width = data[offset] if data[offset] != 0 else 256
            height = data[offset + 1] if data[offset + 1] != 0 else 256
            colors = data[offset + 2]
            bit_depth = int.from_bytes(data[offset + 6:offset + 8], 'little')
            size = int.from_bytes(data[offset + 8:offset + 12], 'little')
            
            print(f"  图像 {i+1}: {width}x{height}, {bit_depth}位, {size}字节")
        
        return True
        
    except Exception as e:
        print(f"读取ICO文件时出错: {e}")
        return False

def create_ascii_preview():
    """创建ASCII艺术预览"""
    print("\n图标ASCII预览 (32x32):")
    print("┌" + "─" * 32 + "┐")
    
    # 创建简化的ASCII预览
    for y in range(16):  # 压缩到16行
        line = "│"
        for x in range(32):
            # 计算相对于中心的位置
            center_x, center_y = 16, 8
            dx = x - center_x
            dy = y - center_y
            distance = (dx*dx + dy*dy) ** 0.5
            
            char = ' '
            
            # 外圈
            if distance > 14:
                char = '█'
            elif distance > 12:
                char = '▓'
            # 主要方向线
            elif abs(dx) < 1 and abs(dy) > 4:  # 垂直线
                char = '║'
            elif abs(dy) < 1 and abs(dx) > 4:  # 水平线
                char = '═'
            # 对角线
            elif abs(abs(dx) - abs(dy)) < 1 and distance > 4:
                char = '╬'
            # 中心波纹
            elif 6 < distance < 8:
                char = '○'
            elif 4 < distance < 6:
                char = '◦'
            elif distance < 3:
                char = '●'
            
            line += char
        
        line += "│"
        print(line)
    
    print("└" + "─" * 32 + "┘")
    
    print("\n图标说明:")
    print("█▓ - 外边框")
    print("║═ - 主要方向指示器 (前后左右)")
    print("╬  - 次要方向指示器 (对角线)")
    print("○◦ - 音频波纹效果")
    print("●  - 中心扬声器")

def main():
    """主函数"""
    print("=== 空间音效可视化工具 - 图标预览 ===\n")
    
    # 检查ICO文件
    ico_file = "resources/app_icon.ico"
    print(f"检查图标文件: {ico_file}")
    
    if read_ico_file(ico_file):
        print("✓ ICO文件格式正确")
    else:
        print("✗ ICO文件读取失败")
        return
    
    # 显示ASCII预览
    create_ascii_preview()
    
    print("\n图标设计理念:")
    print("• 圆形设计代表360度全方位检测")
    print("• 十字线表示主要方向 (前后左右)")
    print("• 对角线表示次要方向 (东北、东南等)")
    print("• 中心波纹表示音频信号")
    print("• 深蓝色背景适合游戏界面")
    print("• 红色和橙色指示器醒目易识别")

if __name__ == "__main__":
    main()