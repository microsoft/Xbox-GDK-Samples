#pragma once

#include "SpriteBatch.h"


namespace NetRumble
{
    enum class TexturePosition
    {
        None,
        Centered
    };

    struct TextureHandle
    {
        TextureHandle() noexcept = default;
        TextureHandle(std::shared_ptr<DX::Texture> texture, D3D12_GPU_DESCRIPTOR_HANDLE handle) : Texture(texture), TextureGPUHandle(handle) {}

        std::shared_ptr<DX::Texture> Texture;
        D3D12_GPU_DESCRIPTOR_HANDLE TextureGPUHandle = {};

        DirectX::XMUINT2 GetTextureSize() const { return Texture->GetTextureSize(); }
    };

    class RenderContext
    {
    public:
        void Begin(DirectX::FXMMATRIX transformMatrix = MatrixIdentity, DirectX::SpriteSortMode sortMode = DirectX::SpriteSortMode::SpriteSortMode_Deferred);
        void End();

        void Draw(const TextureHandle &texture, const DirectX::XMFLOAT2 &position, float rotation = 0.0f, float scale = 1.0f, DirectX::FXMVECTOR color = DirectX::Colors::White, TexturePosition texturePosition = TexturePosition::Centered);
        void Draw(const TextureHandle &texture, const RECT &destinationRect, DirectX::FXMVECTOR color = DirectX::Colors::White, float rotations = 0.0f, TexturePosition texturePosition = TexturePosition::Centered);

        void DrawString(std::shared_ptr<DirectX::SpriteFont> font, std::string_view message, const DirectX::XMFLOAT2 &position, DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0, const DirectX::XMFLOAT2 &origin = Float2Zero, float scale = 1);
    private:
        RenderContext(std::shared_ptr<DirectX::SpriteBatch> spriteBatch, ID3D12GraphicsCommandList *commandList);

        ID3D12GraphicsCommandList *m_commandList;
        std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch;

        static const DirectX::XMMATRIX MatrixIdentity;
        static const DirectX::XMFLOAT2 Float2Zero;

        friend class RenderManager;
    };
}
