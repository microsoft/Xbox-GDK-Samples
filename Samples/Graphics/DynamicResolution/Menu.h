//--------------------------------------------------------------------------------------
// Menu.h
//
// A menu UI implementation, used to control the simulated CPU/GPU spikes in this sample.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "ControllerFont.h"
#include "Utility.h"

namespace
{
    using namespace DirectX;
    
    // List of pre-chosen combinations of options. 
    // Add more here, and in Menu::m_presets.
    enum PRESET : uint32_t
    {
        PRESET_GPU_SPIKE_VSYNC_THRESHOLD,
        PRESET_GPU_SPIKE_VSYNC_LOCK,
        PRESET_GPU_DROP_VSYNC_THRESHOLD,
        PRESET_GPU_DROP_VSYNC_LOCK,
        PRESET_CPU_DROP,

        PRESET_COUNT
    };
    static const char* const c_presetNames[] =
    {
        "Preset: GPU Spike (Tearing)",
        "Preset: GPU Spike (VSync)",
        "Preset: GPU Drop (Tearing)",
        "Preset: GPU Drop (VSync)",
        "Preset: CPU Drop",
    };
    static_assert(PRESET_COUNT == _countof(c_presetNames), "Mismatch between enum and reflected names");
    
    // Base class for various types of menu items
    class UIParam
    {
    public:
	    static constexpr XMVECTORF32 c_activeParamColor = {{{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }}};
	    static constexpr XMVECTORF32 c_inactiveParamColor = {{{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }}};
	    static constexpr XMVECTORF32 c_activeOptionColor = {{{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }}};
	    static constexpr XMVECTORF32 c_inactiveOptionColor = {{{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }}};
	    static constexpr XMVECTORF32 c_invalidOptionColor = {{{ 1.000000000f, 0.000000000f, 0.000000000f, 1.000000000f }}};
	    static constexpr XMVECTORF32 c_unusedOptionColor = {{{ 0.501960814f, 0.501960814f, 0.501960814f, 1.000000000f }}};
	    static constexpr float c_activeParamScale = 1.0f;
	    static constexpr float c_inactiveParamScale = 1.0f;
	    static constexpr float c_optionX = 360.0f;
        static constexpr float c_arrowsDeltaY = 5.0f;
        static constexpr float c_leftArrowDeltaX = -30.0f;
        static constexpr float c_rightArrowDeltaX = 70.0f;

        UIParam(const wchar_t* paramName = nullptr) : m_paramName(paramName), m_status(UIParam::STATUS_DEFAULT)
        {}

        virtual ~UIParam() {}

        virtual void RenderOptionUI(SpriteBatch* batch, SpriteFont* font, XMFLOAT2 position, bool isActive) = 0;

        void RenderUI(SpriteBatch* batch, SpriteFont* font, XMFLOAT2 position, bool isActive)
        {
            // Probably an uninitialized class instance if this triggers
            assert(m_paramName);

            if (isActive)
            {
                font->DrawString(batch, m_paramName, position, c_activeParamColor, 0.0f, { 0.0f, 0.0f }, c_activeParamScale);
            }
            else
            {
                font->DrawString(batch, m_paramName, position, c_inactiveParamColor, 0.0f, { 0.0f, 0.0f }, c_inactiveParamScale);
            }
            RenderOptionUI(batch, font, position, isActive);
        }

        virtual void        DecreaseValue() = 0;
        virtual void        IncreaseValue() = 0;

        enum : uint32_t
        {
            STATUS_DEFAULT,         // Value is okay
            STATUS_INVALID,         // Value is bad
            STATUS_UNUSED,          // Value is irrelevant

            STATUS_COUNT
        };
        uint32_t            GetStatus() { return m_status; }
        void                SetStatus(uint32_t iStatus) { m_status = iStatus; }

    protected:
        const wchar_t*      m_paramName;
        uint32_t            m_status;
    };

    template< typename valType >
    class UIParamTyped : public UIParam
    {
    public:
        UIParamTyped(const wchar_t* paramName, valType value = 0, valType step = 1)
            : UIParam(paramName)
            , m_value(value)
            , m_step(step)
        {}

