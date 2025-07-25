// dear imgui: Renderer Backend for DirectX12
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Renderer: Expose selected render state for draw callbacks to use. Access in '(ImGui_ImplXXXX_RenderState*)GetPlatformIO().Renderer_RenderState'.

// The aim of imgui_impl_dx12.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-06-19: Fixed build on MinGW. (#8702, #4594)
//  2025-06-11: DirectX12: Added support for ImGuiBackendFlags_RendererHasTextures, for dynamic font atlas.
//  2025-05-07: DirectX12: Honor draw_data->FramebufferScale to allow for custom backends and experiment using it (consistently with other renderer backends, even though in normal condition it is not set under Windows).
//  2025-02-24: DirectX12: Fixed an issue where ImGui_ImplDX12_Init() signature change from 2024-11-15 combined with change from 2025-01-15 made legacy ImGui_ImplDX12_Init() crash. (#8429)
//  2025-01-15: DirectX12: Texture upload use the command queue provided in ImGui_ImplDX12_InitInfo instead of creating its own.
//  2024-12-09: DirectX12: Let user specifies the DepthStencilView format by setting ImGui_ImplDX12_InitInfo::DSVFormat.
//  2024-11-15: DirectX12: *BREAKING CHANGE* Changed ImGui_ImplDX12_Init() signature to take a ImGui_ImplDX12_InitInfo struct. Legacy ImGui_ImplDX12_Init() signature is still supported (will obsolete).
//  2024-11-15: DirectX12: *BREAKING CHANGE* User is now required to pass function pointers to allocate/free SRV Descriptors. We provide convenience legacy fields to pass a single descriptor, matching the old API, but upcoming features will want multiple.
//  2024-10-23: DirectX12: Unmap() call specify written range. The range is informational and may be used by debug tools.
//  2024-10-07: DirectX12: Changed default texture sampler to Clamp instead of Repeat/Wrap.
//  2024-10-07: DirectX12: Expose selected render state in ImGui_ImplDX12_RenderState, which you can access in 'void* platform_io.Renderer_RenderState' during draw callbacks.
//  2024-10-07: DirectX12: Compiling with '#define ImTextureID=ImU64' is unnecessary now that dear imgui defaults ImTextureID to u64 instead of void*.
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-05-19: DirectX12: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-02-18: DirectX12: Change blending equation to preserve alpha in output buffer.
//  2021-01-11: DirectX12: Improve Windows 7 compatibility (for D3D12On7) by loading d3d12.dll dynamically.
//  2020-09-16: DirectX12: Avoid rendering calls with zero-sized scissor rectangle since it generates a validation layer warning.
//  2020-09-08: DirectX12: Clarified support for building on 32-bit systems by redefining ImTextureID.
//  2019-10-18: DirectX12: *BREAKING CHANGE* Added extra ID3D12DescriptorHeap parameter to ImGui_ImplDX12_Init() function.
//  2019-05-29: DirectX12: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: DirectX12: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2019-03-29: Misc: Various minor tidying up.
//  2018-12-03: Misc: Added #pragma comment statement to automatically link with d3dcompiler.lib when using D3DCompile().
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-06-12: DirectX12: Moved the ID3D12GraphicsCommandList* parameter from NewFrame() to RenderDrawData().
//  2018-06-08: Misc: Extracted imgui_impl_dx12.cpp/.h away from the old combined DX12+Win32 example.
//  2018-06-08: DirectX12: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle (to ease support for future multi-viewport).
//  2018-02-22: Merged into master with all Win32 code synchronized to other examples.

#include "pch.h"

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_dx12.h"

