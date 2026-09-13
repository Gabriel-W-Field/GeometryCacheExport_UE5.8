# Geometry Cache Alembic Exporter for Unreal Engine 5.8

一个面向 Unreal Engine 5.8 Editor 的开源插件，用于把 `Geometry Cache` 资产导出为 Alembic `.abc` 文件。

An open-source Unreal Engine 5.8 Editor plugin for exporting `Geometry Cache` assets to Alembic `.abc` files.

## 功能 / Features

- 在 Content Browser 中右键 `Geometry Cache` 资产导出。 / Export from the Content Browser context menu.
- 导出动画顶点、三角形索引、UV0、法线和材质 Face Set。 / Export animated vertices, triangle indices, UV0, normals, and material face sets.
- 按 Geometry Cache 样本索引读取网格，避免浮点时间边界造成离散错帧。 / Read streamable tracks by sample index to avoid floating-point boundary mismatches.
- 将轨道矩阵写入 Alembic `Xform`，保留平移、旋转和缩放动画。 / Preserve track translation, rotation, and scale animation in an Alembic `Xform`.
- 每个选中的资产生成一个 `.abc` 文件。 / One `.abc` file is written per selected asset.

## 安装 / Installation

1. 关闭 Unreal Editor。 / Close Unreal Editor.
2. 将预编译包中的 `GeometryCacheExport.uplugin`、`Binaries`、`Source` 等内容复制到项目的 `Plugins/GeometryCacheExport` 文件夹。 / Copy the package contents into the project's `Plugins/GeometryCacheExport` folder.
3. 重新打开项目；如 Unreal 要求重新编译模块，允许它编译。 / Reopen the project and allow a module rebuild if requested.
4. 在 Content Browser 选中 Geometry Cache，右键选择 `Export Geometry Cache to Alembic (.abc)`。 / Select a Geometry Cache and choose the export command.

预编译包位于 `Build/GeometryCacheExport_UE5.8_Binary`，只应作为项目插件安装，不要复制到 Engine Plugins。

The prebuilt package is under `Build/GeometryCacheExport_UE5.8_Binary`. Install it as a project plugin, not an Engine plugin.

从源码编译时，使用与项目相同的 UE5.8 和 Visual Studio 工具链，构建 `Development Editor`。不需要单独安装 Alembic SDK；插件使用 UE 提供的 `AlembicLib` 和 `Imath` 模块。

For source builds, use the same UE5.8 installation and Visual Studio toolchain as the project and build `Development Editor`. No separate Alembic SDK is required; the plugin uses UE's `AlembicLib` and `Imath` modules.

```powershell
& "D:\UE5.8\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin `
  -Plugin="D:\path\to\GeometryCacheExport.uplugin" `
  -Package="D:\path\to\Output" `
  -TargetPlatforms=Win64
```

路径请替换为本机路径。不同 UE 版本的模块、头文件和 ABI 可能不同，请不要把 UE5.8 二进制包当作其他版本的通用插件。

Replace the paths for your machine. UE module APIs, headers, and ABIs may differ; do not treat the UE5.8 binary package as universal.

## 使用限制 / Limitations

- 当前采样率固定为 30 FPS，并使用资产完整时长。 / Sampling is currently fixed at 30 FPS and the full asset duration.
- 仅支持编辑器导出，不支持打包游戏运行时导出。 / This is Editor-only; packaged-game runtime export is not supported.
- 拓扑随帧变化时，无法执行常量拓扑优化；这是导入器的正常保护性提示。 / Heterogeneous topology cannot use constant-topology optimization; the importer warning is expected.
- 坐标系、轴向、单位和 UV 翻转由导入端设置决定，跨 DCC 往返时请明确检查。 / Coordinate system, axes, units, and UV flipping are controlled by importer settings.

## 原理 / Technical path

```text
UGeometryCache -> UGeometryCacheTrack
               -> MeshData by exact sample index
               -> Matrix by UpdateMatrixData
               -> Alembic OXform + OPolyMesh + OFaceSet
               -> .abc
```

核心设计是把顶点动画与轨道矩阵动画分开：网格样本写入 `OPolyMesh`，矩阵写入父级 `OXform`。UE5.8 的 Alembic Geometry Cache 导入器会读取父级变换并应用到每个网格样本。

The design separates vertex animation from track transforms: mesh samples go to `OPolyMesh`, matrices go to a parent `OXform`, which UE5.8's Alembic Geometry Cache importer applies during import.

详细技术路径见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)，其他 Agent 的修改入口见 [AGENTS.md](AGENTS.md)。

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the technical path and [AGENTS.md](AGENTS.md) for the agent-oriented code map.

## 许可证与法律边界 / License and legal boundaries

本仓库中的原创插件代码以 Apache License 2.0 发布，详见 [LICENSE](LICENSE)。该许可证包含版权授权、专利条款和免责声明，但不能保证绝对免于第三方主张；请自行进行法律和依赖审查。

Original plugin code is released under Apache License 2.0; see [LICENSE](LICENSE). It includes copyright, patent, and disclaimer terms, but no license can guarantee immunity from every third-party claim; perform your own legal and dependency review.

本许可证不授予 Unreal Engine、Unreal Engine API、Alembic、Imath 或任何第三方组件的权利。Unreal Engine 不包含在本仓库中，仓库也不重新分发 UE 第三方运行库。使用者须遵守 Epic Games、Alembic、Imath 及相关依赖适用的许可和分发条款。详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

This license grants no rights to Unreal Engine, Unreal Engine APIs, Alembic, Imath, or other third-party components. Unreal Engine and UE third-party runtime libraries are not included or redistributed here. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

软件按“现状”提供，不承诺适用于特定用途或没有错误。 / The software is provided “AS IS”, without warranties.

Copyright 2026 ABCclip and contributors. / 版权所有 2026 ABCclip 及贡献者。

## 贡献与安全 / Contributing and security

欢迎提交 Issue、代码和文档改进，请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。安全问题不要公开上传专有资产或凭据，参见 [SECURITY.md](SECURITY.md)。

Issues, code, and documentation improvements are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md); do not publish proprietary assets or credentials when reporting security issues. See [SECURITY.md](SECURITY.md).