        virtual const wchar_t* GetOptionText(wchar_t* buffer, uint32_t count) const = 0;

        void RenderOptionUI(SpriteBatch* batch, SpriteFont* font, XMFLOAT2 position, bool isActive)
        {
            XMVECTOR optionTextColor[2][UIParam::STATUS_COUNT] =
            {
                {
	                c_inactiveOptionColor, 
	                c_invalidOptionColor, 
	                c_unusedOptionColor, 

                },
                {
	                c_activeOptionColor, 
	                c_invalidOptionColor, 
	                c_unusedOptionColor, 

                },
            };
            wchar_t optionTextBuffer[256] = {};
            const wchar_t* optionText = GetOptionText(optionTextBuffer, _countof(optionTextBuffer));
            const wchar_t* formatText[UIParam::STATUS_COUNT] =
            {
                L"%s",
                L"%s (invalid)",
                L"n/a",
            };
            wchar_t selectText[256];
            swprintf_s(selectText, formatText[m_status], optionText);
            position.x += c_optionX;
            font->DrawString(batch, selectText, position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, isActive ? c_activeParamScale : c_inactiveParamScale);
            if (isActive)
            {
                position.y -= c_arrowsDeltaY;
                position.x += c_leftArrowDeltaX;
                font->DrawString(batch, L"<", position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, 1.2f);
                position.x = position.x - c_leftArrowDeltaX + c_rightArrowDeltaX;
                font->DrawString(batch, L">", position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, 1.2f);
            }
        }

        valType             GetValue() const { return m_value; };
        void                SetValue(valType value) { m_value = value; };
        virtual void        DecreaseValue() { m_value -= m_step; }
        virtual void        IncreaseValue() { m_value += m_step; }

    protected:
        valType              m_value;
        valType              m_step;
    };

    class UIParamInt : public UIParamTyped< int >
    {
    public:
        UIParamInt(const wchar_t* paramName, int min, int max, int value = 0, int step = 1, uint32_t precision = 2)
            : UIParamTyped< int >(paramName, value, step)
            , m_min(min)
            , m_max(max)
            , m_precision(precision) {}

        virtual const wchar_t* GetOptionText(wchar_t* buffer, uint32_t count) const
        {
            swprintf_s(buffer, count, L"%*u", m_precision, GetValue());
            return buffer;
        }

        virtual void        DecreaseValue() { m_value = std::max(m_min, m_value - m_step); }
        virtual void        IncreaseValue() { m_value = std::min(m_max, m_value + m_step); }

    protected:
        int m_min;
        int m_max;

    private:
        uint32_t m_precision;
    };

    template< typename valType >
    class UIParamCountable : public UIParamTyped< valType >
    {
    public:
        UIParamCountable(const wchar_t* paramName, const wchar_t* const* optionNames, uint32_t count, uint32_t value = 0, uint32_t step = 1)
            : UIParamTyped<valType>(paramName, value, step)
            , m_optionNames(optionNames)
            , m_count(count)
        {}

        virtual const wchar_t* GetOptionText(wchar_t*, uint32_t) const
        {
            return m_optionNames[UIParamTyped<valType>::GetValue()];
        }

        virtual void        DecreaseValue() { UIParamTyped<valType>::m_value += m_count - UIParamTyped<valType>::m_step;    UIParamTyped<valType>::m_value %= m_count; }
        virtual void        IncreaseValue() { UIParamTyped<valType>::m_value += UIParamTyped<valType>::m_step;              UIParamTyped<valType>::m_value %= m_count; }

    protected:
        const wchar_t* const*   m_optionNames;
        uint32_t                m_count;
    };

    typedef UIParamCountable< uint32_t > UIParamEnum;

#pragma warning(push)
#pragma warning(disable: 4804)  // unsafe use of type 'bool' in operation
    class UIParamBool : public UIParamCountable< bool >
    {
        const wchar_t* c_optionNames[2] = { L"FALSE", L"TRUE" };

