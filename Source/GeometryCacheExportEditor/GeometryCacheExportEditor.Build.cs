// Copyright 2026 ABCclip and contributors.
// SPDX-License-Identifier: Apache-2.0
using UnrealBuildTool;
using System;
using System.IO;

public class GeometryCacheExportEditor : ModuleRules
{
    public GeometryCacheExportEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Default;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GeometryCache", "AlembicLib", "Imath"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate", "SlateCore", "UnrealEd", "AssetTools", "ContentBrowser",
            "Projects", "DesktopPlatform", "ToolMenus"
        });

        PublicDefinitions.Add("GEOMETRYCACHEEXPORT_WITH_ALEMBIC=1");
    }
}