// DirectX
#ifdef _GAMING_XBOX_SCARLETT
#define TEXTURE_PITCH D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#include "shaders/imgui_impl_dx12_xs_vs.h"
#include "shaders/imgui_impl_dx12_xs_ps.h"
#include "shaders/imgui_impl_dx12_xs_rs.h"
#elif defined(_GAMING_XBOX)
#define TEXTURE_PITCH D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT
#include <d3d12_x.h>
#include <d3dx12_x.h>
#include "shaders/imgui_impl_dx12_x_vs.h"
#include "shaders/imgui_impl_dx12_x_ps.h"
#include "shaders/imgui_impl_dx12_x_rs.h"
#else
#define TEXTURE_PITCH D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
#ifdef _GAMING_DESKTOP
#include "d3dx12.h"
#endif
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi1_4.h>
#include "shaders/imgui_impl_dx12_vs.h"
#include "shaders/imgui_impl_dx12_ps.h"
#include "shaders/imgui_impl_dx12_rs.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning: implicit conversion changes signedness
#endif

// MinGW workaround, see #4594
typedef decltype(D3D12SerializeRootSignature) *_PFN_D3D12_SERIALIZE_ROOT_SIGNATURE;

// DirectX12 data
struct ImGui_ImplDX12_RenderBuffers;

struct ImGui_ImplDX12_Texture
{
    ID3D12Resource*             pTextureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE hFontSrvCpuDescHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE hFontSrvGpuDescHandle;

    ImGui_ImplDX12_Texture()    { memset((void*)this, 0, sizeof(*this)); }
};

struct ImGui_ImplDX12_Data
{
    ImGui_ImplDX12_InitInfo     InitInfo;
    ID3D12Device*               pd3dDevice;
    ID3D12RootSignature*        pRootSignature;
    ID3D12PipelineState*        pPipelineState;
    ID3D12CommandQueue*         pCommandQueue;
    bool                        commandQueueOwned;
    DXGI_FORMAT                 RTVFormat;
    DXGI_FORMAT                 DSVFormat;
    ID3D12DescriptorHeap*       pd3dSrvDescHeap;
    UINT                        numFramesInFlight;

    ImGui_ImplDX12_RenderBuffers* pFrameResources;
    UINT                        frameIndex;

    ImGui_ImplDX12_Texture      FontTexture;
    bool                        LegacySingleDescriptorUsed;

    ImGui_ImplDX12_Data()       { memset((void*)this, 0, sizeof(*this)); frameIndex = UINT_MAX; }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX12_Data* ImGui_ImplDX12_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX12_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Buffers used during the rendering of a frame
struct ImGui_ImplDX12_RenderBuffers
{
    ID3D12Resource*     IndexBuffer;
    ID3D12Resource*     VertexBuffer;
    int                 IndexBufferSize;
    int                 VertexBufferSize;
};

struct VERTEX_CONSTANT_BUFFER_DX12
{
    float   mvp[4][4];
};

// Functions
static void ImGui_ImplDX12_SetupRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* command_list, ImGui_ImplDX12_RenderBuffers* fr)
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    VERTEX_CONSTANT_BUFFER_DX12 vertex_constant_buffer;
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
    }

    // Setup viewport
    D3D12_VIEWPORT vp = {};
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0.0f;
    command_list->RSSetViewports(1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress() + offset;
    vbv.SizeInBytes = fr->VertexBufferSize * stride;
    vbv.StrideInBytes = stride;
    command_list->IASetVertexBuffers(0, 1, &vbv);
    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
    ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
    ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    command_list->IASetIndexBuffer(&ibv);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->SetPipelineState(bd->pPipelineState);
    command_list->SetGraphicsRootSignature(bd->pRootSignature);
    command_list->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

    // Setup blend factor
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    command_list->OMSetBlendFactor(blend_factor);
}

template<typename T>
static inline void SafeRelease(T*& res)
{
    if (res)
        res->Release();
    res = nullptr;
}

