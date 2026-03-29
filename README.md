# 轻量级 2D 游戏引擎

一个基于 C++、SDL3 和 Dear ImGui 的轻量级 2D 游戏引擎实验项目。

当前项目仍处于早期框架搭建阶段，目标是从底层出发，逐步实现一个结构清晰、便于学习、可持续扩展的 2D 引擎，并在此基础上开发一个小型 Demo 游戏完成验证。

## 项目简介

这个项目不追求复刻大型商业游戏引擎，而是聚焦于 2D 游戏开发的核心能力，包括：

- 窗口创建与管理
- 2D 渲染流程
- 输入事件处理
- 资源加载与释放
- 游戏主循环
- 基础场景数据管理
- 简单编辑器界面原型

目前仓库的重点是先把整体骨架搭起来，再逐步补齐各模块细节。

## 当前进度

已经完成的部分：

- `Engine` 总控类与程序入口
- SDL3 窗口初始化与基础控制
- SDL3 Renderer 基础渲染流程
- 输入管理基础框架
- 资源管理基础框架
- `SceneState` / `EditorState` 数据结构
- Dear ImGui 编辑器原型界面
- CMake 工程与第三方依赖接入

仍在建设中的部分：

- 更完整的纹理与精灵系统
- 更规范的对象系统
- 固定帧率与时间步长管理
- 更完整的输入状态系统
- 资源缓存与生命周期管理
- 编辑器面板模块化重构
- Demo 游戏逻辑与验证内容

## 技术栈

- C++20
- SDL3
- SDL3_image
- Dear ImGui
- CMake
- Ninja
- Visual Studio 2022（Windows 开发推荐）

说明：

- 早期申报材料里曾使用 SDL2 的表述。
- 当前代码仓库实际已经切换到 SDL3 / SDL3_image。
- 后续文档会以当前代码实现为准。

## 目录结构

```text
2D-Engine/
├─ asset/                  # 项目资源
├─ external/               # 第三方依赖源码（SDL / SDL_image / ImGui）
├─ include/                # 预留公共头文件目录
├─ src/
│  └─ editor/              # 编辑器 UI 与面板原型
├─ main.cpp                # 程序入口
├─ Engine.*                # 引擎总控
├─ WindowManager.*         # 窗口管理
├─ Renderer2D.*            # 2D 渲染
├─ InputManager.*          # 输入管理
├─ ResourceManager.*       # 资源管理
├─ GameLoop.*              # 游戏循环
├─ SceneState.h            # 场景数据
├─ EditorState.h           # 编辑器状态
├─ TextureManager.*        # 预留模块
└─ Sprite.*                # 预留模块
```

## 环境配置

### 1. 开发环境要求

如果你准备在 Windows 上继续开发，建议使用下面这套环境：

- Windows 10 / 11
- Visual Studio 2022
- Visual Studio 安装时勾选 `Desktop development with C++`
- CMake
- Ninja
- 支持 C++20 的 MSVC 编译器

如果没有勾选 `Desktop development with C++`，Visual Studio 通常无法正常配置和编译 C++ CMake 项目。

### 2. 第三方依赖准备方式

本项目采用的是：

`Visual Studio + CMake + SDL3 源码子目录 + SDL3_image 源码子目录`

这种方式的特点是：

- 不需要手动在属性页里逐个填写 `include` 和 `lib` 路径
- 更适合 CMake 工程
- 更适合当前这个仓库的组织方式

当前仓库已经将依赖源码放在：

- `external/SDL`
- `external/SDL_image`
- `external/imgui`

也就是说，只要这些目录内容完整，通常不需要再手动配置 Visual Studio 属性页。

### 3. Visual Studio 2022 安装建议

推荐安装流程：

1. 安装 Visual Studio 2022 Community
2. 在安装器中勾选 `Desktop development with C++`
3. 保持默认组件即可，除非你明确知道自己需要裁剪
4. 安装完成后重启一次电脑

如果你是第一次搭建环境，这一步最关键。

### 4. 当前项目的依赖接入方式

本仓库的 `CMakeLists.txt` 已经通过 `add_subdirectory(...)` 方式接入 SDL3 和 SDL3_image：

```cmake
add_subdirectory(external/SDL)

set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL_image)
```

这意味着：

- SDL3 从 `external/SDL` 直接参与构建
- SDL3_image 从 `external/SDL_image` 直接参与构建
- 不需要手动配置传统 `.lib` 链接路径
- `SDLIMAGE_VENDORED OFF` 用于减少 `zlib` 等依赖相关报错

### 5. 推荐打开项目的方式

建议直接用 Visual Studio 打开包含 `CMakeLists.txt` 的项目文件夹，而不是自己新建传统 `.vcxproj` 工程。

