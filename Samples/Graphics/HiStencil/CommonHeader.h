//--------------------------------------------------------------------------------------
// CommonHeader.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

namespace CommonHeader
{
	enum class PbrTextureEntries
	{
		AlbedoTexture,
		NormalTexture,
		RMATexture,
		NumMeshTextures,
		RadianceTexture = NumMeshTextures,
		IrrandianceTexture,
		TotalTextures
	};

	enum class DescriptorHeapEntry
	{
		TextFont,
		ControllerFont,
		DepthBuffer,
		StencilBuffer,
		HTileBuffer,
		DepthBufferEsram,
		StencilBufferEsram,
		HTileBufferEsram,
		IntermediateTarget,
		IntermediateTargetEsram,
		MeshTextureExterior,
		MeshTextureRadiance = MeshTextureExterior + static_cast<uint32_t>(PbrTextureEntries::NumMeshTextures),
		MeshTextureIrradiance,
		MeshTextureInteriorFurnishings,
		MeshTextureInteriorGears = MeshTextureInteriorFurnishings + static_cast<uint32_t>(PbrTextureEntries::NumMeshTextures),
		MeshTextureInteriorRobot = MeshTextureInteriorGears + static_cast<uint32_t>(PbrTextureEntries::NumMeshTextures),
		TotalDescriptorHeapEntryCount = 32
	};

	enum class UAV_HEAP
	{
		MayPassAppendBuffer,
		MayPassCountBuffer,
		MayFailAppendBuffer,
		MayFailCountBuffer,
		SMemSingleValueAppendBuffer,
		SMemSingleValuesCountBuffer,
		RenderTarget_UAV,
		RenderTargetEsram_UAV,
		TotalNum
	};

	enum class SamplerIndex
	{
		StaticSampler0,
	};

	enum class RootParameterIndex
	{
		DescriptorTableSRV,
		DescriptorTableUAV,
		ConstantBuffer,
		SRV1,
		RootParameterCount
	};

	constexpr uint32_t RootParameterSRV1Slot = static_cast<uint32_t>(DescriptorHeapEntry::TotalDescriptorHeapEntryCount);
};