// Render function
void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* command_list)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplDX12_UpdateTexture(tex);

    // FIXME: We are assuming that this only gets called once per frame!
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    bd->frameIndex = bd->frameIndex + 1;
    ImGui_ImplDX12_RenderBuffers* fr = &bd->pFrameResources[bd->frameIndex % bd->numFramesInFlight];

    // Create and grow vertex/index buffers if needed
    if (fr->VertexBuffer == nullptr || fr->VertexBufferSize < draw_data->TotalVtxCount)
    {
        SafeRelease(fr->VertexBuffer);
        fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->VertexBufferSize * sizeof(ImDrawVert);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(&fr->VertexBuffer)) < 0)
            return;
    }
    if (fr->IndexBuffer == nullptr || fr->IndexBufferSize < draw_data->TotalIdxCount)
    {
        SafeRelease(fr->IndexBuffer);
        fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->IndexBufferSize * sizeof(ImDrawIdx);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(&fr->IndexBuffer)) < 0)
            return;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    // During Map() we specify a null read range (as per DX12 API, this is informational and for tooling only)
    void* vtx_resource, *idx_resource;
    D3D12_RANGE range = { 0, 0 };
    if (fr->VertexBuffer->Map(0, &range, &vtx_resource) != S_OK)
        return;
    if (fr->IndexBuffer->Map(0, &range, &idx_resource) != S_OK)
        return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += draw_list->VtxBuffer.Size;
        idx_dst += draw_list->IdxBuffer.Size;
    }

    // During Unmap() we specify the written range (as per DX12 API, this is informational and for tooling only)
    range.End = (SIZE_T)((intptr_t)vtx_dst - (intptr_t)vtx_resource);
    IM_ASSERT(range.End == draw_data->TotalVtxCount * sizeof(ImDrawVert));
    fr->VertexBuffer->Unmap(0, &range);
    range.End = (SIZE_T)((intptr_t)idx_dst - (intptr_t)idx_resource);
    IM_ASSERT(range.End == draw_data->TotalIdxCount * sizeof(ImDrawIdx));
    fr->IndexBuffer->Unmap(0, &range);

    // Setup desired DX state
    ImGui_ImplDX12_SetupRenderState(draw_data, command_list, fr);

    // Setup render state structure (for callbacks and custom texture bindings)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplDX12_RenderState render_state;
    render_state.Device = bd->pd3dDevice;
    render_state.CommandList = command_list;
    platform_io.Renderer_RenderState = &render_state;

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX12_SetupRenderState(draw_data, command_list, fr);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // TODO: we get clip_min < 0 in various cases, this blows up on Xbox, just reset them to 0 for now
                if (clip_min.x < 0) clip_min.x = 0;
                if (clip_min.y < 0) clip_min.y = 0;

                // Apply scissor/clipping rectangle
                const D3D12_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                command_list->RSSetScissorRects(1, &r);

                // Bind texture, Draw
                D3D12_GPU_DESCRIPTOR_HANDLE texture_handle = {};
                texture_handle.ptr = (UINT64)pcmd->GetTexID();
                command_list->SetGraphicsRootDescriptorTable(1, texture_handle);
                command_list->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, static_cast<INT>(pcmd->VtxOffset + global_vtx_offset), 0);
            }
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }
    platform_io.Renderer_RenderState = nullptr;
}

static void ImGui_ImplDX12_DestroyTexture(ImTextureData* tex)
{
    ImGui_ImplDX12_Texture* backend_tex = (ImGui_ImplDX12_Texture*)tex->BackendUserData;
    if (backend_tex == nullptr)
        return;
    IM_ASSERT(backend_tex->hFontSrvGpuDescHandle.ptr == (UINT64)tex->TexID);
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    bd->InitInfo.SrvDescriptorFreeFn(&bd->InitInfo, backend_tex->hFontSrvCpuDescHandle, backend_tex->hFontSrvGpuDescHandle);
    SafeRelease(backend_tex->pTextureResource);
    backend_tex->hFontSrvCpuDescHandle.ptr = 0;
    backend_tex->hFontSrvGpuDescHandle.ptr = 0;
    IM_DELETE(backend_tex);

    // Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
    tex->SetTexID(ImTextureID_Invalid);
    tex->SetStatus(ImTextureStatus_Destroyed);
    tex->BackendUserData = nullptr;
}

