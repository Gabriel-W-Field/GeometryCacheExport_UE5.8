# Security Policy / 安全政策

The actively maintained target is the UE5.8 Win64 Editor build. Other engine versions are not guaranteed to be compatible.

当前维护目标是 UE5.8 Win64 Editor，其他 UE 版本不保证兼容。

Do not include proprietary `.uasset`, `.abc`, project files, credentials, or personal data in a public issue. If a problem could expose files, execute unintended commands, or compromise a build environment, use GitHub Security Advisories or a private maintainer contact instead of publishing details immediately.

公开 Issue 中不要上传专有 `.uasset`、`.abc`、项目文件、凭据或个人数据。如果问题可能导致文件泄露、非预期命令执行或构建环境被入侵，请使用 GitHub Security Advisories 或维护者私下联系方式，不要立即公开细节。

The plugin writes only to the user-selected Alembic output path during normal export. It does not intentionally upload assets or telemetry.

本插件正常导出时只写入用户选择的 Alembic 输出路径，不会主动上传资产或遥测数据。