    public:
        UIParamBool(const wchar_t* paramName, bool isValue = false)
            : UIParamCountable< bool >(paramName, c_optionNames, 2, (uint32_t)isValue)
        {}
    };
#pragma warning(pop)

    class UIParamFloat : public UIParamTyped< float >
    {
    public:
        UIParamFloat(const wchar_t* paramName, float value = 0, float step = 1, uint32_t precision = 2)
            : UIParamTyped< float >(paramName, value, step)
            , m_precision(precision)
        {}

        virtual const wchar_t* GetOptionText(wchar_t* buffer, uint32_t count) const
        {
            swprintf_s(buffer, count, L"%*.*f", m_precision, m_precision, GetValue());
            return buffer;
        }

    private:
        uint32_t m_precision;
    };

    //-------------------------------------------------------------------------------------------------------------------------------------
    // Supported tweakable UI parameters.     
    //-------------------------------------------------------------------------------------------------------------------------------------
    enum UI_PARAM : uint32_t
    {
        UI_PARAM_PRESENTATION_THRESHOLD,
        UI_PARAM_CPU_LOAD,
        UI_PARAM_GPU_LOAD,
        UI_PARAM_SPIKE_PROCESSOR,
        UI_PARAM_SPIKE_AMPLITUDE,
        UI_PARAM_SPIKE_PERIOD,
        UI_PARAM_SPIKE_DURATION,
        UI_PARAM_DROP_RESOLUTION_MARGIN,
        UI_PARAM_RAISE_RESOLUTION_MARGIN,
        UI_PARAM_RESOLUTION_HYSTERESIS,

        UI_PARAM_COUNT
    };

    struct Menu
    {
        Menu()
            : m_useBigMenu(true)
            , m_processInput(true)
            , m_activeUIParameter(0)
            , m_visibleUIStart(0)
            , m_visibleUICount(UI_PARAM_COUNT)
        {
            m_params[UI_PARAM_PRESENTATION_THRESHOLD] = new UIParamInt(L"Present threshold", 0, 100, 100, 10, 3);
            m_params[UI_PARAM_CPU_LOAD] = new UIParamFloat(L"CPU load (ms)", 16.0f, 0.1f);
            m_params[UI_PARAM_GPU_LOAD] = new UIParamFloat(L"GPU load at full res (ms)", 14.5f, 0.1f);
            m_params[UI_PARAM_SPIKE_PROCESSOR] = new UIParamEnum(L"Spike on", c_spikeProcessorNames, _countof(c_spikeProcessorNames), static_cast<uint32_t>(SpikeProcessor::Gpu));
            m_params[UI_PARAM_SPIKE_AMPLITUDE] = new UIParamFloat(L"Spike amplitude (ms)", 1.0f, 1.0f);
            m_params[UI_PARAM_SPIKE_PERIOD] = new UIParamFloat(L"Spike period (s)", 5.0f, 5.0f);
            m_params[UI_PARAM_SPIKE_DURATION] = new UIParamFloat(L"Spike duration (s)", 0.5f, 0.5f);
            m_params[UI_PARAM_DROP_RESOLUTION_MARGIN] = new UIParamFloat(L"Drop res margin (ms)", 0.0f, 0.5f);
            m_params[UI_PARAM_RAISE_RESOLUTION_MARGIN] = new UIParamFloat(L"Raise res margin (ms)", 0.0f, 0.5f);
            m_params[UI_PARAM_RESOLUTION_HYSTERESIS] = new UIParamFloat(L"Raise res wait (s)", 1.0f, 0.5f);

            m_currentPreset = PRESET_GPU_SPIKE_VSYNC_THRESHOLD;

            ZeroMemory(m_wasModified, sizeof(m_wasModified));
            ApplyPreset(m_currentPreset);
        }

        virtual ~Menu()
        {
            for (auto param = 0U; param < UI_PARAM_COUNT; ++param)
            {
                delete m_params[param];
            }
        }