void ImGui_ImplDX12_UpdateTexture(ImTextureData* tex)
{
    // Build texture atlas
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    bool need_barrier_before_copy = true; // Do we need a resource barrier before we copy new data in?

    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics systemAdd commentMore actions
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        ImGui_ImplDX12_Texture* backend_tex = IM_NEW(ImGui_ImplDX12_Texture)();
        bd->InitInfo.SrvDescriptorAllocFn(&bd->InitInfo, &backend_tex->hFontSrvCpuDescHandle, &backend_tex->hFontSrvGpuDescHandle); // Allocate a desctriptor handle

        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = static_cast<UINT64>(tex->Width);
        desc.Height = static_cast<UINT64>(tex->Height);
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ID3D12Resource* pTexture = nullptr;
        bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(&pTexture));

       // Create SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        bd->pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, backend_tex->hFontSrvCpuDescHandle);
        SafeRelease(backend_tex->pTextureResource);
        backend_tex->pTextureResource = pTexture;

        // Store identifiers
        tex->SetTexID((ImTextureID)backend_tex->hFontSrvGpuDescHandle.ptr);
        tex->BackendUserData = backend_tex;
        need_barrier_before_copy = false; // Because this is a newly-created texture it will be in D3D12_RESOURCE_STATE_COMMON and thus we don't need a barrier
        // We don't set tex->Status to ImTextureStatus_OK to let the code fallthrough below.
    }

    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
    {
        ImGui_ImplDX12_Texture* backend_tex = (ImGui_ImplDX12_Texture*)tex->BackendUserData;
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // We could use the smaller rect on _WantCreate but using the full rect allows us to clear the texture.
        // FIXME-OPT: Uploading single box even when using ImTextureStatus_WantUpdates. Could use tex->Updates[]
        // - Copy all blocks contiguously in upload buffer.
        // - Barrier before copy, submit all CopyTextureRegion(), barrier after copy.
        const int upload_x = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.x;
        const int upload_y = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.y;
        const int upload_w = (tex->Status == ImTextureStatus_WantCreate) ? tex->Width : tex->UpdateRect.w;
        const int upload_h = (tex->Status == ImTextureStatus_WantCreate) ? tex->Height : tex->UpdateRect.h;

        // Update full texture or selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->UpdateRect but you can use tex->Updates[] to upload individual regions.
        UINT upload_pitch_src = static_cast<UINT>(upload_w * tex->BytesPerPixel);
        UINT upload_pitch_dst = (upload_pitch_src + TEXTURE_PITCH - 1u) & ~(TEXTURE_PITCH - 1u);
        UINT upload_size = upload_pitch_dst * upload_h;

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = upload_size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES props;
        memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // FIXME-OPT: Can upload buffer be reused?
        ID3D12Resource* uploadBuffer = nullptr;
        HRESULT hr = bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(&uploadBuffer));
        IM_ASSERT(SUCCEEDED(hr));

        // Create temporary command list and execute immediately
        ID3D12Fence* fence = nullptr;
        hr = bd->pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(&fence));
        IM_ASSERT(SUCCEEDED(hr));

        HANDLE event = ::CreateEvent(0, 0, 0, 0);
        IM_ASSERT(event != nullptr);

        // FIXME-OPT: Create once and reuse?
        ID3D12CommandAllocator* cmdAlloc = nullptr;
        hr = bd->pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(&cmdAlloc));
        IM_ASSERT(SUCCEEDED(hr));

        // FIXME-OPT: Can be use the one from user? (pass ID3D12GraphicsCommandList* to ImGui_ImplDX12_UpdateTextures)
        ID3D12GraphicsCommandList* cmdList = nullptr;
        hr = bd->pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, nullptr, IID_GRAPHICS_PPV_ARGS(&cmdList));
        IM_ASSERT(SUCCEEDED(hr));

        // Copy to upload buffer
        void* mapped = nullptr;
        D3D12_RANGE range = { 0, upload_size };
        hr = uploadBuffer->Map(0, &range, &mapped);
        IM_ASSERT(SUCCEEDED(hr));
        for (int y = 0; y < upload_h; y++)
            memcpy((void*)((uintptr_t)mapped + y * upload_pitch_dst), tex->GetPixelsAt(upload_x, upload_y + y), upload_pitch_src);
        uploadBuffer->Unmap(0, &range);

        if (need_barrier_before_copy)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = backend_tex->pTextureResource;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            cmdList->ResourceBarrier(1, &barrier);
        }

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        {
            srcLocation.pResource = uploadBuffer;
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = static_cast<UINT>(upload_w);
            srcLocation.PlacedFootprint.Footprint.Height = static_cast<UINT>(upload_h);
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = upload_pitch_dst;
            dstLocation.pResource = backend_tex->pTextureResource;
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;
        }

        cmdList->CopyTextureRegion(&dstLocation, static_cast<UINT>(upload_x), static_cast<UINT>(upload_y), 0, &srcLocation, nullptr);
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = backend_tex->pTextureResource;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            cmdList->ResourceBarrier(1, &barrier);
        }

        hr = cmdList->Close();
        IM_ASSERT(SUCCEEDED(hr));

        ID3D12CommandQueue* cmdQueue = bd->pCommandQueue;
        cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
        hr = cmdQueue->Signal(fence, 1);
        IM_ASSERT(SUCCEEDED(hr));

        // FIXME-OPT: Suboptimal?
        // - To remove this may need to create NumFramesInFlight x ImGui_ImplDX12_FrameContext in backend data (mimick docking version)
        // - Store per-frame in flight: upload buffer?
        // - Where do cmdList and cmdAlloc fit?
        fence->SetEventOnCompletion(1, event);
        ::WaitForSingleObject(event, INFINITE);

        cmdList->Release();
        cmdAlloc->Release();
        ::CloseHandle(event);
        fence->Release();
        uploadBuffer->Release();

        tex->SetStatus(ImTextureStatus_OK);
    }

    if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames >= (int)bd->numFramesInFlight)
        ImGui_ImplDX12_DestroyTexture(tex);
}

