# 轻量级 2D 游戏引擎与可视化编辑工具

> A lightweight 2D game engine and editor prototype built with C++, SDL3 and Dear ImGui.

本项目面向轻量级 2D 原型验证场景，目标是实现一套可持续扩展的“引擎内核 + 编辑器原型 + 样例验证”工具链，而不是单独完成一个游戏 Demo。

当前仓库处于原型开发阶段，已经完成基础运行主链路和一版可交互编辑器原型，后续将继续补齐碰撞、Demo、导出和项目管理能力。

## Overview

这个项目的核心方向是：

- 自研轻量级 2D 游戏引擎
- 基于 ImGui 的可视化编辑工具原型
- 面向课程设计 / 比赛项目 / 原型验证
- 通过平台跳跃类样例验证引擎能力

相比大型商业引擎，这个项目更强调：

- 结构清晰
- 便于理解
- 轻量可控
- 适合做底层学习和原型验证

## Current Status

当前已完成：

- SDL3 窗口初始化、销毁、尺寸调整、全屏切换
- 基础 2D 渲染主链路
- 输入事件循环
- 基于路径的纹理缓存加载
- 场景对象与编辑器状态管理
- ImGui Docking 编辑器主界面
- Hierarchy / Scene / Inspector / Project / Console 面板
- Play / Pause / Stop 编辑器命令流
- 场景 JSON 保存与加载

当前开发中：

- 固定时间步长与更完整的游戏循环
- 更标准的输入状态系统
- 对象 / 精灵 / 组件抽象
- 碰撞检测与平台跳跃逻辑
- 资源导入与真实资源浏览
- 项目管理与导出链路
- 平台跳跃样例 Demo

## Tech Stack

- C++20
- SDL3
- SDL3_image
- Dear ImGui
- nlohmann/json
- CMake
- Ninja
- Visual Studio 2022

## Project Structure

```text
2D-Engine/
├─ asset/                         # 项目资源
├─ backend/                       # 引擎后端
│  ├─ core/                       # Engine / GameLoop / SceneState
│  ├─ input/                      # 输入系统
│  ├─ render/                     # 2D 渲染
│  ├─ resource/                   # 资源管理
│  ├─ window/                     # 窗口管理
│  ├─ SceneSerializer.cpp
│  └─ SceneSerializer.h
├─ frontend/                      # 编辑器前端
│  └─ src/
│     ├─ EditorState.h
│     └─ editor/
│        ├─ EditorUI.cpp
│        ├─ EditorUI.h
│        └─ Panels/
├─ external/                      # 第三方依赖
│  ├─ SDL/
│  ├─ SDL_image/
│  ├─ imgui/
│  └─ json-develop/
├─ include/
├─ main.cpp
├─ CMakeLists.txt
└─ CMakePresets.json
```

## Architecture

当前项目采用“后端引擎 + 前端编辑器”的组织方式：

1. `main.cpp` 创建并启动 `Engine`
2. `backend/core/Engine` 串联窗口、渲染、输入、资源、场景与编辑器状态
3. `backend/*` 提供运行时基础能力
4. `frontend/src/editor/*` 提供基于 ImGui 的编辑器界面
5. `backend/SceneSerializer` 负责场景 JSON 序列化

当前编辑器已经具备最小交互闭环：

- 在 `Hierarchy` 选择对象
- 在 `Inspector` 修改对象位置、缩放和贴图路径
- 在 `Project` 面板切换对象贴图
- 在 `Scene` 面板增删对象、保存 / 加载场景
- 通过工具栏触发 `Play / Pause / Stop`

## Quick Start

### Requirements

- Windows 10 / 11
- Visual Studio 2022
- `Desktop development with C++`
- CMake
- Ninja
- MSVC with C++20 support

### Open In Visual Studio

推荐直接用 Visual Studio 打开项目文件夹：

1. 打开 Visual Studio
2. 选择 `File > Open > Folder`
3. 打开项目根目录 `2D-Engine`
4. 等待 CMake 自动配置