        template< typename valType >
        inline valType GetItemValue(uint32_t itemNum) const
        {
            return dynamic_cast<const UIParamTyped< valType >*>(m_params[itemNum])->GetValue();
        }
        template< typename valType >
        inline void SetItemValue(uint32_t itemNum, valType value)
        {
            m_wasModified[itemNum] = m_wasModified[itemNum] || (value != GetItemValue< valType >(itemNum));
            dynamic_cast<UIParamTyped< valType >*>(m_params[itemNum])->SetValue(value);
        }
        inline uint32_t GetItemStatus(uint32_t itemNum)
        {
            return m_params[itemNum]->GetStatus();
        }
        inline void SetItemStatus(uint32_t itemNum, uint32_t status)
        {
            m_wasModified[itemNum] = m_wasModified[itemNum] || (status != GetItemStatus(itemNum));
            m_params[itemNum]->SetStatus(status);
        }
        inline void DecreaseItemValue(uint32_t iItem)
        {
            m_wasModified[iItem] = true;
            m_params[iItem]->DecreaseValue();
        }
        inline void IncreaseItemValue(uint32_t itemNum)
        {
            m_wasModified[itemNum] = true;
            m_params[itemNum]->IncreaseValue();
        }
        inline bool GetItemModified(uint32_t itemNum)
        {
            return m_wasModified[itemNum];
        }

        void Update(const DirectX::GamePad::ButtonStateTracker& input)
        {
            if (m_processInput)
            {
                using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

                ZeroMemory(m_wasModified, sizeof(m_wasModified));

                float decreaseAmount = 0.0f;
                float increaseAmount = 0.0f;

                // Process UI Input
                if (input.dpadUp == ButtonState::PRESSED || input.leftStickUp == ButtonState::PRESSED)
                {
                    m_activeUIParameter += UI_PARAM_COUNT - 1;
                    m_activeUIParameter %= UI_PARAM_COUNT;
                }
                if (input.dpadDown == ButtonState::PRESSED || input.leftStickDown == ButtonState::PRESSED)
                {
                    m_activeUIParameter += 1;
                    m_activeUIParameter %= UI_PARAM_COUNT;
                }

                // Keep the selected parameter in view, in the small menu
                if (m_visibleUICount < UI_PARAM_COUNT)
                {
                    if ((m_activeUIParameter + 1) % UI_PARAM_COUNT == m_visibleUIStart)
                    {
                        m_visibleUIStart += UI_PARAM_COUNT - 1;
                        m_visibleUIStart %= UI_PARAM_COUNT;
                    }
                    else if ((m_visibleUIStart + m_visibleUICount) % UI_PARAM_COUNT == m_activeUIParameter)
                    {
                        m_visibleUIStart += 1;
                        m_visibleUIStart %= UI_PARAM_COUNT;
                    }
                }

                if (input.dpadLeft == ButtonState::PRESSED || input.leftStickLeft == ButtonState::PRESSED)
                    decreaseAmount = 1.0f;
                if (input.dpadRight == ButtonState::PRESSED || input.leftStickRight == ButtonState::PRESSED)
                    increaseAmount = 1.0f;

                if (decreaseAmount > 0.0f || increaseAmount > 0.0f)
                {
                    if (UIParam::STATUS_UNUSED != GetItemStatus(m_activeUIParameter))
                    {
                        if (decreaseAmount > 0.0f)
                        {
                            DecreaseItemValue(m_activeUIParameter);
                        }
                        if (increaseAmount > 0.0f)
                        {
                            IncreaseItemValue(m_activeUIParameter);
                        }
                    }

                    // If other items are changed, disable preset
                    if (Preset::c_mask & (1 << m_activeUIParameter))
                    {
                        m_currentPreset = -1;
                    }
                }

                // Apply preset if changed
                int presetChange = 0;
                if (input.leftShoulder == ButtonState::PRESSED)
                    presetChange = -1;
                if (input.rightShoulder == ButtonState::PRESSED)
                    presetChange = 1;

                if (presetChange != 0)
                {
                    if (m_currentPreset <= 0 && presetChange < 0)
                        m_currentPreset = PRESET_COUNT - 1;
                    else
                        m_currentPreset = static_cast<int>((m_currentPreset + presetChange) % PRESET_COUNT);

                    ApplyPreset(m_currentPreset);
                }
            }
        }