bool    ImGui_ImplDX12_CreateDeviceObjects()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    if (bd->pPipelineState)
        ImGui_ImplDX12_InvalidateDeviceObjects();

    // Create the root signature
    {
        D3D12_DESCRIPTOR_RANGE descRange = {};
        descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRange.NumDescriptors = 1;
        descRange.BaseShaderRegister = 0;
        descRange.RegisterSpace = 0;
        descRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER param[2] = {};

        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param[0].Constants.ShaderRegister = 0;
        param[0].Constants.RegisterSpace = 0;
        param[0].Constants.Num32BitValues = 16;
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges = &descRange;
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.MipLODBias = 0.f;
        staticSampler.MaxAnisotropy = 0;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSampler.MinLOD = 0.f;
        staticSampler.MaxLOD = 0.f;
        staticSampler.ShaderRegister = 0;
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(param);
        desc.pParameters = param;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &staticSampler;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        bd->pd3dDevice->CreateRootSignature(0, g_imgui_impl_dx12_rs, sizeof(g_imgui_impl_dx12_rs), IID_GRAPHICS_PPV_ARGS(&bd->pRootSignature));
    }

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX12 sample code but remove this dependency you can:
    //  1) compile once, save the compiled shader blobs into a file or source code and assign them to psoDesc.VS/PS [preferred solution]
    //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
    // See https://github.com/ocornut/imgui/pull/638 for sources and details.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.NodeMask = 1;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.pRootSignature = bd->pRootSignature;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = bd->RTVFormat;
    psoDesc.DSVFormat = bd->DSVFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    // Create the vertex shader
    {
        psoDesc.VS.pShaderBytecode = g_imgui_impl_dx12_vs;
        psoDesc.VS.BytecodeLength = sizeof(g_imgui_impl_dx12_vs);

        // Create the input layout
        static D3D12_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        psoDesc.InputLayout = { local_layout, 3 };
    }

    // Create the pixel shader
    {
        psoDesc.PS.pShaderBytecode = g_imgui_impl_dx12_ps;
        psoDesc.PS.BytecodeLength = sizeof(g_imgui_impl_dx12_ps);
    }

    // Create the blending setup
    {
        D3D12_BLEND_DESC& desc = psoDesc.BlendState;
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // Create the rasterizer state
    {
        D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
        desc.FillMode = D3D12_FILL_MODE_SOLID;
        desc.CullMode = D3D12_CULL_MODE_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount = 0;
        desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    // Create depth-stencil State
    {
        D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BackFace = desc.FrontFace;
    }

    HRESULT result_pipeline_state = bd->pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(&bd->pPipelineState));
    if (result_pipeline_state != S_OK)
        return false;

    return true;
}

