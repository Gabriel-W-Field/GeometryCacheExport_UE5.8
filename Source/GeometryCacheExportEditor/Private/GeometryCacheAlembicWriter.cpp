// Copyright 2026 ABCclip and contributors.
// SPDX-License-Identifier: Apache-2.0
#include "GeometryCacheAlembicWriter.h"

#include "GeometryCacheMeshData.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

THIRD_PARTY_INCLUDES_START
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
THIRD_PARTY_INCLUDES_END

#include <cstdint>
#include <exception>
#include <map>
#include <string>
#include <vector>

namespace
{
    static std::string ToUtf8(const FString& Value)
    {
        return std::string(TCHAR_TO_UTF8(*Value));
    }

    static std::string SanitizeObjectName(FStringView Name)
    {
        FString Safe(Name);
        for (TCHAR& Character : Safe)
        {
            if (Character == TEXT('/') || Character == TEXT('\\') || Character == TEXT(':'))
            {
                Character = TEXT('_');
            }
        }
        return ToUtf8(Safe.IsEmpty() ? FString(TEXT("GeometryCacheTrack")) : Safe);
    }

    static Alembic::Abc::M44d ToAlembicMatrix(const FMatrix& Matrix)
    {
        Alembic::Abc::M44d Result;
        for (int32 Row = 0; Row < 4; ++Row)
        {
            for (int32 Column = 0; Column < 4; ++Column)
            {
                Result[Row][Column] = static_cast<double>(Matrix.M[Row][Column]);
            }
        }
        return Result;
    }
}

struct FGeometryCacheAlembicTrackWriter
{
    Alembic::AbcGeom::OXform Xform;
    Alembic::AbcGeom::OPolyMesh Mesh;
    std::map<std::string, Alembic::AbcGeom::OFaceSet> FaceSets;
};

struct FGeometryCacheAlembicWriterImpl
{
    Alembic::Abc::OArchive Archive;
    Alembic::Abc::OObject TopObject;
    Alembic::Abc::TimeSamplingPtr TimeSampling;
    TMap<FString, TUniquePtr<FGeometryCacheAlembicTrackWriter>> Tracks;
    bool bOpen = false;
};

FGeometryCacheAlembicWriter::FGeometryCacheAlembicWriter()
    : Impl(MakeUnique<FGeometryCacheAlembicWriterImpl>())
{
}

FGeometryCacheAlembicWriter::~FGeometryCacheAlembicWriter() = default;

bool FGeometryCacheAlembicWriter::Open(const FGeometryCacheAlembicExportOptions& InOptions, FString& OutError)
{
    Impl->Tracks.Reset();
    Impl->bOpen = false;
    if (InOptions.OutputFile.IsEmpty() || InOptions.SampleRate <= 0.0f)
    {
        OutError = TEXT("Alembic output path or sample rate is invalid.");
        return false;
    }

    const FString ParentDirectory = FPaths::GetPath(InOptions.OutputFile);
    IFileManager::Get().MakeDirectory(*ParentDirectory, true);

    try
    {
        Impl->TimeSampling = Alembic::Abc::TimeSamplingPtr(
            new Alembic::Abc::TimeSampling(1.0 / static_cast<double>(InOptions.SampleRate), 0.0));
        Impl->Archive = Alembic::Abc::OArchive(
            Alembic::AbcCoreOgawa::WriteArchive(), ToUtf8(InOptions.OutputFile));
        Impl->TopObject = Impl->Archive.getTop();
        Impl->bOpen = Impl->Archive.valid();
    }
    catch (const std::exception& Exception)
    {
        OutError = UTF8_TO_TCHAR(Exception.what());
        return false;
    }
    if (!Impl->bOpen)
    {
        OutError = TEXT("Could not create the Alembic archive.");
    }
    return Impl->bOpen;
}