        float Render(SpriteBatch* batch, SpriteFont* font, SpriteFont* controllerFont, XMFLOAT2 position)
        {
            float paramYInc = 30.0f;

            bool useBigMenu = m_useBigMenu || UI_PARAM_COUNT <= m_visibleUICount;

            uint32_t visibleUIStartIndex = useBigMenu ? 0 : m_visibleUIStart;
            uint32_t visibleUICount = useBigMenu ? UI_PARAM_COUNT : m_visibleUICount;

            if (!useBigMenu)
            {
                position.x += 40.0f;
                font->DrawString(batch, L"^", position, Colors::Magenta, 0.0f, { 0.0f, 0.0f }, 0.8f);
                position.x += 100.0f;
                DX::DrawControllerString(batch, font, controllerFont, L"( [B] to expand )", position, Colors::White, 0.8f);
                position.x -= 140.0f;
                position.y += paramYInc;
            }

            for (uint32_t i = 0; i < visibleUICount; ++i)
            {
                uint32_t paramIndex = (visibleUIStartIndex + i) % UI_PARAM_COUNT;

                UIParam* param = m_params[paramIndex];

                param->RenderUI(batch, font, position, (paramIndex == m_activeUIParameter));
                position.y += paramYInc;
            }

            if (!useBigMenu)
            {
                font->DrawString(batch, L"v", position, Colors::Magenta, 0.0f, { 0.0f, 0.0f }, 0.8f);
                position.y += paramYInc;
            }

            return position.y;
        }

        struct Preset
        {
            int m_presentationThreshold;
            float m_cpuLoad;
            float m_gpuLoad;
            SpikeProcessor m_spikeProcessor;
            float m_spikeAmplitude;
            float m_spikePeriod;
            float m_spikeDuration;
            float m_dropResolutionMargin;
            float m_raiseResolutionMargin;
            float m_raiseResolutionWait;

            static constexpr uint32_t c_mask =
                (1 << UI_PARAM_PRESENTATION_THRESHOLD)
                | (1 << UI_PARAM_CPU_LOAD)
                | (1 << UI_PARAM_GPU_LOAD)
                | (1 << UI_PARAM_SPIKE_PROCESSOR)
                | (1 << UI_PARAM_SPIKE_AMPLITUDE)
                | (1 << UI_PARAM_SPIKE_PERIOD)
                | (1 << UI_PARAM_SPIKE_DURATION)
                | (1 << UI_PARAM_DROP_RESOLUTION_MARGIN)
                | (1 << UI_PARAM_RAISE_RESOLUTION_MARGIN)
                | (1 << UI_PARAM_RESOLUTION_HYSTERESIS);
        };

        void ApplyPreset(int presetNum)
        {
            assert(presetNum >= 0);  // Negative numbers indicate custom values, no preset

            const Preset* preset = &m_presets[presetNum];

            SetItemValue< int >(UI_PARAM_PRESENTATION_THRESHOLD, preset->m_presentationThreshold);
            SetItemValue< float >(UI_PARAM_CPU_LOAD, preset->m_cpuLoad);
            SetItemValue< float >(UI_PARAM_GPU_LOAD, preset->m_gpuLoad);
            SetItemValue< uint32_t >(UI_PARAM_SPIKE_PROCESSOR, static_cast<uint32_t>(preset->m_spikeProcessor));
            SetItemValue< float >(UI_PARAM_SPIKE_AMPLITUDE, preset->m_spikeAmplitude);
            SetItemValue< float >(UI_PARAM_SPIKE_PERIOD, preset->m_spikePeriod);
            SetItemValue< float >(UI_PARAM_SPIKE_DURATION, preset->m_spikeDuration);
            SetItemValue< float >(UI_PARAM_DROP_RESOLUTION_MARGIN, preset->m_dropResolutionMargin);
            SetItemValue< float >(UI_PARAM_RAISE_RESOLUTION_MARGIN, preset->m_raiseResolutionMargin);
            SetItemValue< float >(UI_PARAM_RESOLUTION_HYSTERESIS, preset->m_raiseResolutionWait);
        }