推荐方式：

1. 打开 Visual Studio
2. 选择 `File > Open > Folder`
3. 打开当前仓库根目录 `2D-Engine`
4. 等待 Visual Studio 自动识别并配置 CMake

## 构建与运行

### 使用 CMake Presets

仓库内已包含 `CMakePresets.json`，目前主要提供：

- `x64-debug`
- `x64-release`
- `x86-debug`
- `x86-release`

Windows 下推荐先使用 `x64-debug`。

### 命令行构建

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
```

生成后的构建目录通常位于：

```text
out/build/x64-debug/
```

### Visual Studio 中构建

如果你是通过 Visual Studio 直接打开文件夹：

1. 等待 CMake 自动配置完成
2. 选择 `x64-debug` 之类的 preset
3. 点击生成或运行

### 运行时文件复制

当前 `CMakeLists.txt` 已经加入了构建后复制逻辑，会自动处理这些内容：

- 将 `asset/` 目录复制到输出目录
- 将 `SDL3.dll` 复制到可执行文件目录
- 将 `SDL3_image.dll` 复制到可执行文件目录

所以在正常构建成功的前提下，一般不需要再手动复制 DLL。

## 新手环境检查清单

如果你想确认自己的开发环境是不是基本配好了，可以依次检查：

1. Visual Studio 2022 是否已安装
2. 是否勾选了 `Desktop development with C++`
3. 是否能正常打开这个带 `CMakeLists.txt` 的项目目录
4. `external/SDL` 是否包含完整 SDL3 源码
5. `external/SDL_image` 是否包含完整 SDL3_image 源码
6. `external/imgui` 是否存在
7. Visual Studio 是否能成功完成 CMake Configure
8. 是否能成功 Build 出可执行文件

## 常见问题

### 1. `cmake` 命令找不到

这通常说明：

- 没有安装 CMake
- 或者 CMake 没有加入环境变量
- 或者当前终端不是带开发工具链环境的终端

如果你主要用 Visual Studio，可以先直接在 Visual Studio 里打开项目并构建。

### 2. `cl.exe` 找不到

这通常说明没有安装 MSVC 工具链，或者没装 C++ 工作负载。

优先检查 Visual Studio Installer 中是否勾选了：

`Desktop development with C++`

### 3. CMake 改完后还是报旧错

这很常见，通常是 CMake 缓存没有刷新。

可以尝试：

- 关闭 Visual Studio
- 删除 `out/build` 下的旧缓存目录
- 重新打开项目，让 CMake 重新配置

### 4. SDL3_image 报 `zlib` 相关错误

这个问题在新手配置时比较常见。当前项目已经在 `CMakeLists.txt` 里设置了：

```cmake
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
```

这就是为了尽量避开这类常见错误。

### 5. 运行时找不到 DLL

当前工程理论上会在构建后自动复制：

- `SDL3.dll`
- `SDL3_image.dll`

如果仍然报错，先检查输出目录里是否真的存在这些 DLL。

### 6. 程序能运行但图片加载失败

这通常不是 SDL 环境完全失败，而是资源路径问题。

例如当前代码里初始化时尝试加载的是：

```cpp
test.png
```

如果运行目录里没有这张图，程序就会加载失败。后续项目推进时，资源路径会进一步整理。

## 项目架构概览

当前项目的运行主链路大致如下：

1. `main.cpp` 创建并启动 `Engine`
2. `Engine` 负责初始化窗口、渲染器、资源、ImGui 和状态数据
3. `InputManager` 处理 SDL 事件
4. `GameLoop` 处理运行时逻辑更新
5. `Renderer2D` 绘制场景
6. `EditorUI` 绘制编辑器界面

这个阶段的目标不是一次性做完所有功能，而是先把主链路打通。

## 开发路线

后续计划按下面几个方向逐步推进：

1. 完善运行时核心模块
2. 补齐纹理、精灵、对象表示等基础抽象
3. 优化输入、资源和时间管理
4. 将编辑器界面从原型整理成模块化结构
5. 基于引擎实现一个小型 2D Demo 游戏
6. 同步完善文档、演示与项目说明材料

## 项目定位

这个项目更偏向于：

- 学习型
- 架构型
- 原理型
- 课程设计 / 比赛项目 / 个人引擎实验基础

它当前不是一个成熟可商用的游戏引擎，而是一个正在逐步成型的 2D 引擎项目。

## 说明

- 当前仓库仍处于早期阶段，接口和目录结构可能继续调整
- 部分模块文件已创建但尚未完成实现，这是当前开发规划的一部分
- 后续会继续补充更详细的模块设计、使用说明和 Demo 展示内容