### Build From Command Line

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
```

构建输出通常位于：

```text
out/build/x64-debug/
```

## Dependencies

本项目通过源码子目录方式接入依赖，而不是手动配置属性页的 `include/lib` 路径。

依赖位置：

- `external/SDL`
- `external/SDL_image`
- `external/imgui`
- `external/json-develop`

这使得当前仓库更适合 CMake 工程和多人协作维护。

## Runtime Notes

当前 `CMakeLists.txt` 已配置构建后自动复制：

- `asset/`
- `SDL3.dll`
- `SDL3_image.dll`

当前对象贴图路径仍是原型级字符串，例如：

```text
test.png
player.png
enemy.png
```

如果运行目录缺少这些资源，纹理加载会失败。后续会进一步整理资源导入、资源目录和项目文件结构。

## Roadmap

下一阶段计划：

1. 完善时间系统与固定更新循环
2. 完善输入状态记录
3. 引入碰撞系统与平台跳跃核心逻辑
4. 扩展对象表示和资源管理
5. 完成更完整的编辑器交互
6. 实现项目保存 / 加载 / 导出闭环
7. 完成平台跳跃样例 Demo

## Current Modules

### Backend

- `WindowManager`：窗口初始化、销毁、分辨率调整、全屏切换
- `Renderer2D`：遍历场景对象并渲染纹理
- `InputManager`：处理 SDL 事件与基础快捷键
- `ResourceManager`：按路径缓存纹理并统一释放
- `GameLoop`：当前保留最小运行时更新逻辑
- `SceneSerializer`：场景 JSON 保存与加载

### Frontend

- `EditorUI`：整体 Docking 布局与顶层工具栏
- `HierarchyPanel`：对象列表与对象选择
- `ScenePanel`：场景概览、对象增删、属性重置、场景保存/加载
- `InspectorPanel`：编辑位置、缩放和贴图路径
- `AssetPanel`：原型级资源列表与贴图绑定
- `ConsolePanel`：调试输出占位面板

## Known Limitations

当前版本仍然是原型，不应视为成熟引擎。主要限制包括：

- 运行时逻辑仍然非常简化
- 尚未形成完整项目管理闭环
- 尚未完成碰撞系统和 Demo 验证
- 部分模块仍处于占位或过渡实现阶段
- 构建脚本和工程结构仍会继续整理

## ResourceManager Update Notes

This update fills out the resource-system foundation around `ResourceManager`.

Added files:

- `backend/resource/ResourcePathUtils.h`
  Declares the path helper functions used by the resource layer. It is responsible for path normalization, building search roots, and resource path resolution.
- `backend/resource/ResourcePathUtils.cpp`
  Implements the filesystem lookup logic for resource resolution. It supports direct relative paths, absolute paths, and recursive fallback search inside configured resource folders.
- `backend/resource/AssetRegistry.h`
  Declares the editor-facing asset registry used to register imported assets with ID, name, type and path metadata.
- `backend/resource/AssetRegistry.cpp`
  Implements project-style asset import, file copying, manifest persistence, recursive folder import, asset type detection, de-duplication by path and runtime lookup by resource ID or path.

Current `ResourceManager` responsibilities:

- texture loading
- cache reuse by resolved file path
- lookup by original identifier or resolved path
- explicit single-texture release and full-cache release
- renderer binding for repeated texture creation
- search-path management
- last-error reporting for failed resource operations

Default search behavior:

- current working directory
- `asset`
- `asset/image`
- runtime-added path in `Engine`: `asset/image/siheyuan`

Current asset import workflow:

- import image files or a folder from the editor's `Project` panel
- copy imported resources into the project asset library under `asset/imported`
- persist registered assets into `asset/asset_registry.json`
- reload the registry automatically on the next engine startup
- register each asset with resource ID, name, type, source path and relative path
- assign registered texture resources to scene objects through `textureResourceId`

## Development Philosophy

这个项目当前优先保证：

- 主链路先跑通
- 模块边界先明确
- 架构方向先稳定
- 之后再逐步补功能、补工具、补交付

所以仓库中的某些实现会明显偏“原型化”，这是当前开发策略的一部分。
