# Agent Guide / Agent 快速指南

This file is a compact map for coding agents and maintainers. It describes the intended technical path and safe repository boundaries.

本文件为其他 coding agent 和维护者提供快速入口，说明技术路径以及仓库边界。

## Read first / 先读

1. `README.md` — scope, installation, license, and limitations.
2. `docs/ARCHITECTURE.md` — data flow, sampling, transforms, and importer compatibility.
3. `Source/GeometryCacheExportEditor/Private/GeometryCacheExportEditorModule.cpp` — editor menu and export loop.
4. `Source/GeometryCacheExportEditor/Private/GeometryCacheAlembicWriter.cpp` — Alembic hierarchy and samples.
5. `Source/GeometryCacheExportEditor/Public/GeometryCacheAlembicWriter.h` — writer API.

## Source map / 源码地图

| Path | Responsibility |
|---|---|
| `GeometryCacheExport.uplugin` | Plugin metadata and module dependencies. |
| `Source/.../GeometryCacheExportEditorModule.cpp` | Content Browser action and asset sampling. |
| `Source/.../GeometryCacheAlembicWriter.cpp` | `OXform`, `OPolyMesh`, `OFaceSet`, and time sampling. |
| `Source/.../GeometryCacheAlembicWriter.h` | Writer API and export options. |
| `Build/GeometryCacheExport_UE5.8_Binary` | Generated release package; do not edit by hand. |

## Invariants / 必须保持

- When an exact sample index is available, use `GetMeshDataAtSampleIndex()`, not floating-point time lookup.
- 有精确样本索引时必须使用 `GetMeshDataAtSampleIndex()`，不要使用浮点时间查找。
- Keep mesh samples and transform samples on the same frame/time grid.
- 网格样本和矩阵样本必须使用同一帧/时间网格。
- Create each Alembic Face Set once per `OPolyMesh`; write time-aligned membership samples.
- 每个 `OPolyMesh` 的 Face Set 只能创建一次，之后按时间写成员样本。
- Do not falsely force constant topology; importer warnings are meaningful for heterogeneous data.
- 不要虚假声明常量拓扑；异拓扑数据的导入器提示是有效保护。
- Never commit Unreal Engine files, proprietary assets, generated `Intermediate`, or third-party binaries.
- 禁止提交 Unreal Engine 文件、专有资产、生成的 `Intermediate` 或第三方二进制。
- Do not install or overwrite a user's project plugin during development unless explicitly requested.
- 开发时不要自动安装或覆盖用户项目插件，除非用户明确要求。

## Build and tests / 构建与测试

The supported release target is UE5.8 Win64 Editor. Use the `RunUAT.bat BuildPlugin` flow in `README.md`; after C++ changes, verify the generated DLL timestamp and build log so a stale DLL is not mistaken for a successful rebuild.

当前发布目标是 UE5.8 Win64 Editor。使用 README 中的 `RunUAT.bat BuildPlugin` 流程；修改 C++ 后检查 DLL 时间戳和构建日志，避免误用旧 DLL。

For sampling changes test first, middle, last, an 89-frame cache, and heterogeneous topology. For transform changes test identity, translation, rotation, uniform scale, non-uniform scale, and negative determinant transforms in both Blender and UE5.8 import.

采样修改需测试首帧、中间帧、末帧、89 帧缓存和异拓扑缓存；变换修改需测试单位矩阵、平移、旋转、均匀缩放、非均匀缩放及负行列式变换，并在 Blender 和 UE5.8 回导中验证。
