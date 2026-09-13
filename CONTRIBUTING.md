# Contributing / 贡献指南

## Before a pull request / 提交 PR 前

- State the UE version, target platform, importer settings, and build configuration.
- 说明 UE 版本、目标平台、导入设置和构建配置。
- Include a minimal reproducible Geometry Cache description when possible.
- 尽可能提供最小可复现的 Geometry Cache 描述。
- Do not commit proprietary assets, Unreal Engine files, generated `Intermediate`, or third-party binaries.
- 不要提交专有资产、Unreal Engine 文件、生成的 `Intermediate` 或第三方二进制。
- Test first, middle, and last frames and at least one UE5.8 Alembic round trip.
- 测试首帧、中间帧、末帧，并至少完成一次 UE5.8 Alembic 回导验证。

## Code expectations / 代码要求

- Preserve exact sample-index reads for streamable tracks.
- 保留 Streamable Track 的精确样本索引读取。
- Keep transform and mesh sample timing aligned.
- 保持矩阵与网格样本时间对齐。
- Do not suppress topology warnings by falsely declaring heterogeneous data constant-topology.
- 不要通过虚假声明常量拓扑来消除异拓扑警告。
- Keep changes focused and document UE-version-specific API assumptions.
- 修改应保持聚焦，并记录 UE 版本相关的 API 假设。

## License / 许可证

By contributing, you agree that your contribution is provided under the Apache License 2.0 in `LICENSE`.

提交贡献即表示你同意该贡献按 `LICENSE` 中的 Apache License 2.0 发布。
