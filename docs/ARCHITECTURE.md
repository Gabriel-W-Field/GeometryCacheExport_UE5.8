# Architecture / 技术原理

## Scope / 范围

The plugin is an Unreal Editor module. It takes a `UGeometryCache` asset and writes one Alembic archive per asset. It does not export Actors, Sequencer bindings, materials, textures, or packaged-game runtime code.

本插件是 Unreal Editor 模块，将 `UGeometryCache` 导出为每资产一个 Alembic 文件。不导出 Actor、Sequencer 绑定、材质、纹理，也不提供打包游戏运行时接口。

## Export flow / 导出流程

1. `ToolMenus` extends the Content Browser context menu for `UGeometryCache`.
2. The user chooses an output `.abc` path.
3. The writer creates a 30 FPS Alembic `TimeSampling`.
4. Each Geometry Cache track becomes an `OXform` with a child `OPolyMesh`.
5. Each frame writes positions, triangle indices, face counts, optional UV0, normals, and material face sets.
6. The archive is closed.

1. `ToolMenus` 为 `UGeometryCache` 扩展 Content Browser 右键菜单。
2. 用户选择 `.abc` 输出路径。
3. Writer 创建 30 FPS 的 Alembic `TimeSampling`。
4. 每个 Track 对应一个 `OXform`，其下有一个 `OPolyMesh`。
5. 每帧写入顶点、三角形索引、面顶点数、可选 UV0、法线和材质 Face Set。
6. 关闭档案。

## Time sampling / 时间采样

Calling `GetMeshDataAtTime(SampleIndex / 30.0f)` can select an adjacent source sample at a floating-point boundary because UE's sample search and imported sample times are not necessarily identical. This was the source of the observed discrete-frame offset.

使用 `GetMeshDataAtTime(SampleIndex / 30.0f)` 时，UE 的样本查找边界和导入样本时间可能存在微小差异，导致少数边界值选中相邻样本，这就是曾经的离散帧错位来源。

For `UGeometryCacheTrackStreamable`, the exporter uses `GetMeshDataAtSampleIndex(SampleIndex, ...)`. `UpdateMatrixData()` is evaluated on the same logical time grid, keeping vertex and transform samples aligned.

因此 `UGeometryCacheTrackStreamable` 使用 `GetMeshDataAtSampleIndex(SampleIndex, ...)`，并在同一逻辑时间网格调用 `UpdateMatrixData()`，保持顶点和变换对齐。

## Alembic hierarchy / Alembic 层级

```text
Top
└── TrackName                 OXform
    └── Mesh                   OPolyMesh
        └── Material_*         OFaceSet
```

The mesh stays in local vertex space. Track matrices are written as `XformSample` matrix operations on the parent `OXform`. UE5.8's Alembic Geometry Cache importer traverses this hierarchy and applies the transform to each mesh sample.

网格保持在局部顶点空间，轨道矩阵作为父级 `OXform` 的 `XformSample` 写入。UE5.8 的 Alembic Geometry Cache 导入器会遍历该层级并应用变换。

## Topology and Face Sets / 拓扑与面集

Each `OPolyMesh` creates each Face Set schema once and writes membership samples over time. If vertex count, index count, or face layout changes, the data is heterogeneous-topology and constant-topology optimization is not valid.

每个 `OPolyMesh` 的 Face Set schema 只创建一次，成员关系按时间写入。如果顶点数、索引数或面布局变化，数据就是异拓扑，不能执行常量拓扑优化。

## Coordinate conversion / 坐标转换

The exporter writes UE-side numeric coordinates. Axis conversion is intentionally left to the importing application. UE's Alembic presets may contain negative scale, which consistently reverses winding and related buffers; a preset mismatch should be diagnosed separately from a sampling error.

导出器写入 UE 侧数值坐标，轴向转换交由导入端决定。UE Alembic 预设可能包含负缩放，会一致地反转绕序和相关缓冲区；预设不匹配应与采样错误分开排查。

## Extension points / 修改入口

- Add options in `GeometryCacheAlembicWriter.h` and expose them from the editor module.
- Add mesh attributes in `GeometryCacheAlembicWriter.cpp`.
- Change asset traversal and user interaction in `GeometryCacheExportEditorModule.cpp`.
- For a new UE version, verify `GeometryCacheTrackStreamable`, `GeometryCacheMeshData`, Alembic module names, and importer transform behavior first.

- 在 `GeometryCacheAlembicWriter.h` 增加选项并由 Editor 模块暴露。
- 在 `GeometryCacheAlembicWriter.cpp` 增加网格属性。
- 在 `GeometryCacheExportEditorModule.cpp` 修改资产遍历和交互。
- 适配新 UE 版本时先核对 Track、MeshData、Alembic 模块名和导入器变换行为。
