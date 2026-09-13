// Copyright 2026 ABCclip and contributors.
// SPDX-License-Identifier: Apache-2.0
#include "GeometryCacheExportEditorModule.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserMenuContexts.h"
#include "DesktopPlatformModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GeometryCache.h"
#include "GeometryCacheTrack.h"
#include "GeometryCacheTrackStreamable.h"
#include "GeometryCacheMeshData.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "GeometryCacheAlembicWriter.h"
#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "GeometryCacheExportEditor"

void FGeometryCacheExportEditorModule::StartupModule()
{
    ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGeometryCacheExportEditorModule::RegisterMenus));
}

void FGeometryCacheExportEditorModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
        UToolMenus::UnregisterOwner(this);
    }
}

void FGeometryCacheExportEditorModule::RegisterMenus()
{
    if (!UToolMenus::IsToolMenuUIEnabled())
    {
        return;
    }

    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UGeometryCache::StaticClass());
    if (!Menu)
    {
        return;
    }

    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
    Section.AddDynamicEntry("GeometryCacheExport", FNewToolMenuSectionDelegate::CreateRaw(
        this, &FGeometryCacheExportEditorModule::AddExportMenu));
}

void FGeometryCacheExportEditorModule::AddExportMenu(FToolMenuSection& InSection)
{
    const UContentBrowserAssetContextMenuContext* Context =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(InSection);
    if (!Context)
    {
        return;
    }

    const bool bAllGeometryCaches = !Context->SelectedAssets.ContainsByPredicate(
        [](const FAssetData& AssetData)
        {
            return !AssetData.IsInstanceOf(UGeometryCache::StaticClass());
        });
    if (!bAllGeometryCaches)
    {
        return;
    }

    FToolUIAction UIAction;
    UIAction.ExecuteAction = FToolMenuExecuteAction::CreateRaw(
        this, &FGeometryCacheExportEditorModule::ExecuteExport);

    InSection.AddMenuEntry(
        "GeometryCacheExportToAlembic",
        LOCTEXT("ExportGeometryCache", "Export Geometry Cache to Alembic (.abc)"),
        LOCTEXT("ExportGeometryCacheTooltip", "Samples the selected Geometry Cache assets and writes Alembic files."),
        FSlateIcon(),
        UIAction);
}

void FGeometryCacheExportEditorModule::ExecuteExport(const FToolMenuContext& MenuContext)
{
    const UContentBrowserAssetContextMenuContext* Context =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext);
    if (!Context)
    {
        return;
    }

    TArray<FAssetData> SelectedAssets;
    for (const FAssetData& AssetData : Context->SelectedAssets)
    {
        if (AssetData.IsInstanceOf(UGeometryCache::StaticClass()))
        {
            SelectedAssets.Add(AssetData);
        }
    }

    if (!SelectedAssets.IsEmpty())
    {
        ExportSelectedAssets(MoveTemp(SelectedAssets));
    }
}

void FGeometryCacheExportEditorModule::ExportSelectedAssets(TArray<FAssetData> SelectedAssets)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        return;
    }

    for (const FAssetData& AssetData : SelectedAssets)
    {
        UGeometryCache* Cache = Cast<UGeometryCache>(AssetData.GetAsset());
        if (!Cache)
        {
            continue;
        }

        FString DefaultFile = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Exports"), Cache->GetName() + TEXT(".abc"));
        FString Folder = FPaths::GetPath(DefaultFile);
        IFileManager::Get().MakeDirectory(*Folder, true);
        FString ChosenFile;
        TArray<FString> Files;
        if (!DesktopPlatform->SaveFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            LOCTEXT("SaveAlembic", "Export Geometry Cache").ToString(), Folder, FPaths::GetCleanFilename(DefaultFile),
            TEXT("Alembic files (*.abc)|*.abc"), EFileDialogFlags::None, Files) || Files.Num() == 0)
        {
            continue;
        }
        ChosenFile = Files[0];

        FGeometryCacheAlembicExportOptions Options;
        Options.OutputFile = ChosenFile;
        Options.EndTime = Cache->CalculateDuration();

        FGeometryCacheAlembicWriter Writer;
        FString Error;
        if (!Writer.Open(Options, Error))
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error));
            continue;
        }

        const int32 SampleCount = FMath::Max(1, FMath::CeilToInt((Options.EndTime - Options.StartTime) * Options.SampleRate) + 1);
        FScopedSlowTask SlowTask(SampleCount, LOCTEXT("ExportingGeometryCache", "Exporting Geometry Cache to Alembic..."));
        bool bSuccess = true;

        for (UGeometryCacheTrack* Track : Cache->Tracks)
        {
            if (!Track)
            {
                continue;
            }
            UGeometryCacheTrackStreamable* StreamableTrack = Cast<UGeometryCacheTrackStreamable>(Track);
            const bool bHasMatrixAnimation = Track->GetMaxSampleTime() > 0.0f;
            int32 MatrixSampleIndex = INDEX_NONE;
            FMatrix TrackMatrix = FMatrix::Identity;

            for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
            {
                if (SlowTask.ShouldCancel())
                {
                    bSuccess = false;
                    Error = TEXT("Export cancelled.");
                    break;
                }
                const float Time = FMath::Min(Options.StartTime + SampleIndex / Options.SampleRate, Options.EndTime);
                FGeometryCacheMeshData MeshData;
                const bool bReadMesh = StreamableTrack
                    ? StreamableTrack->GetMeshDataAtSampleIndex(SampleIndex, MeshData)
                    : Track->GetMeshDataAtTime(Time, MeshData);
                if (!bReadMesh)
                {
                    bSuccess = false;
                    Error = FString::Printf(TEXT("Could not sample Geometry Cache track at %.3f seconds."), Time);
                    break;
                }
                if (bHasMatrixAnimation)
                {
                    Track->UpdateMatrixData(Time, false, MatrixSampleIndex, TrackMatrix);
                }
                if (!Writer.WriteMesh(Time, MeshData, TrackMatrix, Track->GetName(), Error))
                {
                    bSuccess = false;
                    break;
                }
                SlowTask.EnterProgressFrame(1.0f);
            }
            if (!bSuccess)
            {
                break;
            }
        }
        bSuccess = Writer.Close(Error) && bSuccess;
        FMessageDialog::Open(EAppMsgType::Ok, bSuccess
            ? FText::Format(LOCTEXT("ExportSucceeded", "Exported {0}"), FText::FromString(ChosenFile))
            : FText::FromString(Error));
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGeometryCacheExportEditorModule, GeometryCacheExportEditor)
