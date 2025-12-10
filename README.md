# AutoSplatoon

基于[ClubchatGames](https://github.com/nullstalgia/ClubchatGames)

---
**软件界面：**

![UI](image/AutoSplatoon_User_Interface.png)

---
**实机效果**

![实机效果](image/Performance_on_Switch.jpg)

---
**支持硬件：**

ESP32系列，e.g. ESP32 WROOM模组、ESP32 WROVER模组、ESP32 PICO V3芯片、ESP32 PICO D4芯片...

---
**使用教程：**

详见B站[【斯普拉遁】广场涂鸦自动化工具！保姆级教程](https://www.bilibili.com/video/BV1va411R7TJ?vd_source=08b359f4e68b47a7ff089bcfa5caa191)

![视频封面](image/Video_Cover.png)

---
**TODO**

- [x] 支持用户自定义绘图速度
- [ ] 内置图像灰度处理，无需使用Photoshop事先处理
- [ ] 根据画面黑白像素个数优化绘图速度
- [x] 支持从指定位置开始绘图
 
---
**Windows 编译与打包**

1. 安装 Qt 5.15.2（组件：`win64_msvc2019_64`）与 Visual Studio（或 Microsoft C++ Build Tools，包含 `nmake`）。
2. 将 Qt 安装路径设置为环境变量 `Qt5_DIR`（例如 `C:\Qt\5.15.2\msvc2019_64`）。
3. 在项目根目录执行：
   - `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Configuration release`
4. 构建完成后，可执行文件与依赖位于 `AutoSplatoon/dist`。

---
**Linux 使用 Docker 远程编译**

1. 在有 Docker 的 Linux 机器上执行：
   - `git clone https://github.com/saaak/AutoSplatoon.git`
   - `cd AutoSplatoon`
   - `bash scripts/docker-build-linux.sh`
2. 输出位于 `AutoSplatoon/dist`，为 Linux 可执行文件。
