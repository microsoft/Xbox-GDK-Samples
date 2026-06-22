#include "pch.h"
#include "RenderContext.h"

#include "SpriteBatch.h"

namespace PlayFabMultiplayerRumble
{
    RenderContext::RenderContext(std::shared_ptr<DirectX::SpriteBatch> spriteBatch, ID3D12GraphicsCommandList *commandList)
        : m_commandList(commandList), m_spriteBatch(spriteBatch)

    {

    }

    void RenderContext::Begin(DirectX::FXMMATRIX transformMatrix, DirectX::SpriteSortMode sortMode)
    {
        m_spriteBatch->Begin(m_commandList, sortMode, transformMatrix);
    }

    void RenderContext::End()
    {
        m_spriteBatch->End();
    }

    void RenderContext::Draw(const TextureHandle &texture, const DirectX::XMFLOAT2 &position, float rotation, float scale, DirectX::FXMVECTOR color, TexturePosition texturePosition)
    {
        auto textureSize = texture.GetTextureSize();
        m_spriteBatch->Draw(texture.TextureGPUHandle, textureSize, position, nullptr, color, rotation, texturePosition == TexturePosition::Centered ? DirectX::XMFLOAT2{ textureSize.x / 2.0f, textureSize.y / 2.0f } : Float2Zero, scale);
    }

    void RenderContext::Draw(const TextureHandle &texture, const RECT &destinationRect, DirectX::FXMVECTOR color, float rotation, TexturePosition texturePosition)
    {
		auto textureSize = texture.GetTextureSize();
        m_spriteBatch->Draw(texture.TextureGPUHandle, texture.GetTextureSize(), destinationRect, nullptr, color, rotation, texturePosition == TexturePosition::Centered ? DirectX::XMFLOAT2{ textureSize.x / 2.0f, textureSize.y / 2.0f } : Float2Zero);
    }

    void RenderContext::DrawString(std::shared_ptr<DirectX::SpriteFont> font, std::string_view message, const DirectX::XMFLOAT2 &position, DirectX::FXMVECTOR color, float rotation, const DirectX::XMFLOAT2 &origin, float scale)
    {
        font->DrawString(m_spriteBatch.get(), message.data(), position, color, rotation, origin, scale);
    }

    const DirectX::XMMATRIX RenderContext::MatrixIdentity = DirectX::XMMatrixIdentity();
    const DirectX::XMFLOAT2 RenderContext::Float2Zero = { 0,0 };
}