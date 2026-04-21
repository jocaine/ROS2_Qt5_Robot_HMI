<!--
 * @Author: jocaine 2508087548@qq.com
 * @Date: 2023-09-02 07:23:43
 * @LastEditors: jocaine 2508087548@qq.com
 * @LastEditTime: 2026-04-21
 * @FilePath: /ROS2_Qt5_Robot_HMI/README_en.md
-->
<div align="center">

# ROS Qt5 GUI App

*A lightweight ROS1/ROS2 mobile robot human-machine interaction software*

[简体中文](./README.md) | [English](./README_en.md)

[![GitHub last commit](https://img.shields.io/github/last-commit/jocaine/ROS2_Qt5_Robot_HMI?style=flat-square)](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/commits/master)
[![GitHub stars](https://img.shields.io/github/stars/jocaine/ROS2_Qt5_Robot_HMI?style=flat-square)](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/jocaine/ROS2_Qt5_Robot_HMI?style=flat-square)](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/network/members)
[![GitHub issues](https://img.shields.io/github/issues/jocaine/ROS2_Qt5_Robot_HMI?style=flat-square)](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/issues)

![humble](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/ros_humble_build.yaml/badge.svg)
![foxy](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/ros_foxy_build.yaml/badge.svg)
![noetic](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/ros_noetic_build.yaml/badge.svg)
![galactic](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/ros_galactic_build.yaml/badge.svg)
![melodic](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/ros_melodic_build.yaml/badge.svg)
![windows](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/actions/workflows/windows_build.yaml/badge.svg)

</div>

## 📖 Introduction

This project is developed based on Qt5 and built with CMake, enabling the use of a single codebase in both ROS1/ROS2 systems. During compilation, the software automatically detects the ROS1/ROS2 environment variables and builds accordingly, achieving isolation between ROS communication and interface.

All features are self-implemented through custom drawing, making it easy to run on low-performance edge devices. The project has integrated CI to ensure compatibility across multiple ROS versions and system versions.

### ✨ Features

- ROS1 Communication Support - Basic features implemented, continuously optimized
- ROS2 Communication Support - Stable and long-term support maintenance
- ROSBridge Communication Support - Supports WebSocket connection and automatic reconnection
- Global/Local Map Display - Supports OccupancyGrid maps
- Real-time Robot Position Display - Based on TF transforms
- Robot Speed Dashboard - Real-time linear and angular velocity display
- Manual Robot Control - Supports velocity control
- Robot Relocation - Supports 2D Pose Estimate
- Single/Multi-point Navigation - Supports navigation goal setting
- Global/Local Path Display - Real-time planned path display
- Topological Point Editing - Visual editing of topological points
- Battery Level Display - Subscribes to BatteryState topic
- Map Obstacle Editing - Supports map editing
- Topological Path Editing - Visual editing of topological paths
- Map Load/Save - Supports map file management
- Camera Image Display - Supports multiple image streams
- Robot Footprint Display - Subscribes to footprint topic
- LiDAR Display - Supports LaserScan visualization
- Node Health Monitoring - Monitors ROS2 node liveness and topic data flow grouped by subsystem, with real-time system health indicator in toolbar (Normal / Degraded / Fault)

### 🖼️ Interface Preview

![Main Interface](./doc/images/main.png)
![Running Effect](./doc/images/main.gif)
![Mapping Effect](./doc/images/mapping.gif)

## 🚀 Quick Start

### Requirements

- **Operating System**: Ubuntu 18.04+ / Windows 10+
- **ROS Environment**: ROS1 (Melodic/Noetic) or ROS2 (Foxy/Galactic/Humble)
- **Qt5**: Qt5.12+ (Qt5 Core, Widgets, SVG)
- **CMake**: 3.16+
- **Compiler**: GCC 7+ / MSVC 2019+

## 🚀 Build and Usage

> **💡 Tip:** Click the tabs below to switch between different platform build and usage instructions

<details open>
<summary><b>🐧 Linux Platform</b></summary>

### Install Dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
  qtbase5-dev \
  qtbase5-private-dev \
  libqt5svg5-dev \
  qtbase5-dev-tools \
  libeigen3-dev \
  libgtest-dev \
  libsdl2-dev \
  libsdl2-image-dev
```

### CMake Upgrade

Systems with Ubuntu 20.04 and below come with an outdated CMake version that needs to be upgraded to 3.16+. Ubuntu 22.04 and above can skip this step.

```bash
wget https://cmake.org/files/v3.16/cmake-3.16.4-Linux-x86_64.sh -O cmake-install.sh
chmod +x cmake-install.sh
sudo ./cmake-install.sh --prefix=/usr/local --skip-license
```

### Build from Source

```bash
# Clone repository
git clone https://github.com/jocaine/ROS2_Qt5_Robot_HMI.git
cd ROS2_Qt5_Robot_HMI
```

#### Method 1: Manual CMake Build

```bash
# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)
```

#### Method 2: Use build.sh Script

```bash
./build.sh
```

#### Method 3: Use Gitee Mirror for Accelerated Build

Replaces third-party library sources with Gitee mirror for faster compilation:

```bash
./build_cn.sh
```

Or manually specify mirrors:

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -Ddockwidget_GIT_REPOSITORY=https://gitee.com/kqz2007/qt-advanced-docking-system_github.git \
  -Dnlohmann_json_GIT_REPOSITORY=https://gitee.com/athtan/json.git \
  -Dyaml-cpp_GIT_REPOSITORY=https://gitee.com/dragonet_220/yaml-cpp.git \
  -Dwebsocketpp_GIT_REPOSITORY=https://gitee.com/open-source-software_1/websocketpp.git
make -j$(nproc)
```

### Run

#### Method 1: Using Startup Script (Recommended)

After building, the startup script will be automatically copied to the `build` directory:

```bash
cd build
./start.sh
```

The startup script will automatically set library file paths and launch the program.

#### Method 2: Manual Run

```bash
cd build
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
./ros_qt5_gui_app
```

#### Method 3: Run After Installation {#method-3-run-after-installation}

```bash
cd build
make install

cd ../install/bin
./start.sh
```

</details>

<details>
<summary><b>🪟 Windows Platform</b></summary>

### Install Dependencies

Windows platform recommends using vcpkg to manage dependencies. The project includes a `vcpkg.json` manifest file that automatically installs all dependencies.

**Install dependencies using vcpkg:**

1. Install vcpkg (if not already installed):
```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. Set environment variable (optional, recommended):
```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\path\to\vcpkg", "User")
```

3. Install project dependencies:
```powershell
cd Ros_Qt5_Gui_App
vcpkg install --triplet x64-windows
```

**Note:** The first installation of large dependencies like Qt5 will take a long time (30-60 minutes) as they need to be compiled from source. Subsequent builds will use cache and be much faster.

### Build from Source

```powershell
# Clone repository
git clone https://github.com/jocaine/ROS2_Qt5_Robot_HMI.git
cd ROS2_Qt5_Robot_HMI
```

#### Method 1: Manual CMake Build

```powershell
# Create build directory
mkdir build
cd build

# Configure CMake (specify vcpkg toolchain)
cmake .. `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_WITH_TEST=OFF

# Build
cmake --build . --config Release --parallel

# Install
cmake --install . --config Release
```

#### Method 2: Use Gitee Mirror for Accelerated Build

Replaces third-party library sources with Gitee mirror for faster compilation:

```powershell
# Create build directory
mkdir build
cd build

# Configure CMake with Gitee mirror for acceleration
cmake .. `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_WITH_TEST=OFF `
  -Ddockwidget_GIT_REPOSITORY=https://gitee.com/kqz2007/qt-advanced-docking-system_github.git `
  -Dnlohmann_json_GIT_REPOSITORY=https://gitee.com/athtan/json.git `
  -Dyaml-cpp_GIT_REPOSITORY=https://gitee.com/dragonet_220/yaml-cpp.git `
  -Dwebsocketpp_GIT_REPOSITORY=https://gitee.com/open-source-software_1/websocketpp.git

# Build
cmake --build . --config Release --parallel

# Install
cmake --install . --config Release
```

### Run

#### Method 1: Using Startup Script (Recommended)

After building, the startup script will be automatically copied to the `build` directory:

```powershell
cd build
.\start.bat
```

The startup script will automatically set library file paths and launch the program.

#### Method 2: Manual Run

```powershell
cd build
.\ros_qt5_gui_app.exe
```

#### Method 3: Run After Installation {#method-3-run-after-installation-windows}

```powershell
cd build
cmake --install . --config Release

cd ..\install\bin
.\start.bat
```

</details>

## 📥 Release Binary Distribution

Download the binary package for your system version from the [release](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/releases) page:

- **Linux**: Download `.tar.gz` package, extract and refer to [Linux Method 3: Run After Installation](#method-3-run-after-installation) to run the program
- **Windows**: Download `.zip` package, extract and refer to [Windows Method 3: Run After Installation](#method-3-run-after-installation-windows) to run the program

### Configuration

Before first run, please ensure:

1. **ROS Environment Configured**: Make sure ROS setup.bash/setup.bat has been sourced
2. **Topic Configuration**: Check if topic names in the configuration interface match your ROS system
3. **Channel Selection**: Select the correct communication channel (ROS1/ROS2/ROSBridge) in the configuration interface

For detailed configuration instructions, please refer to [User Guide](./doc/usage_en.md)

## 📚 Documentation

- [User Guide](./doc/usage_en.md) - Feature usage tutorials
- [Development Guide](./doc/development_en.md) - Development environment setup and code structure
- [FAQ](./doc/faq_en.md) - FAQ and troubleshooting

## 🏗️ Project Structure

```
Ros_Qt5_Gui_App/
├── src/                    # Source code directory
│   ├── core/              # Core module (main program entry)
│   ├── mainwindow/        # Main window and interface
│   ├── common/            # Common libraries
│   ├── basic/             # Basic data structures
│   ├── channel/           # Communication channels (ROS1/ROS2/ROSBridge)
│   └── plugin/            # Plugin system
├── install/               # Installation scripts
│   ├── linux/bin/        # Linux startup scripts
│   └── windows/bin/       # Windows startup scripts
├── doc/                   # Documentation directory
├── cmake/                 # CMake modules
└── CMakeLists.txt        # Main CMake configuration file
```

## 🤝 Contributing

Welcome to submit [Issues](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/issues) and [Pull Requests](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/pulls)!

If you have any ideas or suggestions, feel free to submit them to [Issues](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/issues)!

### Contributing Guide

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📊 Star History

<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=jocaine/ROS2_Qt5_Robot_HMI&type=Timeline&theme=dark" />
    <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=jocaine/ROS2_Qt5_Robot_HMI&type=Timeline" />
    <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=jocaine/ROS2_Qt5_Robot_HMI&type=Timeline" width="75%" />
  </picture>
</div>

## 📱 Related Projects

This project is forked from [chengyangkj/Ros_Qt5_Gui_App](https://github.com/chengyangkj/Ros_Qt5_Gui_App), with continued iteration focused on ROS2 and HMI productization.

## 🔗 Upstream Repository

| Repository | Description |
|------------|-------------|
| [chengyangkj/Ros_Qt5_Gui_App](https://github.com/chengyangkj/Ros_Qt5_Gui_App) | Original upstream, supports ROS1/ROS2/ROSBridge |

## 💬 Contact

- **Issues**: [GitHub Issues](https://github.com/jocaine/ROS2_Qt5_Robot_HMI/issues)

## 📄 License

This project is licensed under the [MIT](LICENSE) License.

## 🙏 Acknowledgments

Thanks to all contributors and users for their support!
