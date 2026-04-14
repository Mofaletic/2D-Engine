# LancelotEngine

> A lightweight 2D engine and editor prototype built around C++, OpenGL and Dear ImGui.

本仓库当前以 `master` 分支为主线，定位是一个面向课程设计与原型验证的轻量级二维引擎项目。  
它不是成熟商用引擎，而是一个正在持续迭代的 `引擎底层 + 可视化编辑器 + 项目管理` 原型。

## Status

当前状态：`Active development`

主线已经具备这些核心方向的基础实现：

- OpenGL 渲染抽象层与 2D 渲染骨架
- 基于 ImGui 的编辑器工作区
- Scene 视口渲染到编辑器面板
- Project 面板驱动的项目与资源管理
- 资源导入、资产清单、场景保存加载
- 原生 C++ 脚本的运行时编译与绑定雏形

目前仍然是原型阶段，很多能力已经起步，但还没有形成完整的游戏引擎闭环。

## Overview

项目当前采用“后端引擎 + 前端编辑器”的结构：

- `backend/` 负责窗口、渲染、资源、项目、脚本、场景等运行时能力
- `frontend/` 负责基于 Dear ImGui 的编辑器界面
- `main.cpp` 启动 `Engine`
- `Engine` 串联窗口、渲染器、ImGui、项目状态、资源系统与编辑器命令流

从技术实现上看，这条主线不是 SDL Renderer 方案，而是：

- `GLFW + GLAD + OpenGL` 负责窗口上下文与渲染
- `Dear ImGui` 负责编辑器 UI
- `SDL3 + SDL3_image` 当前主要用于图像加载与部分平台辅助能力
- `nlohmann/json` 负责场景与项目相关序列化

## Current Capabilities

当前代码里已经能对应到的能力包括：

- `Renderer / RendererAPI / RenderCommand / OpenGL backend`
- `VertexBuffer / IndexBuffer / VertexArray / Shader / Material / Texture2D`
- 正交相机与 2D Quad 提交链路
- Scene 视口 FBO 渲染，并在 ImGui 面板中显示
- Scene 视口相机控制：`W A S D` 平移，`Q E` 旋转，滚轮缩放
- 层级面板、属性面板、场景面板、项目面板、控制台面板
- Play / Pause / Stop 编辑器模式切换
- 对象创建、删除、选择、拖拽移动、属性修改
- 场景保存 / 加载
- 资产导入、资产注册表、项目资源同步
- 项目创建 / 打开 / 基础文件管理
- 纹理资源拖拽到场景对象
- 脚本资源绑定到对象
- Windows 下原生 C++ 脚本运行时编译与动态加载雏形

## Tech Stack

- `C++20`
- `OpenGL`
- `GLFW`
- `GLAD`
- `Dear ImGui`
- `SDL3`
- `SDL3_image`
- `nlohmann/json`
- `CMake`
- `Ninja`
- `Visual Studio 2022`

## Repository Layout

```text
2D-Engine/
├─ app/                          # 编辑器侧控制器与上层应用逻辑
├─ asset/                        # 当前仓库中的示例资源
├─ backend/
│  ├─ core/                      # Engine / GameLoop / SceneState / Ref / Timestep
│  ├─ input/                     # 输入处理
│  ├─ platform/
│  │  ├─ imgui/                  # 自定义 ImGui 平台/渲染后端接入
│  │  └─ opengl/                 # OpenGL 具体实现
│  ├─ project/                   # 项目创建、打开、文件操作
│  ├─ render/                    # 渲染抽象层与 Renderer2D
│  ├─ resource/                  # ResourceManager / AssetRegistry / 路径处理
│  ├─ script/                    # 原生脚本运行时
│  ├─ window/                    # GLFW 窗口管理
│  ├─ SceneSerializer.*
│  └─ ...
├─ docs/                         # 阶段文档与里程碑说明
├─ external/                     # 第三方依赖
│  ├─ SDL/
│  ├─ SDL_image/
│  ├─ glad/
│  ├─ glfw/
│  ├─ imgui/
│  └─ json-develop/
├─ frontend/
│  └─ src/
│     ├─ EditorState.h
│     └─ editor/
│        ├─ EditorUI.*
│        └─ Panels/
├─ include/
├─ CMakeLists.txt
├─ CMakePresets.json
└─ main.cpp
```

## Build

### Requirements

- Windows 10 / 11
- Visual Studio 2022
- Visual Studio workload: `Desktop development with C++`
- CMake 3.16+
- Ninja
- MSVC `cl.exe`

### Recommended

推荐直接用 Visual Studio 打开仓库根目录：

