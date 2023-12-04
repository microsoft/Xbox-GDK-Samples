//--------------------------------------------------------------------------------------
// Particles.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Particles.h"

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

namespace
{
    //--------------------------------------------------------------------------------------
    // Constants

    constexpr int   g_NumStalkParticles = 500;
    constexpr int   g_NumGroundBurstParticles = 345;
    constexpr int   g_NumMushroomParticles = 200;

    constexpr int   g_MaxMushroomClouds = 8;

    constexpr float g_ParticleSpread = 4.0f;
    constexpr float g_ParticleStartSize = 0.0f;
    constexpr float g_ParticleEndSize = 10.0f;
    constexpr float g_ParticleSizeExponent = 128.0f;

    constexpr float g_ParticleMushroomCloudLifeSpan = 10.0f;
    constexpr float g_ParticleGroundBurstLifeSpan = 9.0f;

    constexpr float g_ParticleMushroomStartSpeed = 20.0f;
    constexpr float g_ParticleStalkStartSpeed = 50.0f;
    constexpr float g_ParticleGroundBurstStartSpeed = 100.0f;

    constexpr float g_ParticleEndSpeed = 4.0f;
    constexpr float g_ParticleSpeedExponent = 32.0f;
    constexpr float g_ParticleFadeExponent = 4.0f;
    constexpr float g_ParticleRollAmount = 0.2f;
    constexpr float g_ParticleWindFalloff = 20.0f;

    constexpr float g_ParticleLightRaise = 1.0f;

    constexpr int   g_MaxFlashColors = 4;

    static const XMVECTOR g_FlashAttenuation = XMVectorSet(0, 0.0f, 3.0f, 0);
    static const XMVECTOR g_MeshLightAttenuation = XMVectorSet(0, 0, 1.5f, 0);
    constexpr float g_FlashLife = 0.50f;
    constexpr float g_FlashIntensity = 1000.0f;

    //--------------------------------------------------------------------------------------
    // Array of colors for explosion flashes.

    static const XMVECTOR FLASH_COLORS[g_MaxFlashColors] =
    {
        XMVECTOR(XMVectorSet(1.0f, 0.5f, 0.00f, 0.9f)),
        XMVECTOR(XMVectorSet(1.0f, 0.3f, 0.05f, 0.9f)),
        XMVECTOR(XMVectorSet(1.0f, 0.4f, 0.00f, 0.9f)),
        XMVECTOR(XMVectorSet(0.8f, 0.3f, 0.05f, 0.9f))
    };

    // Get a random percentage value.
    float RandPercent()
    {
        float ret = static_cast<float>((rand() % 20000) - 10000);
        return ret / 10000.0f;
    }

    // Perform a quick depth sort on a collection of particles.
    template <class T>
    void QuickDepthSort(T* pIndices, float* pDepths, int lo, int hi)
    {
        //  lo is the lower index, hi is the upper index
        //  of the region of array a that is to be sorted
        int i = lo, j = hi;
        float h;
        T index;
        float x = pDepths[(lo + hi) / 2];

        //  partition
        do
        {
            while (pDepths[i] < x) i++;
            while (pDepths[j] > x) j--;
            if (i <= j)
            {
                h = pDepths[i]; pDepths[i] = pDepths[j]; pDepths[j] = h;
                index = pIndices[i]; pIndices[i] = pIndices[j]; pIndices[j] = index;
                i++; j--;
            }
        } while (i <= j);

        //  recursion
        if (lo < j)
        {
            QuickDepthSort(pIndices, pDepths, lo, j);
        }

        if (i < hi)
        {
            QuickDepthSort(pIndices, pDepths, i, hi);
        }
    }

    // Element size lookup of common vertex attribute formats
    int GetElementSize(DXGI_FORMAT format)
    {
        switch (format)
        {
            // 8-bit Formats
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_UNORM:
            return 1;
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_UNORM:
            return 2;
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return 4;
            // 16-bit Formats
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_FLOAT:
            return 2;
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
            return 4;
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return 8;
            // 32-bit Formats
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_FLOAT:
            return 4;
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
            return 8;
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return 12;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return 16;

        default:
            throw std::exception("Missed a vertex attribute case.");
        }
    }
}

namespace ATG
{
    ParticleSystem::ParticleSystem()
        : m_posMul{}
        , m_dirMul{}
        , m_center{}
        , m_flashColor{}
        , m_particles(nullptr)
        , m_numParticles(0)
        , m_spread(0.0f)
        , m_lifeSpan(0.0f)
        , m_startSize(0.0f)
        , m_endSize(0.0f)
        , m_sizeExponent(0.0f)
        , m_startSpeed(0.0f)
        , m_endSpeed(0.0f)
        , m_speedExponent(0.0f)
        , m_fadeExponent(0.0f)
        , m_rollAmount(0.0f)
        , m_windFalloff(0.0f)
        , m_currentTime(0.0f)
        , m_startTime(0.0f)
    { }

    HRESULT ParticleSystem::CreateParticleSystem(ParticleWorld* pWorld, int NumParticles)
    {
        m_particles = pWorld->GetNextParticleBlock(UINT(NumParticles));
        m_numParticles = NumParticles;
        return S_OK;
    }

    void ParticleSystem::SetSystemAttributes(FXMVECTOR Center,
        float Spread, float LifeSpan, float FadeExponent,
        float StartSize, float EndSize, float SizeExponent,
        float StartSpeed, float EndSpeed, float SpeedExponent,
        float RollAmount, float WindFalloff,
        int NumStreamers, float SpeedVariance,
        FXMVECTOR Direction, FXMVECTOR DirVariance,
        GXMVECTOR Color0, CXMVECTOR Color1,
        CXMVECTOR PosMul, CXMVECTOR DirMul)
    {
        XMStoreFloat3(&m_center, Center);
        m_spread = Spread;
        m_lifeSpan = LifeSpan;
        m_startSize = StartSize;
        m_endSize = EndSize;
        m_sizeExponent = SizeExponent;
        m_startSpeed = StartSpeed;
        m_endSpeed = EndSpeed;
        m_speedExponent = SpeedExponent;
        m_fadeExponent = FadeExponent;

        m_rollAmount = RollAmount;
        m_windFalloff = WindFalloff;
        XMStoreFloat3(&m_posMul, PosMul);
        XMStoreFloat3(&m_dirMul, DirMul);

        m_numStreamers = NumStreamers;
        m_speedVariance = SpeedVariance;
        XMStoreFloat3(&m_direction, Direction);
        XMStoreFloat3(&m_dirVariance, DirVariance);

        XMStoreFloat4(&m_color0, Color0);
        XMStoreFloat4(&m_color1, Color1);

        Init();
    }

