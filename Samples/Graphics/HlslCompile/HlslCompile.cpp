//--------------------------------------------------------------------------------------
// SimpleTriangle.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HlslCompile.h"

#include "ATGColors.h"
#include "ReadData.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    struct Vertex
    {
        XMFLOAT4 position;
        XMFLOAT4 color;
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	// Prepare the command list to render a new frame.
	m_deviceResources->Prepare();
	Clear();

	auto commandList = m_deviceResources->GetCommandList();
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

	auto size = m_deviceResources->GetOutputSize();
	RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

	XMFLOAT2 positionTitle = { (float)safeRect.left, (float)safeRect.top, };
	m_spriteBatch->Begin(commandList);
	m_spriteBatch->SetViewport(m_deviceResources->GetScreenViewport());
	m_fontDescription->DrawString(m_spriteBatch.get(), L"HlslCompile sample - To test HLSL symbols, take a PIX GPU capture and examine the pixel shaders for each triangle below", positionTitle);
	m_spriteBatch->End();

	auto width = size.right - size.left;
	auto height = size.bottom - size.top;
	auto yMargin = 0.02f * height;

	D3D12_VIEWPORT viewportExample;
	viewportExample.TopLeftX = positionTitle.x;
	viewportExample.TopLeftY = positionTitle.y + 4.0f * yMargin;
	viewportExample.Width = 
	viewportExample.Height = 0.05f * height;
	viewportExample.MinDepth = 0.0f;
	viewportExample.MaxDepth = 1.0;

	auto DrawExample = [commandList, this, &viewportExample](const wchar_t* name,
		ID3D12PipelineState* pso)-> void
	{
		static const VertexPositionColor c_vertexData[3] =
		{
			{
				XMFLOAT3(0.0f, 1.0f, 1.0f),
				SimpleMath::Vector4(Colors::Red),
			},  // Top / Red
			{
				XMFLOAT3(1.0f, -1.0f, 1.0f),
				SimpleMath::Vector4(Colors::Green),
			},  // Right / Green
			{
				XMFLOAT3(-1.0f, -1.0f, 1.0f),
				SimpleMath::Vector4(Colors::Blue),
			}   // Left / Blue
		};

		PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, name);
		commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		commandList->SetPipelineState(pso);
		commandList->RSSetViewports(1, &viewportExample);
		m_primitiveBatch->Begin(commandList);
		m_primitiveBatch->DrawTriangle(c_vertexData[0], c_vertexData[1], c_vertexData[2]);
		m_primitiveBatch->End();
		auto vp = m_deviceResources->GetScreenViewport();
		commandList->RSSetViewports(1, &vp);
		PIXEndEvent(commandList);
	};

	auto DrawDescription = [this, commandList, &viewportExample, width, yMargin](const wchar_t* description, size_t size)->void
	{
		auto xTab1 = 0.005f * width;
		XMFLOAT2 positionSize = {
			viewportExample.TopLeftX + viewportExample.Width + xTab1,
			viewportExample.TopLeftY + viewportExample.Height / 2.0f - yMargin / 2.0f,
		};
		auto xTab2 = 0.080f * width;
		XMFLOAT2 positionDescription = {
			positionSize.x + xTab2,
			positionSize.y,
		};

		wchar_t buffer[2048] = L"";
		swprintf_s(buffer, L"(%6zd bytes)", size);

		m_spriteBatch->Begin(commandList);
		m_spriteBatch->SetViewport(m_deviceResources->GetScreenViewport());
		m_fontDescription->DrawString(m_spriteBatch.get(), buffer, positionSize, Colors::Cyan);
		m_fontDescription->DrawString(m_spriteBatch.get(), description, positionDescription);
		m_spriteBatch->End();
	};

	// Draw triangle with each variant of the pixel shader.
	// See comments at the CreateGraphicsPipelineState calls later in this file
	DrawExample(L"DxcEmbeddedPdb", m_pipelineStateDxcEmbeddedPdb.Get());
	DrawDescription(L"Dxc.exe - Symbols are embedded in the shader binary and automatically found by PIX.", m_sizeDxcEmbeddedPdb);
	viewportExample.TopLeftY += viewportExample.Height + yMargin;
	DrawExample(L"DxcManualPdb", m_pipelineStateDxcManualPdb.Get());
	DrawDescription(L"Dxc.exe - Symbols are saved to a user-specified pdb file and automatically found by PIX.", m_sizeDxcManualPdb);
	viewportExample.TopLeftY += viewportExample.Height + yMargin;
	DrawExample(L"DxcAutoPdb", m_pipelineStateDxcAutoPdb.Get());
	DrawDescription(L"Dxc.exe - Symbols are saved to a pdb file with an auto-generated filename, and the user must set a pdb path in PIX.", m_sizeDxcAutoPdb);
	viewportExample.TopLeftY += viewportExample.Height + yMargin;

	DrawExample(L"DxCompilerEmbeddedPdb", m_pipelineStateDxCompilerEmbeddedPdb.Get());
