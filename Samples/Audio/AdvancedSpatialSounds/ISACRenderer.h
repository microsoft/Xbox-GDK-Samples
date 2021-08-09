//--------------------------------------------------------------------------------------
// ISACRenderer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SpatialAudioClient.h"
#include <mmdeviceapi.h>

#pragma once

    // Primary ISAC Renderering Class
    class ISACRenderer
    {
    public:
        enum RenderState {
            Inactive = 0,
            Active,
            Resetting
        };

        ISACRenderer();

        HRESULT			InitializeAudioDevice();
        bool            IsActive() const { return m_ISACrenderstate == RenderState::Active; };
        bool            IsResetting() const { return m_ISACrenderstate == RenderState::Resetting; }
        void			Reset() { m_ISACrenderstate = RenderState::Resetting; }
        UINT32	        GetMaxDynamicObjects() const { return m_MaxDynamicObjects; }
        Microsoft::WRL::ComPtr<ISpatialAudioObjectRenderStream>		m_SpatialAudioStream;

        HANDLE m_bufferCompletionEvent;
    private:
        ~ISACRenderer();

        RenderState				m_ISACrenderstate;
        UINT32					m_MaxDynamicObjects;

        Microsoft::WRL::ComPtr<ISpatialAudioClient>					m_SpatialAudioClient;
    };



