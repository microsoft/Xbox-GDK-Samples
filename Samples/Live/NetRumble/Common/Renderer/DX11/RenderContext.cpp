#include "pch.h"
#include "RenderContext.h"

#include "SpriteBatch.h"

namespace ThunderRumble
{
    RenderContext::RenderContext(std::shared_ptr<DirectX::SpriteBatch> spriteBatch, ID3D11BlendState *blendstate)
        : m_spriteBatch(spriteBatch), m_blendState(blendstate)
    {

    }

    void RenderContext::Begin(DirectX::FXMMATRIX transformMatrix, DirectX::SpriteSortMode sortMode)
    {
        m_spriteBatch->Begin(sortMode, m_blendState, nullptr, nullptr, nullptr, nullptr, transformMatrix);
    }

    void RenderContext::End()
    {
        m_spriteBatch->End();
    }

    void RenderContext::Draw(const TextureHandle &texture, const DirectX::XMFLOAT2 &position, float rotation, float scale, DirectX::FXMVECTOR color, TexturePosition texturePosition)
    {

        m_spriteBatch->Draw(texture.Texture->Get(), position, nullptr, color, rotation, texturePosition == TexturePosition::Centered ? DirectX::XMFLOAT2{ texture.Texture->Width() / 2.0f, texture.Texture->Height() / 2.0f } : Float2Zero, scale);
    }

    void RenderContext::Draw(const TextureHandle &texture, const RECT &destinationRect, DirectX::FXMVECTOR color)
    {
        m_spriteBatch->Draw(texture.Texture->Get(), destinationRect, nullptr, color);
    }

    void RenderContext::DrawString(std::shared_ptr<DirectX::SpriteFont> font, const std::wstring &message, const DirectX::XMFLOAT2 &position, DirectX::FXMVECTOR color, float rotation, const DirectX::XMFLOAT2 &origin, float scale)
    {
        font->DrawString(m_spriteBatch.get(), message.c_str(), position, color, rotation, origin, scale);
    }

    const DirectX::XMMATRIX RenderContext::MatrixIdentity = DirectX::XMMatrixIdentity();
    const DirectX::XMFLOAT2 RenderContext::Float2Zero = { 0,0 };
}