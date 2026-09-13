// Copyright 2026 ABCclip and contributors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Modules/ModuleManager.h"

class FMenuBuilder;
struct FAssetData;
class FExtender;
struct FToolMenuContext;
struct FToolMenuSection;

class FGeometryCacheExportEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void AddExportMenu(FToolMenuSection& InSection);
    void ExecuteExport(const FToolMenuContext& MenuContext);
    void ExportSelectedAssets(TArray<FAssetData> SelectedAssets);

    FDelegateHandle ToolMenusStartupHandle;
};