#if _GAMING_XBOX_SCARLETT
    DrawDescription(L"DxCompiler_xs.dll - Symbols are embedded in the shader binary and automatically found by PIX.", m_sizeDxCompilerEmbeddedPdb);
#else
    DrawDescription(L"DxCompiler_x.dll - Symbols are embedded in the shader binary and automatically found by PIX.", m_sizeDxCompilerEmbeddedPdb);
#endif
    viewportExample.TopLeftY += viewportExample.Height + yMargin;
	DrawExample(L"DxCompilerManualPdb", m_pipelineStateDxCompilerManualPdb.Get());
#if _GAMING_XBOX_SCARLETT
    DrawDescription(L"DxCompiler_xs.dll - Symbols are saved to a user-specified pdb file and automatically found by PIX.", m_sizeDxCompilerManualPdb);
#else
    DrawDescription(L"DxCompiler_x.dll - Symbols are saved to a user-specified pdb file and automatically found by PIX.", m_sizeDxCompilerManualPdb);
#endif
	viewportExample.TopLeftY += viewportExample.Height + yMargin;
	DrawExample(L"DxCompilerAutoPdb", m_pipelineStateDxCompilerAutoPdb.Get());
#if _GAMING_XBOX_SCARLETT
    DrawDescription(L"DxCompiler_xs.dll - Symbols are saved to a pdb file with an auto-generated filename, and the user must set a pdb path in PIX.", m_sizeDxCompilerAutoPdb);
#else
    DrawDescription(L"DxCompiler_x.dll - Symbols are saved to a pdb file with an auto-generated filename, and the user must set a pdb path in PIX.", m_sizeDxCompilerAutoPdb);
#endif
	viewportExample.TopLeftY += viewportExample.Height + yMargin;

	PIXEndEvent(commandList);

	// Show the new frame.
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
	m_deviceResources->Present();
	PIXBeginEvent(m_deviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"CommitGraphicsMemory");
	m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
	PIXEndEvent(m_deviceResources->GetCommandQueue());
	PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
	auto device = m_deviceResources->GetD3DDevice();

	auto size = m_deviceResources->GetOutputSize();

	m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

	// Create root signature.
	auto vertexShaderBlob = DX::ReadData(L"VertexShader.cso");

	// Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.

	DX::ThrowIfFailed(
		device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
			IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

	// Create the pipeline state, which includes loading shaders.
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = VertexPositionColor::InputLayout;
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	psoDesc.SampleDesc.Count = 1;

	auto LoadPixelShader = [device](const wchar_t *name, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc, const IID& riid, void** pso) -> size_t
	{
		auto pixelShaderBlob = DX::ReadData(name);
		psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, riid, pso));

		return pixelShaderBlob.size();
	};

	// The standard pixel shader, compiled with dxc /Zi.
	m_sizeDxcEmbeddedPdb = LoadPixelShader(L"PixelShader_Dxc_EmbeddedPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxcEmbeddedPdb.ReleaseAndGetAddressOf()));

	// The standard pixel shader, compiled with dxc /Zi /Qstrip_debug /Fd <manual-path-name>.
	m_sizeDxcManualPdb = LoadPixelShader(L"PixelShader_Dxc_ManualPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxcManualPdb.ReleaseAndGetAddressOf()));

	// The standard pixel shader, compiled with dxc /Zi /Qstrip_debug /Fd <manual-path>\.
	// The trailing backslash tells the compiler to automatically choose a filename equal to the shader hash
	m_sizeDxcAutoPdb = LoadPixelShader(L"PixelShader_Dxc_AutoPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxcAutoPdb.ReleaseAndGetAddressOf()));


	// The pixel shader compiled with IDxcCompiler and /Zi.
	m_sizeDxCompilerEmbeddedPdb = LoadPixelShader(L"PixelShader_DxCompiler_EmbeddedPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxCompilerEmbeddedPdb.ReleaseAndGetAddressOf()));

	// The pixel shader compiled with IDxcCompiler and /Zi /Qstrip_debug, with the symbols
	// saved to a file with a name chosen manually. 
	m_sizeDxCompilerManualPdb = LoadPixelShader(L"PixelShader_DxCompiler_ManualPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxCompilerManualPdb.ReleaseAndGetAddressOf()));

	// The pixel shader compiled with IDxcCompiler and /Zi /Qstrip_debug, with the symbols
	// saved to a file named after the shader hash. 
	m_sizeDxCompilerAutoPdb = LoadPixelShader(L"PixelShader_DxCompiler_AutoPdb.cso", psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateDxCompilerAutoPdb.ReleaseAndGetAddressOf()));

	{
		m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<VertexPositionColor>>(device);

		m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

		ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
		SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
		m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

		bool uhd = (size.right - size.left) > 3000;

		m_fontDescription = std::make_unique<SpriteFont>(device,
			resourceUpload,
			uhd ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
			m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontDescription),
			m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontDescription));

		auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
		uploadResourcesFinished.wait();     // Wait for resources to upload
	}

	// Wait until assets have been uploaded to the GPU.
	m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}
#pragma endregion
