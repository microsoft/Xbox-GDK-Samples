#pragma once

#include "SpriteBatch.h"
#include "CommonStates.h"


namespace ThunderRumble
{
    enum class TexturePosition
    {
        None,
        Centered
    };

    struct TextureHandle
    {
        TextureHandle() = default;
        TextureHandle(std::shared_ptr<DX::Texture> texture) : Texture(texture) {}

        std::shared_ptr<DX::Texture> Texture;
        DirectX::XMUINT2 GetTextureSize() const { return DirectX::XMUINT2{ static_cast<uint32_t>(Texture->Width()), static_cast<uint32_t>(Texture->Height()) }; }
    };

    class RenderContext
    {
    public:
        void Begin(DirectX::FXMMATRIX transformMatrix = MatrixIdentity, DirectX::SpriteSortMode sortMode = DirectX::SpriteSortMode::SpriteSortMode_Deferred);
        void End();

        void Draw(const TextureHandle &texture, const DirectX::XMFLOAT2 &position, float rotation = 0.0f, float scale = 1.0f, DirectX::FXMVECTOR color = DirectX::Colors::White, TexturePosition texturePosition = TexturePosition::Centered);
        void Draw(const TextureHandle &texture, const RECT &destinationRect, DirectX::FXMVECTOR color = DirectX::Colors::White);

        void DrawString(std::shared_ptr<DirectX::SpriteFont> font, const std::wstring &message, const DirectX::XMFLOAT2 &position, DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0, const DirectX::XMFLOAT2 &origin = Float2Zero, float scale = 1);
    private:
        RenderContext(std::shared_ptr<DirectX::SpriteBatch> spriteBatch, ID3D11BlendState *blendstate);

        ID3D11BlendState*                     m_blendState;
        std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch;

        static const DirectX::XMMATRIX MatrixIdentity;
        static const DirectX::XMFLOAT2 Float2Zero;

        friend class RenderManager;
    };
}
