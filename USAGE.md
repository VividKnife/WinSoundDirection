# 使用指南

## 安装和运行

### 系统要求
- Windows 10/11 (64位)
- 支持空间音效的音频设备（推荐）
- Visual C++ Redistributable 2019或更新版本

### 运行程序
1. 下载或编译 `SpatialAudioVisualizer.exe`
2. 双击运行程序
3. 程序会自动检测音频设备和空间音效支持
4. 如果支持空间音效，会显示"Spatial Audio支持：YES"
5. 如果不支持，会自动使用立体声模拟模式

## 界面说明

### 主界面
- **圆形指示器**: 显示声音来源方向
- **指南针**: 显示方向参考（可选）
- **强度条**: 显示音频强度（右上角）

### 方向指示
- **前方**: 12点方向
- **后方**: 6点方向  
- **左侧**: 9点方向
- **右侧**: 3点方向
- **上方**: 中心向上
- **下方**: 中心向下

### 视觉反馈
- **亮度**: 表示声音强度
- **大小**: 表示声音距离
- **颜色**: 可自定义主题

## 操作方法

### 快捷键操作
- **Home键**: 快速切换显示/隐藏
- **Ctrl+Shift+Q**: 退出程序

### 鼠标操作
- **拖拽**: 移动窗口位置
- **双击**: 切换显示/隐藏
- **右键**: 显示设置菜单（未来版本）

### 系统托盘
- **双击图标**: 恢复显示
- **右键图标**: 显示菜单
  - 显示/隐藏
  - 设置（未来版本）
  - 退出

## 配置文件

配置文件位置：`%APPDATA%\SpatialAudioVisualizer\config.json`

### 主要配置项

```json
{
  "version": "1.0",
  "audio": {
    "sensitivity": 0.5,
    "noiseThreshold": 0.1,
    "enableDirectionFiltering": true,
    "updateFrequency": 60
  },
  "visual": {
    "transparency": 0.8,
    "indicatorSize": 50,
    "showCompass": true,
    "showIntensityMeter": true
  },
  "window": {
    "position_x": 100,
    "position_y": 100,
    "size_width": 200,
    "size_height": 200,
    "alwaysOnTop": true,
    "clickThrough": false
  },
  "hotkey": {
    "toggleKey": 36,
    "toggleModifiers": 0,
    "enableGlobalHotkeys": true,
    "showTrayIcon": true
  }
}
```

### 配置说明

#### 音频配置 (audio)
- `sensitivity`: 音频敏感度 (0.0-1.0)
- `noiseThreshold`: 噪声阈值 (0.0-1.0)
- `enableDirectionFiltering`: 启用方向过滤
- `updateFrequency`: 更新频率 (Hz)

#### 视觉配置 (visual)
- `transparency`: 透明度 (0.0-1.0)
- `indicatorSize`: 指示器大小 (像素)
- `showCompass`: 显示指南针
- `showIntensityMeter`: 显示强度计

#### 窗口配置 (window)
- `position_x/y`: 窗口位置
- `size_width/height`: 窗口大小
- `alwaysOnTop`: 始终置顶
- `clickThrough`: 点击穿透

#### 快捷键配置 (hotkey)
- `toggleKey`: 切换键虚拟键码
- `toggleModifiers`: 修饰键
- `enableGlobalHotkeys`: 启用全局快捷键
- `showTrayIcon`: 显示托盘图标

## 故障排除

### 常见问题

#### 1. 程序无法启动
- 检查是否安装了Visual C++ Redistributable
- 检查Windows版本是否支持
- 查看日志文件：`spatial_audio_visualizer.log`

#### 2. 检测不到空间音效
- 确认音频设备支持空间音效
- 检查Windows空间音效设置
- 尝试重新启动程序

#### 3. 方向检测不准确
- 调整音频敏感度设置
- 检查音频设备驱动程序
- 确认游戏音频设置正确

#### 4. 界面不显示
- 按Home键切换显示
- 检查窗口是否移动到屏幕外
- 重置配置文件

#### 5. 快捷键不工作
- 检查是否有其他程序占用快捷键
- 确认全局快捷键功能已启用
- 尝试以管理员身份运行

### 日志文件
程序运行时会生成日志文件 `spatial_audio_visualizer.log`，包含：
- 启动和关闭信息
- 音频设备检测结果
- 错误和警告信息
- 性能统计数据

### 重置配置
如果程序出现问题，可以删除配置文件重置为默认设置：
1. 关闭程序
2. 删除 `%APPDATA%\SpatialAudioVisualizer\config.json`
3. 重新启动程序

## 性能优化建议

1. **降低更新频率**: 如果CPU使用率过高，可以降低`updateFrequency`
2. **关闭不必要功能**: 关闭指南针和强度计可以提升性能
3. **调整敏感度**: 适当提高噪声阈值可以减少不必要的计算
4. **使用点击穿透**: 启用后可以避免意外点击影响游戏

## 游戏兼容性

### 测试过的游戏
- Counter-Strike 2
- Valorant  
- Call of Duty系列
- Battlefield系列
- Rainbow Six Siege

### 最佳实践
1. 在游戏中启用空间音效
2. 使用支持空间音效的耳机
3. 调整游戏音频设置为最高质量
4. 将程序窗口放置在不影响游戏的位置

## 技术支持

如果遇到问题，请提供以下信息：
- Windows版本
- 音频设备型号
- 程序日志文件
- 问题的详细描述

联系方式：请在GitHub项目页面提交Issue