    void ParticleSystem::Init()
    {
        for (int i = 0; i < m_numParticles; i++)
        {
            XMVECTOR offset = XMVectorMultiply(XMVectorScale(XMVectorSet(RandPercent(), RandPercent(), RandPercent(), 0), m_spread), XMLoadFloat3(&m_posMul));
            XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_center), offset));

            XMVECTOR dir = XMVectorMultiply(XMVectorSet(RandPercent(), fabs(RandPercent()), RandPercent(), 0), XMLoadFloat3(&m_dirMul));
            XMStoreFloat3(&m_particles[i].Dir, XMVector3Normalize(dir));

            m_particles[i].Radius = m_startSize;
            m_particles[i].Life = m_startTime;
            m_particles[i].Fade = 0.0f;

            m_particles[i].Rot = RandPercent() * XM_PI * 2.0f;

            float Lerp = RandPercent();
            XMVECTOR Color = XMVectorAdd(XMVectorScale(XMLoadFloat4(&m_color0), Lerp), XMVectorScale(XMLoadFloat4(&m_color1), (1.0f - Lerp)));
            m_particles[i].Color = static_cast<DWORD>(XMVectorGetW(Color) * 255.0f) << 24;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetZ(Color) * 255.0f) & 255) << 16;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetY(Color) * 255.0f) & 255) << 8;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetX(Color) * 255.0f) & 255);
        }

        m_hasStarted = FALSE;
        m_currentTime = m_startTime;
    }

    void ParticleSystem::AdvanceSystem(float ElapsedTime, FXMVECTOR /*Right*/, FXMVECTOR /*Up*/, FXMVECTOR WindVel, GXMVECTOR /*Gravity*/)
    {
        if (m_currentTime > 0)
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                float t = m_particles[i].Life / m_lifeSpan;
                float tm1 = t - 1.0f;
                float SizeLerp = 1.0f - powf(tm1, m_sizeExponent);
                float SpeedLerp = 1.0f - powf(tm1, m_speedExponent);
                float FadeLerp = 1.0f - powf(tm1, m_fadeExponent);

                float Size = SizeLerp * m_endSize + (1.0f - SizeLerp) * m_startSize;
                float Speed = SpeedLerp * m_endSpeed + (1.0f - SpeedLerp) * m_startSpeed;
                float Fade = FadeLerp;

                XMVECTOR Vel = XMVectorScale(XMLoadFloat3(&m_particles[i].Dir), Speed);
                float Rot = 0.0f;
                float WindAmt = 1.0f;

                Vel = XMVectorAdd(Vel, XMVectorScale(WindVel, WindAmt));

                XMVECTOR dPos = XMVectorScale(Vel, ElapsedTime);
                XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_particles[i].Pos), dPos));

                m_particles[i].Radius = Size;

                m_particles[i].Life += ElapsedTime;
                m_particles[i].Fade = Fade;

                m_particles[i].Rot += Rot;

                m_particles[i].IsVisible = TRUE;
            }
            if (!m_hasStarted)
            {
                m_hasStarted = TRUE;
            }
        }
        else
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                m_particles[i].IsVisible = FALSE;
                m_particles[i].Life += ElapsedTime;
            }
        }

        m_currentTime += ElapsedTime;
    }

    void MushroomParticleSystem::Init()
    {
        for (int i = 0; i < m_numParticles; i++)
        {
            XMVECTOR offset = XMVectorMultiply(XMVectorScale(XMVectorSet(RandPercent(), RandPercent(), RandPercent(), 0), m_spread), XMLoadFloat3(&m_posMul));
            XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_center), offset));

            XMVECTOR dir = XMVectorMultiply(XMVectorSet(RandPercent(), fabs(RandPercent()), RandPercent(), 0), XMLoadFloat3(&m_dirMul));
            XMStoreFloat3(&m_particles[i].Dir, XMVector3Normalize(dir));

            m_particles[i].Radius = m_startSize;
            m_particles[i].Life = m_startTime;
            m_particles[i].Fade = 0.0f;

            m_particles[i].Rot = RandPercent() * XM_PI * 2.0f;

            float Lerp = RandPercent();
            XMVECTOR Color = XMVectorAdd(XMVectorScale(XMLoadFloat4(&m_color0), Lerp), XMVectorScale(XMLoadFloat4(&m_color1), (1.0f - Lerp)));
            m_particles[i].Color = static_cast<DWORD>(XMVectorGetW(Color) * 255.0f) << 24;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetZ(Color) * 255.0f) & 255) << 16;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetY(Color) * 255.0f) & 255) << 8;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetX(Color) * 255.0f) & 255);
        }

        m_hasStarted = FALSE;
        m_currentTime = m_startTime;
    }

    void MushroomParticleSystem::AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR /*Up*/, FXMVECTOR WindVel, GXMVECTOR /*Gravity*/)
    {
        if (m_currentTime > 0)
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                float t = m_particles[i].Life / m_lifeSpan;
                float tm1 = t - 1.0f;
                float SizeLerp = 1.0f - powf(tm1, m_sizeExponent);
                float SpeedLerp = 1.0f - powf(tm1, m_speedExponent);
                float FadeLerp = 1.0f - powf(tm1, m_fadeExponent);

                float Size = SizeLerp * m_endSize + (1.0f - SizeLerp) * m_startSize;
                float Speed = SpeedLerp * m_endSpeed + (1.0f - SpeedLerp) * m_startSpeed;
                float Fade = FadeLerp;

                XMVECTOR Delta = XMVectorSubtract(XMLoadFloat3(&m_particles[i].Pos), XMLoadFloat3(&m_center));
                float rightDist = XMVectorGetX(XMVector3Dot(Delta, Right));

                XMVECTOR Vel = XMVectorScale(XMLoadFloat3(&m_particles[i].Dir), Speed);
                float Rot = 0.0f;
                float WindAmt = 1.0f;

                // Higher level should roll outward
                Rot = -rightDist * m_rollAmount * ElapsedTime * (1.0f - t);

                Vel = XMVectorAdd(Vel, XMVectorScale(WindVel, WindAmt));

                XMVECTOR dPos = XMVectorScale(Vel, ElapsedTime);
                XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_particles[i].Pos), dPos));

                m_particles[i].Radius = Size;

                m_particles[i].Life += ElapsedTime;
                m_particles[i].Fade = Fade;

                m_particles[i].Rot += Rot;

                m_particles[i].IsVisible = TRUE;
            }

            if (!m_hasStarted)
            {
                m_hasStarted = TRUE;
            }
        }
        else
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                m_particles[i].IsVisible = FALSE;
                m_particles[i].Life += ElapsedTime;
            }
        }

        m_currentTime += ElapsedTime;
    }

    void StalkParticleSystem::Init()
    {
        for (int i = 0; i < m_numParticles; i++)
        {
            XMVECTOR offset = XMVectorMultiply(XMVectorScale(XMVectorSet(RandPercent(), RandPercent(), RandPercent(), 0), m_spread), XMLoadFloat3(&m_posMul));
            XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_center), offset));

            XMVECTOR dir = XMVectorMultiply(XMVectorSet(RandPercent(), fabs(RandPercent()), RandPercent(), 0), XMLoadFloat3(&m_dirMul));
            XMStoreFloat3(&m_particles[i].Dir, XMVector3Normalize(dir));

            m_particles[i].Radius = m_startSize;
            m_particles[i].Life = m_startTime;
            m_particles[i].Fade = 0.0f;

            m_particles[i].Rot = RandPercent() * XM_PI * 2.0f;

            float Lerp = RandPercent();
            XMVECTOR Color = XMVectorAdd(XMVectorScale(XMLoadFloat4(&m_color0), Lerp), XMVectorScale(XMLoadFloat4(&m_color1), (1.0f - Lerp)));
            m_particles[i].Color = static_cast<DWORD>(XMVectorGetW(Color) * 255.0f) << 24;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetZ(Color) * 255.0f) & 255) << 16;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetY(Color) * 255.0f) & 255) << 8;
            m_particles[i].Color |= (static_cast<DWORD>(XMVectorGetX(Color) * 255.0f) & 255);
        }

        m_hasStarted = FALSE;
        m_currentTime = m_startTime;
    }

    void StalkParticleSystem::AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR /*Up*/, FXMVECTOR WindVel, GXMVECTOR /*Gravity*/)
    {
        if (m_currentTime > 0)
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                float t = m_particles[i].Life / m_lifeSpan;
                float tm1 = t - 1.0f;
                float SizeLerp = 1.0f - powf(tm1, m_sizeExponent);
                float SpeedLerp = 1.0f - powf(tm1, m_speedExponent);
                float FadeLerp = 1.0f - powf(tm1, m_fadeExponent);

                float Size = SizeLerp * m_endSize + (1.0f - SizeLerp) * m_startSize;
                float Speed = SpeedLerp * m_endSpeed + (1.0f - SpeedLerp) * m_startSpeed;
                float Fade = FadeLerp;

                XMVECTOR Delta = XMVectorSubtract(XMLoadFloat3(&m_particles[i].Pos), XMLoadFloat3(&m_center));
                float rightDist = XMVectorGetX(XMVector3Dot(Delta, Right));

                XMVECTOR Vel = XMVectorScale(XMLoadFloat3(&m_particles[i].Dir), Speed);
                float Rot = 0.0f;
                float WindAmt = 1.0f;

                XMVECTOR DeltaXZ = Delta;
                DeltaXZ = XMVectorSetY(DeltaXZ, 0.0f);

                float LenSq = XMVectorGetX(XMVector3LengthSq(DeltaXZ)) / m_spread;
                float Exp = std::min(XMVectorGetY(WindVel), (1.0f / LenSq));
                Vel = XMVectorSubtract(Vel, XMVectorSet(0.1f * XMVectorGetX(DeltaXZ), 0.1f * XMVectorGetZ(DeltaXZ), Exp, 0));

                WindAmt = std::max(0.0f, std::min(1.0f, XMVectorGetY(Delta) / m_windFalloff));

                // Lower level should roll inward
                Rot = rightDist * m_rollAmount * ElapsedTime * (1.0f - t);

                Vel = XMVectorAdd(Vel, XMVectorScale(WindVel, WindAmt));

                XMVECTOR dPos = XMVectorScale(Vel, ElapsedTime);
                XMStoreFloat3(&m_particles[i].Pos, XMVectorAdd(XMLoadFloat3(&m_particles[i].Pos), dPos));

                m_particles[i].Radius = Size;

                m_particles[i].Life += ElapsedTime;
                m_particles[i].Fade = Fade;

                m_particles[i].Rot += Rot;

                m_particles[i].IsVisible = TRUE;
            }

            if (!m_hasStarted)
            {
                m_hasStarted = TRUE;
            }
        }
        else
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                m_particles[i].IsVisible = FALSE;
                m_particles[i].Life += ElapsedTime;
            }
        }

        m_currentTime += ElapsedTime;
    }

    void GroundBurstParticleSystem::Init()
    {
        int index = 0;
        int ParticlesPerStream = (m_numParticles / m_numStreamers) + 1;
        for (int s = 0; s < m_numStreamers; s++)
        {
            XMVECTOR StreamerDir = XMVectorAdd(XMLoadFloat3(&m_direction), XMVectorMultiply(XMVectorSet(RandPercent(), RandPercent(), RandPercent(), 0), XMLoadFloat3(&m_dirVariance)));
            StreamerDir = XMVector3Normalize(StreamerDir);

            XMVECTOR StreamerPos = XMVectorScale(XMVectorSet(RandPercent(), RandPercent(), RandPercent(), 0), m_spread);

            for (int i = 0; i < ParticlesPerStream; i++)
            {
                if (index < m_numParticles)
                {
                    XMVECTOR delta = XMVectorMultiply(StreamerPos, XMLoadFloat3(&m_posMul));
                    XMStoreFloat3(&m_particles[index].Pos, XMVectorAdd(XMLoadFloat3(&m_center), delta));

                    float Speed = m_startSpeed + RandPercent() * m_speedVariance;

                    XMStoreFloat3(&m_particles[index].Dir, XMVectorMultiply(XMVectorScale(StreamerDir, Speed), XMLoadFloat3(&m_dirMul)));

                    float RadiusLerp = (Speed / (m_startSpeed + m_speedVariance));

                    m_particles[index].Radius = m_startSize * RadiusLerp + m_endSize * (1 - RadiusLerp);
                    m_particles[index].Life = m_startTime;
                    m_particles[index].Fade = 0.0f;

                    m_particles[index].Rot = RandPercent() * XM_PI * 2.0f;

                    m_particles[index].RotRate = RandPercent() * 1.5f;

                    float Lerp = RandPercent();
                    XMVECTOR Color = XMVectorAdd(XMVectorScale(XMLoadFloat4(&m_color0), Lerp), XMVectorScale(XMLoadFloat4(&m_color1), (1.0f - Lerp)));
                    m_particles[index].Color = static_cast<DWORD>(XMVectorGetW(Color) * 255.0f) << 24;
                    m_particles[index].Color |= (static_cast<DWORD>(XMVectorGetZ(Color) * 255.0f) & 255) << 16;
                    m_particles[index].Color |= (static_cast<DWORD>(XMVectorGetY(Color) * 255.0f) & 255) << 8;
                    m_particles[index].Color |= (static_cast<DWORD>(XMVectorGetX(Color) * 255.0f) & 255);

                    index++;
                }
            }
        }

        m_hasStarted = FALSE;

        m_currentTime = m_startTime;
    }

    void GroundBurstParticleSystem::AdvanceSystem(float ElapsedTime, FXMVECTOR /*Right*/, FXMVECTOR /*Up*/, FXMVECTOR WindVel, GXMVECTOR Gravity)
    {
        if (m_currentTime > 0)
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                float t = m_particles[i].Life / m_lifeSpan;
                float tm1 = t - 1.0f;
                float SizeLerp = 1.0f - powf(tm1, m_sizeExponent);
                float SpeedLerp = powf(tm1, m_speedExponent);
                float FadeLerp = 1.0f - powf(tm1, m_fadeExponent);
                float Fade = FadeLerp;

                XMVECTOR pos = XMLoadFloat3(&m_particles[i].Pos);
                XMVECTOR dir = XMLoadFloat3(&m_particles[i].Dir);
                XMVECTOR Delta = XMVectorSubtract(pos, XMLoadFloat3(&m_center));

                float Rot = m_particles[i].RotRate * ElapsedTime;
                float WindAmt = 1.0f;

                WindAmt = std::max(0.0f, std::min(1.0f, XMVectorGetY(Delta) / m_windFalloff));
                XMVECTOR Wind = XMVectorScale(WindVel, WindAmt);
                Wind = XMVectorSetY(Wind, 0);

                pos = XMVectorAdd(pos, XMVectorScale((XMVectorAdd(dir, Wind)), ElapsedTime));
                if (XMVectorGetY(pos) < 0)
                {
                    pos = XMVectorSetY(pos, 0);
                }
                dir = XMVectorAdd(dir, XMVectorScale(Gravity, ElapsedTime));

                float Drag = 8.0f * SpeedLerp;
                dir = XMVectorScale(dir, 1.0f - Drag * ElapsedTime);

                XMStoreFloat3(&m_particles[i].Pos, pos);
                XMStoreFloat3(&m_particles[i].Dir, dir);

                m_particles[i].Radius += SizeLerp * ElapsedTime;

                m_particles[i].Life += ElapsedTime;
                m_particles[i].Fade = Fade;

                m_particles[i].Rot += Rot;

                m_particles[i].IsVisible = TRUE;
            }

            if (!m_hasStarted)
            {
                m_hasStarted = TRUE;
            }
        }
        else
        {
            for (int i = 0; i < m_numParticles; i++)
            {
                m_particles[i].IsVisible = FALSE;
                m_particles[i].Life += ElapsedTime;
            }
        }

        m_currentTime += ElapsedTime;
    }

    ParticleWorld::ParticleWorld()
        : m_isRenderingDeferred(true)
        , m_numParticlesToDraw(0)
        , m_numUsedParticles(0)
        , m_numActiveParticles(0)
        , m_particleConstants{}
        , m_glowConstants{}
        , m_vbView{}
        , m_glowLightsCBAddr(0)
    { }

    void ParticleWorld::CreateParticleArray(int MaxParticles)
    {
        // None are used at the beginning...
        m_numUsedParticles = 0;

        // Create the particle array
        m_particleArray.reset(new Particle[size_t(MaxParticles)]);

        // Create the index array.
        m_particleIndices.reset(new int[size_t(MaxParticles)]);

        // Create the depth array
        m_particleDepths.reset(new float[size_t(MaxParticles)]);
    }

    void ParticleWorld::DestroyParticleArray()
    {
        m_numUsedParticles = 0;

        m_particleArray.reset();
        m_particleIndices.reset();
        m_particleDepths.reset();
    }

    void ParticleWorld::SortParticles(FXMVECTOR EyePosition)
    {
        // Calculate the camera-distance for each particle.
        for (int i = 0; i < m_numUsedParticles; i++)
        {
            m_particleIndices[size_t(i)] = i;
            XMVECTOR Delta = XMVectorSubtract(EyePosition, XMLoadFloat3(&m_particleArray[size_t(i)].Pos));
            m_particleDepths[size_t(i)] = XMVectorGetX(XMVector3LengthSq(Delta));
        }

        // Sort based on distance from camera.
        QuickDepthSort(m_particleIndices.get(), m_particleDepths.get(), 0, m_numUsedParticles - 1);
    }

    void ParticleWorld::CopyParticlesToVertexBuffer(ParticleVertex * pVB, FXMVECTOR EyePosition, FXMVECTOR Right, FXMVECTOR Up)
    {
        SortParticles(EyePosition);

        static const XMFLOAT2 Quad[4] =
        {
            XMFLOAT2(-1, -1),
            XMFLOAT2(1, -1),
            XMFLOAT2(1,  1),
            XMFLOAT2(-1,  1)
        };

        m_numActiveParticles = 0;
        int iVBIndex = 0;

        for (int i = m_numUsedParticles - 1; i >= 0; i--)
        {
            auto index = static_cast<size_t>(m_particleIndices[size_t(i)]);

            // Ignore particles that are not visible.
            if (!m_particleArray[index].IsVisible)
                continue;

            XMVECTOR Pos = XMLoadFloat3(&m_particleArray[index].Pos);
            float Radius = m_particleArray[index].Radius;
            float Rot = m_particleArray[index].Rot;
            float Fade = m_particleArray[index].Fade;
            DWORD Color = m_particleArray[index].Color;

            // rotate
            float SinTheta = sinf(Rot);
            float CosTheta = cosf(Rot);

            XMFLOAT2 New[4];
            for (int v = 0; v < 4; v++)
            {
                New[v].x = CosTheta * Quad[v].x - SinTheta * Quad[v].y;
                New[v].y = SinTheta * Quad[v].x + CosTheta * Quad[v].y;

                New[v].x *= Radius;
                New[v].y *= Radius;
            }

            // Tri 0 (0,1,3)
            XMVECTOR NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[0].x), XMVectorScale(Up, New[0].y)));
            pVB[iVBIndex + 2].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 2].UV = XMFLOAT2(0, 1);
            pVB[iVBIndex + 2].Life = Fade;
            pVB[iVBIndex + 2].Rot = Rot;
            pVB[iVBIndex + 2].Color = Color;
            NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[1].x), XMVectorScale(Up, New[1].y)));
            pVB[iVBIndex + 1].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 1].UV = XMFLOAT2(1, 1);
            pVB[iVBIndex + 1].Life = Fade;
            pVB[iVBIndex + 1].Rot = Rot;
            pVB[iVBIndex + 1].Color = Color;
            NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[3].x), XMVectorScale(Up, New[3].y)));
            pVB[iVBIndex + 0].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 0].UV = XMFLOAT2(0, 0);
            pVB[iVBIndex + 0].Life = Fade;
            pVB[iVBIndex + 0].Rot = Rot;
            pVB[iVBIndex + 0].Color = Color;

            // Tri 1 (3,1,2)
            NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[3].x), XMVectorScale(Up, New[3].y)));
            pVB[iVBIndex + 5].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 5].UV = XMFLOAT2(0, 0);
            pVB[iVBIndex + 5].Life = Fade;
            pVB[iVBIndex + 5].Rot = Rot;
            pVB[iVBIndex + 5].Color = Color;
            NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[1].x), XMVectorScale(Up, New[1].y)));
            pVB[iVBIndex + 4].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 4].UV = XMFLOAT2(1, 1);
            pVB[iVBIndex + 4].Life = Fade;
            pVB[iVBIndex + 4].Rot = Rot;
            pVB[iVBIndex + 4].Color = Color;
            NewPos = XMVectorAdd(Pos, XMVectorAdd(XMVectorScale(Right, New[2].x), XMVectorScale(Up, New[2].y)));
            pVB[iVBIndex + 3].Pos = XMFLOAT3(XMVectorGetX(NewPos), XMVectorGetY(NewPos), XMVectorGetZ(NewPos));
            pVB[iVBIndex + 3].UV = XMFLOAT2(1, 0);
            pVB[iVBIndex + 3].Life = Fade;
            pVB[iVBIndex + 3].Rot = Rot;
            pVB[iVBIndex + 3].Color = Color;

            iVBIndex += 6;

            // Keep track of how many particles are active.
            ++m_numActiveParticles;
        }
    }

    void ParticleWorld::CreateDeviceDependentResources(DX::DeviceResources* deviceResources, ResourceUploadBatch& resourceUpload, const Model& terrainModel)
    {
        auto device = deviceResources->GetD3DDevice();

        BuildTerrainPositionArray(terrainModel);

        // Calculate the maximum possible number of particles in the scene, and allocate based on this value.
        int MaxParticles = g_MaxMushroomClouds * (g_NumStalkParticles + g_NumMushroomParticles) + g_MaxGroundBursts * g_NumGroundBurstParticles;

        CreateParticleArray(MaxParticles);

        XMVECTOR Color0 = XMVectorSet(1.0f, 1.0f, 1.0f, 1);
        XMVECTOR Color1 = XMVectorSet(0.6f, 0.6f, 0.6f, 1);
        XMVECTOR PosMul = XMVectorSet(1, 1, 1, 0);
        XMVECTOR DirMul = XMVectorSet(1, 1, 1, 0);

        m_numParticlesToDraw = 0;

        // Create initial particles for all our mushroom clouds.
        // There's two seperate systems here - one simulating the "mushroom top" part of the cloud, the other
        // simulating the stalk.
        for (int i = 0; i < g_MaxMushroomClouds; i += 2)
        {
            auto AttachVertex = static_cast<size_t>(rand() % int(m_terrainVertexPositions.size()));
            XMVECTOR Location = XMVectorScale(XMLoadFloat3(&m_terrainVertexPositions[AttachVertex]), 0.1f);
            Location = XMVectorAdd(Location, XMVectorSet(0, 5.0f, 0, 0));

            m_particleSystems[i].reset(new MushroomParticleSystem());
            m_particleSystems[i]->CreateParticleSystem(this, g_NumMushroomParticles);
            XMVECTOR Zero = XMVectorZero();
            m_particleSystems[i]->SetSystemAttributes(Location,
                g_ParticleSpread, g_ParticleMushroomCloudLifeSpan, g_ParticleFadeExponent,
                g_ParticleStartSize, g_ParticleEndSize, g_ParticleSizeExponent,
                g_ParticleMushroomStartSpeed, g_ParticleEndSpeed, g_ParticleSpeedExponent,
                g_ParticleRollAmount, g_ParticleWindFalloff,
                1, 0, Zero, Zero,
                Color0, Color1,
                PosMul, DirMul);

            m_numParticlesToDraw += g_NumMushroomParticles;

            XMVECTOR PosDirMul = XMVectorSet(1.0f, 0.1f, 1.0f, 0.0f);

            m_particleSystems[i + 1].reset(new StalkParticleSystem());
            m_particleSystems[i + 1]->CreateParticleSystem(this, g_NumStalkParticles);
            m_particleSystems[i + 1]->SetSystemAttributes(Location,
                15.0f, g_ParticleMushroomCloudLifeSpan, g_ParticleFadeExponent * 2.0f,
                g_ParticleStartSize * 0.5f, g_ParticleEndSize * 0.5f, g_ParticleSizeExponent,
                g_ParticleStalkStartSpeed, -1.0f, g_ParticleSpeedExponent,
                g_ParticleRollAmount, g_ParticleWindFalloff,
                1, 0, Zero, Zero,
                Color0, Color1,
                PosDirMul, PosDirMul);

            m_numParticlesToDraw += g_NumStalkParticles;
        }

        // Create initial particles for all our ground-burst explosions.
        for (int i = g_MaxMushroomClouds; i < g_MaxGroundBursts; i++)
        {
            auto AttachVertex = static_cast<size_t>(rand() % int(m_terrainVertexPositions.size()));
            XMVECTOR Location = XMVectorScale(XMLoadFloat3(&m_terrainVertexPositions[AttachVertex]), 0.1f);
            Location = XMVectorAdd(Location, XMVectorSet(0, 5.0f, 0, 0));

            m_particleSystems[i].reset(new GroundBurstParticleSystem());

            m_particleSystems[i]->CreateParticleSystem(this, g_NumGroundBurstParticles);
            m_particleSystems[i]->SetSystemAttributes(Location,
                1.0f, g_ParticleGroundBurstLifeSpan, g_ParticleFadeExponent,
                0.5f, 8.0f, 1.0f,
                g_ParticleGroundBurstStartSpeed, g_ParticleEndSpeed, 4.0f,
                g_ParticleRollAmount, 1.0f,
                30, 100.0f, XMVectorSet(0, 0.5f, 0, 0), XMVectorSet(1.0f, 0.5f, 1.0f, 0),
                Color0, Color1,
                PosMul, DirMul);

            m_numParticlesToDraw += g_NumGroundBurstParticles;
        }

        // initialize heaps
        m_srvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 128, HeapCount);
        m_rtvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 128, RTV_HeapCount);

        // load the particle texture
        CreateDDSTextureFromFile(device, resourceUpload, L"DeferredParticle.dds", m_particleTex.ReleaseAndGetAddressOf());
        device->CreateShaderResourceView(m_particleTex.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_ParticleTex));

        {
            // Define the input layout for our particles.
            const D3D12_INPUT_ELEMENT_DESC ParticleILD[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "LIFE",     0, DXGI_FORMAT_R32_FLOAT,       0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "THETA",    0, DXGI_FORMAT_R32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // Create a blend-state for the deferred rendering pass.
            CD3DX12_BLEND_DESC descBlendDeferred(D3D12_DEFAULT);
            descBlendDeferred.RenderTarget[0].BlendEnable = TRUE;
            descBlendDeferred.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            descBlendDeferred.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

            // Create the blend state for the composition pass.
            CD3DX12_BLEND_DESC descBlendComposition(descBlendDeferred);
            descBlendComposition.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            descBlendComposition.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            descBlendComposition.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
            descBlendComposition.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

            // Create the depth-stencil state for the particle rendering passes (forward and deferred).
            CD3DX12_DEPTH_STENCIL_DESC descDepthParticle(D3D12_DEFAULT);
            descDepthParticle.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;     // ...but, don't write anything.

            // Create the depth-stencil state for the composition pass.
            CD3DX12_DEPTH_STENCIL_DESC descDepthComposition(descDepthParticle);
            descDepthComposition.DepthEnable = FALSE;
            descDepthComposition.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

            // Create the forward PSO
            auto particleVS = DX::ReadData(L"ParticleVS.cso");
            auto forwardPS = DX::ReadData(L"ParticleForwardPS.cso");

            // Strip the root signature from one of the shaders (they both leverage the same root signature.)
            DX::ThrowIfFailed(device->CreateRootSignature(
                0,
                particleVS.data(),
                particleVS.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descForwardPSO = {};
            descForwardPSO.InputLayout.pInputElementDescs = ParticleILD;
            descForwardPSO.InputLayout.NumElements = _countof(ParticleILD);
            descForwardPSO.pRootSignature = m_rootSignature.Get();
            descForwardPSO.VS.pShaderBytecode = particleVS.data();
            descForwardPSO.VS.BytecodeLength = particleVS.size();
            descForwardPSO.PS.pShaderBytecode = forwardPS.data();
            descForwardPSO.PS.BytecodeLength = forwardPS.size();
            descForwardPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            descForwardPSO.BlendState = descBlendComposition;
            descForwardPSO.DepthStencilState = descDepthParticle;
            descForwardPSO.SampleMask = UINT_MAX;
            descForwardPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            descForwardPSO.NumRenderTargets = 1;
            descForwardPSO.RTVFormats[0] = deviceResources->GetBackBufferFormat();
            descForwardPSO.DSVFormat = deviceResources->GetDepthBufferFormat();
            descForwardPSO.SampleDesc.Count = 1;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descForwardPSO, IID_GRAPHICS_PPV_ARGS(m_forwardPSO.ReleaseAndGetAddressOf())));
            DX::ThrowIfFailed(m_forwardPSO->SetName(L"ParticleForwardPSO"));

            // Create the deferred PSO
            auto deferredPS = DX::ReadData(L"ParticleDeferredPS.cso");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descDeferredPSO = {};
            memcpy(&descDeferredPSO, &descForwardPSO, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
            descDeferredPSO.PS.pShaderBytecode = deferredPS.data();
            descDeferredPSO.PS.BytecodeLength = deferredPS.size();
            descDeferredPSO.BlendState = descBlendDeferred;
            descDeferredPSO.NumRenderTargets = 2;
            descDeferredPSO.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            descDeferredPSO.RTVFormats[1] = deviceResources->GetBackBufferFormat();
            descDeferredPSO.DSVFormat = deviceResources->GetDepthBufferFormat();
            descDeferredPSO.SampleDesc.Count = 1;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descDeferredPSO, IID_GRAPHICS_PPV_ARGS(m_deferredPSO.ReleaseAndGetAddressOf())));
            DX::ThrowIfFailed(m_deferredPSO->SetName(L"ParticleDeferredPSO"));

            // Create the full-screen quad PSO
            auto fsqVS = DX::ReadData(L"DeferredLightingVS.cso");
            auto fsqPS = DX::ReadData(L"DeferredLightingPS.cso");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descQuadPSO = {};
            memcpy(&descQuadPSO, &descForwardPSO, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
            descQuadPSO.InputLayout.pInputElementDescs = nullptr;
            descQuadPSO.InputLayout.NumElements = 0;
            descQuadPSO.VS.pShaderBytecode = fsqVS.data();
            descQuadPSO.VS.BytecodeLength = fsqVS.size();
            descQuadPSO.PS.pShaderBytecode = fsqPS.data();
            descQuadPSO.PS.BytecodeLength = fsqPS.size();
            descQuadPSO.DepthStencilState = descDepthComposition;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descQuadPSO, IID_GRAPHICS_PPV_ARGS(m_quadPSO.ReleaseAndGetAddressOf())));
            DX::ThrowIfFailed(m_quadPSO->SetName(L"DeferredLightingPSO"));
        }
    }

    void ParticleWorld::CreateWindowSizeDependendentResources(DX::DeviceResources* deviceResources)
    {
        auto device = deviceResources->GetD3DDevice();

        //---------------------------------------------
        // Create the deferred render targets

        // Normal map texture
        const D3D12_VIEWPORT viewport = deviceResources->GetScreenViewport();
        D3D12_RESOURCE_DESC descTex = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            static_cast<UINT64>(viewport.Width),
            static_cast<UINT>(viewport.Height),
            1,
            1,
            1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        const float aClearColor[] = { 0, 0, 0, 0 };
        CD3DX12_CLEAR_VALUE optimizedClearValue(descTex.Format, aClearColor);
        const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &descTex,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &optimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_normalTex.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_normalTex->SetName(L"Normal Texture"));

        device->CreateShaderResourceView(m_normalTex.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_NormalTex));
        device->CreateRenderTargetView(m_normalTex.Get(), nullptr, m_rtvPile->GetCpuHandle(RTV_NormalTex));

        // Diffuse texture
        descTex.Format = deviceResources->GetBackBufferFormat();
        optimizedClearValue.Format = descTex.Format;
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &descTex,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &optimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_diffuseTex.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_diffuseTex->SetName(L"Diffuse Texture"));

        device->CreateShaderResourceView(m_diffuseTex.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_DiffuseTex));
        device->CreateRenderTargetView(m_diffuseTex.Get(), nullptr, m_rtvPile->GetCpuHandle(RTV_DiffuseTex));
    }

    void ParticleWorld::Update(bool IsSuspended, FXMMATRIX view, CXMMATRIX proj, CXMVECTOR camPos)
    {
        static const XMVECTOR WindVel = XMVectorSet(-2.0f, 10.0f, 0, 0);
        static const XMVECTOR Gravity = XMVectorSet(0, -9.8f, 0.0f, 0);

        // Get Right, Up and Look vectors from our camera matrix.
        XMMATRIX viewProj = XMMatrixMultiply(view, proj);

        XMMATRIX CamTranspose = XMMatrixTranspose(view);
        XMVECTOR Right = CamTranspose.r[0];
        XMVECTOR Up = CamTranspose.r[1];
        XMVECTOR Look = CamTranspose.r[2];

        // Update per-frame constant data...
        XMStoreFloat3(&m_particleConstants.LightDirection, XMVector3Normalize(XMVectorSet(1.0f, -0.5f, -1.5f, 0.0f)));
        XMStoreFloat3(&m_particleConstants.Right, Right);
        XMStoreFloat3(&m_particleConstants.Up, Up);
        XMStoreFloat3(&m_particleConstants.Forward, Look);

        // And store the clip and world matrices.
        XMStoreFloat4x4(&m_particleConstants.ViewProj, XMMatrixTranspose(viewProj));

        // Advance all the particle simulations.
        for (int i = 0; i < g_MaxGroundBursts; ++i)
        {
            float AdvanceTime = IsSuspended ? 0.0f : 2.5f / 60.0f;
            m_particleSystems[i]->AdvanceSystem(AdvanceTime, Right, Up, WindVel, Gravity);
        }

        // Copy particle vertex data into our vertex buffer.
        size_t vertSizeBytes = sizeof(ParticleVertex) * 6 * m_numParticlesToDraw;
        auto vertexMem = GraphicsMemory::Get().Allocate(vertSizeBytes, 16, GraphicsMemory::TAG_VERTEX);
        CopyParticlesToVertexBuffer(reinterpret_cast<ParticleVertex*>(vertexMem.Memory()), camPos, XMLoadFloat3(&m_particleConstants.Right), XMLoadFloat3(&m_particleConstants.Up));

        // Set up input data for particle rendering
        m_vbView.BufferLocation = vertexMem.GpuAddress();
        m_vbView.SizeInBytes = sizeof(ParticleVertex) * 6 * m_numParticlesToDraw;
        m_vbView.StrideInBytes = sizeof(ParticleVertex);

        unsigned int NumActiveSystems = 0;

        // Update lifecycle info for our mushroom clouds.
        for (int i = 0; i < g_MaxMushroomClouds; i += 2)
        {
            float CurrentTime = m_particleSystems[i]->GetCurrentTime();
            float LifeSpan = m_particleSystems[i]->GetLifeSpan();

            // If this simulation is expiring...
            if (CurrentTime > LifeSpan)
            {
                // Reset the simulation, and move it to a random location on the terrain.
                // Note, we do this for the stalk and the mushroom head.
                auto AttachVertex = static_cast<size_t>(rand() % int(m_terrainVertexPositions.size()));
                XMVECTOR Center = XMVectorScale(XMLoadFloat3(&m_terrainVertexPositions[AttachVertex]), 0.1f);
                Center = XMVectorAdd(Center, XMVectorSet(0, 5.0f, 0, 0));
                float StartTime = -fabs(RandPercent()) * 4.0f;
                XMVECTOR FlashColor = FLASH_COLORS[rand() % g_MaxFlashColors];

                m_particleSystems[i]->SetCenter(Center);
                m_particleSystems[i]->SetStartTime(StartTime);
                m_particleSystems[i]->SetFlashColor(FlashColor);
                m_particleSystems[i]->Init();

                m_particleSystems[i + 1]->SetCenter(Center);
                m_particleSystems[i + 1]->SetStartTime(StartTime);
                m_particleSystems[i + 1]->SetFlashColor(FlashColor);
                m_particleSystems[i + 1]->Init();
            }
            // Otherwise, if we have slots available in our flash-light array, and the system is early enough in its
            // simulation cycle, set up a "flash light" for this system to illuminate itself and the scene.
            else if (CurrentTime > 0.0f && CurrentTime < g_FlashLife && NumActiveSystems < g_MaxFlashLights)
            {
                XMVECTOR Center = m_particleSystems[i]->GetCenter();
                XMVECTOR FlashColor = m_particleSystems[i]->GetFlashColor();

                float Intensity = g_FlashIntensity * ((g_FlashLife - CurrentTime) / g_FlashLife);
                XMStoreFloat4(&m_glowConstants.GlowLightPosIntensity[NumActiveSystems], XMVectorSet(
                    XMVectorGetX(Center),
                    XMVectorGetY(Center) + g_ParticleLightRaise,
                    XMVectorGetZ(Center),
                    Intensity));
                XMStoreFloat4(&m_glowConstants.GlowLightColor[NumActiveSystems], FlashColor);
                NumActiveSystems++;
            }
        }

        // Update lifecycle info for our ground bursts
        for (int i = g_MaxMushroomClouds; i < g_MaxGroundBursts; i++)
        {
            float CurrentTime = m_particleSystems[i]->GetCurrentTime();
            float LifeSpan = m_particleSystems[i]->GetLifeSpan();

            // If the simulation is expiring...
            if (CurrentTime > LifeSpan)
            {
                // Reset the simulation, and move it to a random location on the terrain.
                auto AttachVertex = static_cast<size_t>(rand() % int(m_terrainVertexPositions.size()));
                XMVECTOR Center = XMVectorScale(XMLoadFloat3(&m_terrainVertexPositions[AttachVertex]), 0.1f);
                Center = XMVectorAdd(Center, XMVectorSet(0, 5.0f, 0, 0));
                float StartTime = -fabs(RandPercent()) * 4.0f;
                XMVECTOR FlashColor = FLASH_COLORS[rand() % g_MaxFlashColors];

                float StartSpeed = g_ParticleGroundBurstStartSpeed + RandPercent() * 30.0f;
                m_particleSystems[i]->SetCenter(Center);
                m_particleSystems[i]->SetStartTime(StartTime);
                m_particleSystems[i]->SetStartSpeed(StartSpeed);
                m_particleSystems[i]->SetFlashColor(FlashColor);
                m_particleSystems[i]->Init();
            }
            // Otherwise, if we have slots available in our flash-light array, and the system is early enough in its
            // simulation cycle, set up a "flash light" for this system to illuminate itself and the scene.
            else if (CurrentTime > 0.0f && CurrentTime < g_FlashLife && NumActiveSystems < g_MaxFlashLights)
            {
                XMVECTOR Center = m_particleSystems[i]->GetCenter();
                XMVECTOR FlashColor = m_particleSystems[i]->GetFlashColor();

                float Intensity = g_FlashIntensity * ((g_FlashLife - CurrentTime) / g_FlashLife);
                XMStoreFloat4(&m_glowConstants.GlowLightPosIntensity[NumActiveSystems], XMVectorSet(
                    XMVectorGetX(Center),
                    XMVectorGetY(Center) + g_ParticleLightRaise,
                    XMVectorGetZ(Center),
                    Intensity));
                XMStoreFloat4(&m_glowConstants.GlowLightColor[NumActiveSystems], FlashColor);
                NumActiveSystems++;
            }
        }

        // Update the remainder of the glow-light constant data
        m_glowConstants.NumGlowLights = NumActiveSystems;
        XMStoreFloat4(&m_glowConstants.MeshLightAttenuation, g_MeshLightAttenuation);
        XMStoreFloat4(&m_glowConstants.GlowLightAttenuation, g_FlashAttenuation);

        m_glowLightsCBAddr = GraphicsMemory::Get().AllocateConstant(m_glowConstants).GpuAddress();
    }

    void ParticleWorld::Render(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE hRT, D3D12_CPU_DESCRIPTOR_HANDLE hDS)
    {
        ScopedPixEvent Particles(commandList, PIX_COLOR_DEFAULT, L"Particles");

        auto frameCBMem = GraphicsMemory::Get().AllocateConstant(m_particleConstants);

        // Set up render state.
        ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        commandList->SetGraphicsRootConstantBufferView(RootParamCBPerFrame, frameCBMem.GpuAddress());
        commandList->SetGraphicsRootConstantBufferView(RootParamCBGlow, m_glowLightsCBAddr);

        commandList->IASetVertexBuffers(0, 1, &m_vbView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetGraphicsRootDescriptorTable(RootParamSRV, m_srvPile->GetGpuHandle(SRV_ParticleTex));

        // Render using currently selected technique.
        if (m_isRenderingDeferred)
        {
            RenderDeferred(commandList, hRT, hDS);
        }
        else
        {
            RenderForward(commandList);
        }
    }

    void ParticleWorld::RenderForward(ID3D12GraphicsCommandList* commandList)
    {
        commandList->SetPipelineState(m_forwardPSO.Get());

        // Render particles
        commandList->DrawInstanced(UINT(m_numActiveParticles * 6), 1, 0, 0);
    }

    void ParticleWorld::RenderDeferred(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE hRT, D3D12_CPU_DESCRIPTOR_HANDLE hDS)
    {
        // transition resources to render target
        TransitionResource(commandList, m_diffuseTex.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        TransitionResource(commandList, m_normalTex.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Clear deferred targets for this frame.
        const float aClearColor[] = { 0, 0, 0, 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE aRTs[] = { m_rtvPile->GetCpuHandle(RTV_NormalTex), m_rtvPile->GetCpuHandle(RTV_DiffuseTex) };
        commandList->ClearRenderTargetView(aRTs[0], aClearColor, 0, nullptr);
        commandList->ClearRenderTargetView(aRTs[1], aClearColor, 0, nullptr);

        // Set the deferred render targets.
        commandList->OMSetRenderTargets(_countof(aRTs), aRTs, true, &hDS);

        commandList->SetPipelineState(m_deferredPSO.Get());

        // Render particles.
        commandList->DrawInstanced(UINT(m_numActiveParticles * 6), 1, 0, 0);

        // Restore scene render target
        commandList->OMSetRenderTargets(1, &hRT, true, &hDS);

        // transition resources to shader resource
        TransitionResource(commandList, m_diffuseTex.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        TransitionResource(commandList, m_normalTex.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->SetPipelineState(m_quadPSO.Get());

        // Use deferred targets as our source textures (shader views).
        commandList->SetGraphicsRootDescriptorTable(RootParamSRV, m_srvPile->GetGpuHandle(SRV_DiffuseTex));

        // Render the full-screen quad.
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void ParticleWorld::BuildTerrainPositionArray(const Model& model)
    {
        assert(!model.meshes.empty());

        auto& mesh = model.meshes[0];
        assert(!mesh->opaqueMeshParts.empty());

        auto& part = mesh->opaqueMeshParts[0];
        assert(part->vertexBuffer.Memory());

        char* mem = (char*)part->vertexBuffer.Memory();
        assert(mem != nullptr);

        uint32_t byteOffset = 0;
        size_t posIndex = 0;
        for (; posIndex < part->vbDecl->size(); ++posIndex)
        {
            auto& attr = part->vbDecl->at(posIndex);

            if (strcmp(attr.SemanticName, "SV_Position") == 0)
            {
                // Determine whether the attribute uses absolute or relative byte offset.
                if (attr.AlignedByteOffset != D3D12_APPEND_ALIGNED_ELEMENT)
                {
                    // If the element uses absolute byte offset
                    byteOffset = attr.AlignedByteOffset;
                }

                break;
            }

            byteOffset += attr.AlignedByteOffset == D3D12_APPEND_ALIGNED_ELEMENT ? GetElementSize(attr.Format) : (attr.AlignedByteOffset - byteOffset);
        }
        assert(posIndex != part->vbDecl->size());

        m_terrainVertexPositions.resize(part->vertexBufferSize / part->vertexStride);

        for (size_t i = 0; i < m_terrainVertexPositions.size(); ++i, mem += part->vertexStride)
        {
            auto position = reinterpret_cast<float*>(mem + byteOffset);

            XMStoreFloat3(&m_terrainVertexPositions[i], XMVectorSet(position[0], position[1], position[2], 0.0f));
        }
    }
}