bool FGeometryCacheAlembicWriter::WriteMesh(float Time, const FGeometryCacheMeshData& MeshData, const FMatrix& TrackMatrix, FStringView ObjectName, FString& OutError)
{
    if (!Impl->bOpen)
    {
        OutError = TEXT("Alembic writer is not open.");
        return false;
    }
    if (MeshData.Positions.IsEmpty() || MeshData.Indices.Num() == 0 || MeshData.Indices.Num() % 3 != 0)
    {
        OutError = FString::Printf(TEXT("Geometry Cache track '%s' has invalid mesh data at %.3f seconds."), *FString(ObjectName), Time);
        return false;
    }

    FString Key(ObjectName);
    FGeometryCacheAlembicTrackWriter* TrackWriter = nullptr;
    if (TUniquePtr<FGeometryCacheAlembicTrackWriter>* Existing = Impl->Tracks.Find(Key))
    {
        TrackWriter = Existing->Get();
    }
    else
    {
        TUniquePtr<FGeometryCacheAlembicTrackWriter> NewTrack = MakeUnique<FGeometryCacheAlembicTrackWriter>();
        const std::string TrackName = SanitizeObjectName(ObjectName);
        NewTrack->Xform = Alembic::AbcGeom::OXform(Impl->TopObject, TrackName, Alembic::Abc::Argument(Impl->TimeSampling));
        NewTrack->Mesh = Alembic::AbcGeom::OPolyMesh(NewTrack->Xform, "Mesh", Alembic::Abc::Argument(Impl->TimeSampling));
        TrackWriter = NewTrack.Get();
        Impl->Tracks.Add(Key, MoveTemp(NewTrack));
    }

    std::vector<Alembic::Abc::V3f> Positions;
    Positions.reserve(MeshData.Positions.Num());
    for (const FVector3f& Position : MeshData.Positions)
    {
        Positions.emplace_back(Position.X, Position.Y, Position.Z);
    }
    std::vector<int32_t> Indices;
    Indices.reserve(MeshData.Indices.Num());
    for (const uint32 Index : MeshData.Indices)
    {
        if (Index >= static_cast<uint32>(MeshData.Positions.Num()))
        {
            OutError = FString::Printf(TEXT("Geometry Cache track '%s' has an out-of-range index at %.3f seconds."), *FString(ObjectName), Time);
            return false;
        }
        Indices.push_back(static_cast<int32_t>(Index));
    }
    std::vector<int32_t> FaceCounts(MeshData.Indices.Num() / 3, 3);

    Alembic::AbcGeom::OV2fGeomParam::Sample UVSample;
    if (MeshData.VertexInfo.bHasUV0 && MeshData.TextureCoordinates.Num() == MeshData.Positions.Num())
    {
        std::vector<Alembic::Abc::V2f> UVs;
        UVs.reserve(MeshData.TextureCoordinates.Num());
        for (const FVector2f& UV : MeshData.TextureCoordinates)
        {
            UVs.emplace_back(UV.X, UV.Y);
        }
        UVSample = Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(UVs), Alembic::AbcGeom::kVertexScope);
    }

    Alembic::AbcGeom::ON3fGeomParam::Sample NormalSample;
    if (MeshData.VertexInfo.bHasTangentZ && MeshData.TangentsZ.Num() == MeshData.Positions.Num())
    {
        std::vector<Alembic::Abc::N3f> Normals;
        Normals.reserve(MeshData.TangentsZ.Num());
        for (const FPackedNormal& Normal : MeshData.TangentsZ)
        {
            const FVector3f Value = Normal.ToFVector3f();
            Normals.emplace_back(Value.X, Value.Y, Value.Z);
        }
        NormalSample = Alembic::AbcGeom::ON3fGeomParam::Sample(Alembic::Abc::N3fArraySample(Normals), Alembic::AbcGeom::kVertexScope);
    }

    try
    {
        Alembic::AbcGeom::XformSample XformSample;
        XformSample.setMatrix(ToAlembicMatrix(TrackMatrix));
        TrackWriter->Xform.getSchema().set(XformSample);

        Alembic::AbcGeom::OPolyMeshSchema::Sample Sample(
            Alembic::Abc::P3fArraySample(Positions),
            Alembic::Abc::Int32ArraySample(Indices),
            Alembic::Abc::Int32ArraySample(FaceCounts),
            UVSample,
            NormalSample);
        TrackWriter->Mesh.getSchema().set(Sample);

        std::map<std::string, std::vector<int32_t>> CurrentFaceSets;
        for (const FGeometryCacheMeshBatchInfo& Batch : MeshData.BatchesInfo)
        {
            const uint32 FirstFace = Batch.StartIndex / 3;
            if (Batch.StartIndex + Batch.NumTriangles * 3 > static_cast<uint32>(MeshData.Indices.Num()))
            {
                continue;
            }
            std::vector<int32_t> Faces;
            Faces.reserve(Batch.NumTriangles);
            for (uint32 FaceIndex = 0; FaceIndex < Batch.NumTriangles; ++FaceIndex)
            {
                Faces.push_back(static_cast<int32_t>(FirstFace + FaceIndex));
            }
            const std::string FaceSetName = "Material_" + std::to_string(Batch.MaterialIndex);
            std::vector<int32_t>& FaceSetFaces = CurrentFaceSets[FaceSetName];
            FaceSetFaces.insert(FaceSetFaces.end(), Faces.begin(), Faces.end());
        }

        // A FaceSet is created once per PolyMesh. Its membership is sampled
        // per frame, just like the mesh geometry. Write every known FaceSet
        // on every frame so all FaceSet schemas stay time-aligned.
        for (const auto& CurrentFaceSet : CurrentFaceSets)
        {
            const std::string& FaceSetName = CurrentFaceSet.first;
            auto FaceSetIt = TrackWriter->FaceSets.find(FaceSetName);
            if (FaceSetIt == TrackWriter->FaceSets.end())
            {
                Alembic::AbcGeom::OFaceSet FaceSet = TrackWriter->Mesh.getSchema().createFaceSet(FaceSetName);
                FaceSetIt = TrackWriter->FaceSets.emplace(FaceSetName, FaceSet).first;
            }
        }
        for (auto& FaceSetEntry : TrackWriter->FaceSets)
        {
            const auto CurrentFaceSetIt = CurrentFaceSets.find(FaceSetEntry.first);
            const std::vector<int32_t> EmptyFaces;
            const std::vector<int32_t>& Faces = CurrentFaceSetIt != CurrentFaceSets.end()
                ? CurrentFaceSetIt->second
                : EmptyFaces;
            FaceSetEntry.second.getSchema().set(Alembic::AbcGeom::OFaceSetSchema::Sample(
                Alembic::Abc::Int32ArraySample(Faces)));
        }
    }
    catch (const std::exception& Exception)
    {
        OutError = UTF8_TO_TCHAR(Exception.what());
        return false;
    }
    return true;
}

bool FGeometryCacheAlembicWriter::Close(FString& OutError)
{
    if (!Impl->bOpen)
    {
        return false;
    }
    Impl->Tracks.Reset();
    Impl->TopObject.reset();
    Impl->Archive.reset();
    Impl->TimeSampling.reset();
    Impl->bOpen = false;
    return true;
}