1. 打开 Visual Studio 2022
2. 选择 `File > Open > Folder`
3. 选择仓库根目录 `2D-Engine`
4. 等待 CMake 配置完成

### Command Line

仓库当前只有 `configurePresets`，没有 `buildPresets`。  
因此命令行构建应使用下面这组命令：

```powershell
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

Release 版本：

```powershell
cmake --preset x64-release
cmake --build out/build/x64-release
```

## Run Notes

当前主线对运行目录和资源路径有要求，这一点需要提前知道：

- `Renderer2D` 会尝试读取 `assets/shaders/Renderer2D_Quad.glsl`
- 默认贴图搜索路径包括 `.`, `asset`, `asset/image`, `asset/image/siheyuan`
- 当前仓库里可见的示例贴图主要是 `asset/image/siheyuan/pillar.png`

这意味着：

- 如果工作目录不对，程序可能找不到资源
- 如果 `assets/shaders/Renderer2D_Quad.glsl` 缺失，渲染初始化会失败
- 当前仓库没有在 CMake 中自动复制资源目录到构建输出目录

更稳的做法是：

- 从仓库根目录启动程序，或
- 手动确保构建输出目录能访问到 `asset/` 与 `assets/` 所需资源

## Editor Workflow

当前编辑器原型大致支持这条链路：

1. 创建或打开项目
2. 在 `Project` 面板导入文件或目录
3. 资产写入项目目录并登记到 `asset_registry.json`
4. 在 `Hierarchy` / `Scene` 中创建对象
5. 在 `Inspector` 中编辑对象变换、贴图、脚本
6. 在 `Scene` 面板直接拖拽资产到对象或空白区域
7. 保存场景到项目目录
8. 进入 `Play` 模式做最小运行验证

## Current Modules

### Runtime

- `WindowManager`
  负责 GLFW 窗口初始化、尺寸变化、全屏切换和 framebuffer 回调。
- `RendererAPI / RenderCommand / Renderer`
  提供图形 API 抽象与统一提交入口。
- `Renderer2D`
  负责 Quad 渲染、Scene 视口 FBO、相机更新和对象绘制。
- `ResourceManager`
  负责纹理路径解析、SDL 图像解码、OpenGL 纹理创建、缓存与释放。
- `AssetRegistry`
  负责资产登记、导入、项目资源同步、清单保存和路径索引。
- `ProjectManager`
  负责项目创建、打开，以及项目内文件的创建、删除、重命名、移动。
- `ScriptRuntime`
  负责 Windows 下原生脚本的编译、加载与执行。
- `SceneSerializer`
  负责场景 JSON 存取。

### Editor

- `EditorUI`
  负责 DockSpace、菜单栏、工具栏和整体布局。
- `HierarchyPanel`
  负责对象树与选择。
- `ScenePanel`
  负责 Scene 视口显示、拖拽放置、保存场景和对象编辑快捷操作。
- `InspectorPanel`
  负责对象属性、贴图绑定、脚本绑定。
- `AssetPanel`
  负责项目文件浏览、导入、创建、删除、重命名、移动、文本预览。
- `ConsolePanel`
  负责日志显示与筛选。

## Known Limitations

当前版本仍然有几个很实际的限制：

- 项目仍是原型，不是稳定成品
- 运行时游戏逻辑还比较薄，`GameLoop` 还没有形成完整玩法层
- 没有成熟的组件系统、碰撞系统、动画系统、音频系统
- 渲染目前是基础 2D 提交链路，还没有批处理、图集、排序优化等完整性能方案
- 原生脚本运行时目前是 Windows-first，并依赖本机 `cl.exe`
- 部分资源与着色器路径仍需进一步整理
- 当前仓库里 `assets/shaders/Renderer2D_Quad.glsl` 路径疑似缺失，需要补齐或修正路径

## Roadmap

下一阶段更合理的推进方向是：

1. 补齐并稳定 `assets/shaders` 等运行必需资源
2. 整理渲染主链路，确保 Scene 视口与资源系统完全稳定
3. 补齐对象/组件抽象，减少当前直接写字段的状态管理方式
4. 完善项目工作流与资源导入闭环
5. 增强脚本系统与运行时反馈
6. 增加碰撞、平台跳跃逻辑与 Demo 场景验证
7. 补性能统计、批处理和更完整的编辑器交互

## Notes

如果你要对外描述这个项目，当前最准确的表述是：

`一个基于 C++、OpenGL 和 Dear ImGui 的轻量级 2D 引擎与可视化编辑器原型，包含渲染抽象层、Scene 视口、项目管理、资源导入、场景序列化和脚本运行时雏形。`
