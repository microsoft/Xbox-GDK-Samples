#include "pch.h"
#include "RenderContext.h"

#include "SpriteBatch.h"

namespace ThunderRumble
{
    RenderContext::RenderContext()
    {

    }

    void RenderContext::Begin(DirectX::FXMMATRIX transformMatrix, DirectX::SpriteSortMode sortMode)
    {
    }

    void RenderContext::End()
    {
    }

    void RenderContext::Draw(const TextureHandle &texture, const DirectX::XMFLOAT2 &position, float rotation, float scale, DirectX::FXMVECTOR color, TexturePosition texturePosition)
    {
    }

    void RenderContext::Draw(const TextureHandle &texture, const RECT &destinationRect, DirectX::FXMVECTOR color)
    {
    }

    void RenderContext::DrawString(std::shared_ptr<DirectX::SpriteFont> font, const std::wstring &message, const DirectX::XMFLOAT2 &position, DirectX::FXMVECTOR color, float rotation, const DirectX::XMFLOAT2 &origin, float scale)
    {
    }

    const DirectX::XMMATRIX RenderContext::MatrixIdentity = DirectX::XMMatrixIdentity();
    const DirectX::XMFLOAT2 RenderContext::Float2Zero = { 0,0 };
}