// Copyright 2026 ABCclip and contributors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoreMinimal.h"
#include "Math/Matrix.h"

struct FGeometryCacheMeshData;
struct FGeometryCacheAlembicWriterImpl;

struct FGeometryCacheAlembicExportOptions
{
    FString OutputFile;
    float StartTime = 0.0f;
    float EndTime = -1.0f;
    float SampleRate = 30.0f;
    bool bExportNormals = true;
    bool bExportUVs = true;
};

class FGeometryCacheAlembicWriter
{
public:
    FGeometryCacheAlembicWriter();
    ~FGeometryCacheAlembicWriter();

    FGeometryCacheAlembicWriter(const FGeometryCacheAlembicWriter&) = delete;
    FGeometryCacheAlembicWriter& operator=(const FGeometryCacheAlembicWriter&) = delete;

    bool Open(const FGeometryCacheAlembicExportOptions& Options, FString& OutError);
    /** Writes a mesh sample and its Geometry Cache track transform. */
    bool WriteMesh(float Time, const FGeometryCacheMeshData& MeshData, const FMatrix& TrackMatrix, FStringView ObjectName, FString& OutError);
    bool Close(FString& OutError);

private:
    TUniquePtr<FGeometryCacheAlembicWriterImpl> Impl;
};