void    ImGui_ImplDX12_InvalidateDeviceObjects()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return;

    if (bd->commandQueueOwned)
        SafeRelease(bd->pCommandQueue);
    bd->commandQueueOwned = false;
    SafeRelease(bd->pRootSignature);
    SafeRelease(bd->pPipelineState);

    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
            ImGui_ImplDX12_DestroyTexture(tex);

    for (UINT i = 0; i < bd->numFramesInFlight; i++)
    {
        ImGui_ImplDX12_RenderBuffers* fr = &bd->pFrameResources[i];
        SafeRelease(fr->IndexBuffer);
        SafeRelease(fr->VertexBuffer);
    }
}

bool ImGui_ImplDX12_Init(ImGui_ImplDX12_InitInfo* init_info)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX12_Data* bd = IM_NEW(ImGui_ImplDX12_Data)();
    bd->InitInfo = *init_info; // Deep copy
    init_info = &bd->InitInfo;

    bd->pd3dDevice = init_info->Device;
    IM_ASSERT(init_info->CommandQueue != NULL);
    bd->pCommandQueue = init_info->CommandQueue;
    bd->RTVFormat = init_info->RTVFormat;
    bd->DSVFormat = init_info->DSVFormat;
    bd->numFramesInFlight = init_info->NumFramesInFlight;
    bd->pd3dSrvDescHeap = init_info->SrvDescriptorHeap;

    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_dx12";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.

    IM_ASSERT(init_info->SrvDescriptorAllocFn != nullptr && init_info->SrvDescriptorFreeFn != nullptr);

    // Create buffers with a default size (they will later be grown as needed)
    bd->frameIndex = UINT_MAX;
    bd->pFrameResources = new ImGui_ImplDX12_RenderBuffers[bd->numFramesInFlight];
    for (int i = 0; i < (int)bd->numFramesInFlight; i++)
    {
        ImGui_ImplDX12_RenderBuffers* fr = &bd->pFrameResources[i];
        fr->IndexBuffer = nullptr;
        fr->VertexBuffer = nullptr;
        fr->IndexBufferSize = 10000;
        fr->VertexBufferSize = 5000;
    }

    return true;
}

void ImGui_ImplDX12_Shutdown()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    // Clean up windows and device objects
    ImGui_ImplDX12_InvalidateDeviceObjects();
    delete[] bd->pFrameResources;

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    IM_DELETE(bd);
}

void ImGui_ImplDX12_NewFrame()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX12_Init()?");

    if (!bd->pPipelineState)
        ImGui_ImplDX12_CreateDeviceObjects();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