        const char* GetPresetText()
        {
            if (m_currentPreset < 0)
            {
                return "Preset: Custom";
            }

            return c_presetNames[m_currentPreset];
        }

        UIParam*                    m_params[UI_PARAM_COUNT];
        bool                        m_wasModified[UI_PARAM_COUNT];
        bool                        m_useBigMenu;
        bool                        m_processInput;
        uint32_t                    m_activeUIParameter;   // Which UI item is affected by l/r input
        uint32_t                    m_visibleUIStart;
        uint32_t                    m_visibleUICount;

        static const Preset         m_presets[PRESET_COUNT];
        int                         m_currentPreset;    // Current preset, or -1 if custom
    };

    const Menu::Preset Menu::m_presets[] =
    {
        // PRESET_GPU_SPIKE_VSYNC_THRESHOLD, 
        {
            100,                    // uint32_t m_presentationThreshold;
            16.0f,                  // float m_cpuLoad;
            16.0f,                  // float m_gpuLoad;
            SpikeProcessor::Gpu,    // uint32_t m_spikeProcessor;
            2.0f,                   // float m_spikeAmplitude;
            5.0f,                   // float m_spikePeriod;
            0.5f,                   // float m_spikeDuration;
            3.0f,                   // float m_dropResolutionMargin;
            0.0f,                   // float m_raiseResolutionMargin;
            1.0f,                   // float m_raiseResolutionWait;
        },
        // PRESET_GPU_SPIKE_VSYNC_LOCK, 
        {
            0,                      // uint32_t m_presentationThreshold;
            16.0f,                  // float m_cpuLoad;
            16.0f,                  // float m_gpuLoad;
            SpikeProcessor::Gpu,    // uint32_t m_spikeProcessor;
            2.0f,                   // float m_spikeAmplitude;
            5.0f,                   // float m_spikePeriod;
            0.5f,                   // float m_spikeDuration;
            0.0f,                   // float m_dropResolutionMargin;
            0.0f,                   // float m_raiseResolutionMargin;
            1.0f,                   // float m_raiseResolutionWait;
        },
        // PRESET_GPU_DROP_VSYNC_THRESHOLD, 
        {
            100,                    // uint32_t m_presentationThreshold;
            16.0f,                  // float m_cpuLoad;
            17.0f,                  // float m_gpuLoad;
            SpikeProcessor::None,   // uint32_t m_spikeProcessor;
            0.0f,                   // float m_spikeAmplitude;
            5.0f,                   // float m_spikePeriod;
            0.0f,                   // float m_spikeDuration;
            0.0f,                   // float m_dropResolutionMargin;
            0.0f,                   // float m_raiseResolutionMargin;
            1.0f,                   // float m_raiseResolutionWait;
        },
        // PRESET_GPU_DROP_VSYNC_LOCK, 
        {
            0,                      // uint32_t m_presentationThreshold;
            16.0f,                  // float m_cpuLoad;
            17.0f,                  // float m_gpuLoad;
            SpikeProcessor::None,   // uint32_t m_spikeProcessor;
            0.0f,                   // float m_spikeAmplitude;
            5.0f,                   // float m_spikePeriod;
            0.0f,                   // float m_spikeDuration;
            0.0f,                   // float m_dropResolutionMargin;
            0.0f,                   // float m_raiseResolutionMargin;
            1.0f,                   // float m_raiseResolutionWait;
        },
        // PRESET_CPU_DROP, 
        {
            100,                    // uint32_t m_presentationThreshold;
            18.0f,                  // float m_cpuLoad;
            14.5f,                  // float m_gpuLoad;
            SpikeProcessor::None,   // uint32_t m_spikeProcessor;
            0.0f,                   // float m_spikeAmplitude;
            5.0f,                   // float m_spikePeriod;
            0.0f,                   // float m_spikeDuration;
            0.0f,                   // float m_dropResolutionMargin;
            0.0f,                   // float m_raiseResolutionMargin;
            1.0f,                   // float m_raiseResolutionWait;
        },
    };
}
