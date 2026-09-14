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

namespace FramePacingUtils
{
    // Base class for various types of menu items
    class UIParam
    {
    public:
        static constexpr DirectX::XMVECTORF32 c_activeParamColor = {{{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }}};
        static constexpr DirectX::XMVECTORF32 c_inactiveParamColor = {{{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }}};
        static constexpr DirectX::XMVECTORF32 c_activeOptionColor = {{{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }}};
        static constexpr DirectX::XMVECTORF32 c_inactiveOptionColor = {{{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }}};
        static constexpr DirectX::XMVECTORF32 c_readonlyOptionColor = {{{ 0.501960814f, 0.501960814f, 0.501960814f, 1.0f }}};
        static constexpr float c_activeParamScale = 1.0f;
        static constexpr float c_inactiveParamScale = 1.0f;
        static constexpr float c_optionX = 360.0f;
        static constexpr float c_arrowsDeltaY = 5.0f;
        static constexpr float c_leftArrowDeltaX = -45.0f;
        static constexpr float c_rightArrowDeltaX = 155.0f;

        UIParam(const wchar_t* paramName = nullptr) : m_paramName(paramName), m_status(UIParam::STATUS_DEFAULT)
        {}

        virtual ~UIParam() = default;

        virtual void RenderOptionUI(DirectX::SpriteBatch* batch, DirectX::SpriteFont* font, DirectX::XMFLOAT2 position, bool isActive) = 0;

        void RenderUI(DirectX::SpriteBatch* batch, DirectX::SpriteFont* font, DirectX::XMFLOAT2 position, bool isActive)
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
            STATUS_DEFAULT,         // Value is user-controlled
            STATUS_AUTO,        // Value is not user-controlled

            STATUS_COUNT
        };
        uint32_t            GetStatus() const { return m_status; }
        void                SetStatus(uint32_t status) { m_status = status; }

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

        void RenderOptionUI(DirectX::SpriteBatch* batch, DirectX::SpriteFont* font, DirectX::XMFLOAT2 position, bool isActive) override
        {
            DirectX::XMVECTOR optionTextColor[2][STATUS_COUNT] =
            {
                {
                    c_inactiveOptionColor,
                    c_readonlyOptionColor,

                },
                {
                    c_activeOptionColor,
                    c_readonlyOptionColor,

                },
            };
            wchar_t optionTextBuffer[256] = {};
            const wchar_t* optionText = GetOptionText(optionTextBuffer, _countof(optionTextBuffer));
            const wchar_t* formatText[UIParam::STATUS_COUNT] =
            {
                L"%ls",
                L"%ls",
            };
            wchar_t selectText[256];
            swprintf_s(selectText, formatText[m_status], optionText);
            position.x += c_optionX;
            font->DrawString(batch, selectText, position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, isActive ? c_activeParamScale : c_inactiveParamScale);
            if (isActive)
            {
                position.y -= c_arrowsDeltaY;
                position.x += c_leftArrowDeltaX;
                font->DrawString(batch, (STATUS_AUTO == m_status) ? L"[" :L"<", position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, 1.2f);
                position.x = position.x - c_leftArrowDeltaX + c_rightArrowDeltaX;
                font->DrawString(batch, (STATUS_AUTO == m_status) ? L"]" : L">", position, optionTextColor[isActive][m_status], 0.0f, { 0.0f, 0.0f }, 1.2f);
            }
        }

        valType             GetValue() const { return m_value; };
        void                SetValue(valType value) { m_value = value; };
        void                DecreaseValue() override { m_value -= m_step; }
        void                IncreaseValue() override { m_value += m_step; }

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

        const wchar_t* GetOptionText(wchar_t* buffer, uint32_t count) const override
        {
            swprintf_s(buffer, count, L"%*d", m_precision, GetValue());
            return buffer;
        }

        void        DecreaseValue() override { m_value = std::max(m_min, m_value - m_step); }
        void        IncreaseValue() override { m_value = std::min(m_max, m_value + m_step); }

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

        const wchar_t* GetOptionText(wchar_t*, uint32_t) const override
        {
            return m_optionNames[UIParamTyped<valType>::GetValue()];
        }

        void        DecreaseValue() override { UIParamTyped<valType>::m_value += m_count - UIParamTyped<valType>::m_step;    UIParamTyped<valType>::m_value %= m_count; }
        void        IncreaseValue() override { UIParamTyped<valType>::m_value += UIParamTyped<valType>::m_step;              UIParamTyped<valType>::m_value %= m_count; }

    protected:
        const wchar_t* const*   m_optionNames;
        uint32_t                m_count;
    };

    using UIParamEnum = UIParamCountable<uint32_t>;

    class UIParamFloat : public UIParamTyped< float >
    {
    public:
        UIParamFloat(const wchar_t* paramName, float value = 0, float step = 1, uint32_t precision = 2)
            : UIParamTyped< float >(paramName, value, step)
            , m_precision(precision)
        {}

        const wchar_t* GetOptionText(wchar_t* buffer, uint32_t count) const override
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
    enum : uint32_t
    {
        UI_PARAM_FRAME_LOAD_SEQUENCE,

        UI_PARAM_CPU_UPDATE_LOAD,
        UI_PARAM_CPU_RENDER_LOAD,
        UI_PARAM_CPU_TO_GPU_LAG,
        UI_PARAM_GPU_GRAPHICS_LOAD,
        UI_PARAM_GPU_COMPUTE_LOAD,

        UI_PARAM_FRAME_RATE,
        UI_PARAM_FRAME_THRESHOLD,
        UI_PARAM_FRAME_PERIOD,
        UI_PARAM_FRAME_BUFFERS,
        UI_PARAM_FRAME_OFFSET,

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
            m_params[UI_PARAM_FRAME_LOAD_SEQUENCE] = new UIParamEnum(L"Auto Frame Load", c_frameLoadSequenceNames, _countof(c_frameLoadSequenceNames));

            m_params[UI_PARAM_CPU_UPDATE_LOAD] = new UIParamFloat(L"CPU Update (ms)", 10.0f);
            m_params[UI_PARAM_CPU_RENDER_LOAD] = new UIParamFloat(L"CPU Render (ms)", 12.0f);
            m_params[UI_PARAM_CPU_TO_GPU_LAG] = new UIParamFloat(L"CPU To GPU Lag (ms)", 2.0f);
            m_params[UI_PARAM_GPU_GRAPHICS_LOAD] = new UIParamFloat(L"GPU Graphics (ms)", 15.0f);
            m_params[UI_PARAM_GPU_COMPUTE_LOAD] = new UIParamFloat(L"GPU Compute (ms)", 3.0f);

            m_params[UI_PARAM_FRAME_RATE] = new UIParamEnum(L"Frame Rate", c_frameRateNames, _countof(c_frameRateNames), 1, 2); // skip 120/40 fps for now
            m_params[UI_PARAM_FRAME_THRESHOLD] = new UIParamInt(L"Frame Threshold (%)", 0, 100, 0, 10);
            m_params[UI_PARAM_FRAME_PERIOD] = new UIParamInt(L"Frame Period", 1, 3, 2);
            m_params[UI_PARAM_FRAME_BUFFERS] = new UIParamInt(L"Frame Buffers", 2, 4, 3);
            m_params[UI_PARAM_FRAME_OFFSET] = new UIParamFloat(L"Frame Offset (ms)", 0.0f, 1.0f);
        }

        ~Menu()
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
            dynamic_cast<UIParamTyped< valType >*>(m_params[itemNum])->SetValue(value);
        }
        inline uint32_t GetItemStatus(uint32_t itemNum)
        {
            return m_params[itemNum]->GetStatus();
        }
        inline void SetItemStatus(uint32_t itemNum, uint32_t status)
        {
            m_params[itemNum]->SetStatus(status);
        }
        inline void DecreaseItemValue(uint32_t itemNum)
        {
            m_params[itemNum]->DecreaseValue();
        }
        inline void IncreaseItemValue(uint32_t itemNum)
        {
            m_params[itemNum]->IncreaseValue();
        }

        void Update(const DirectX::GamePad::ButtonStateTracker& input)
        {
            if (m_processInput)
            {
                using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

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
                    if (UIParam::STATUS_AUTO != GetItemStatus(m_activeUIParameter))
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
                }
            }
        }

        float Render(DirectX::SpriteBatch* batch, DirectX::SpriteFont* font, DirectX::SpriteFont* controllerFont, DirectX::XMFLOAT2 position)
        {
            float paramYInc = 30.0f;

            bool useBigMenu = m_useBigMenu || UI_PARAM_COUNT <= m_visibleUICount;

            uint32_t visibleUIStartIndex = useBigMenu ? 0 : m_visibleUIStart;
            uint32_t visibleUICount = useBigMenu ? UI_PARAM_COUNT : m_visibleUICount;

            if (!useBigMenu)
            {
                position.x += 40.0f;
                font->DrawString(batch, L"^", position, DirectX::Colors::Magenta, 0.0f, { 0.0f, 0.0f }, 0.8f);
                position.x += 100.0f;
                DX::DrawControllerString(batch, font, controllerFont, L"( [B] to expand )", position, DirectX::Colors::White, 0.8f);
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
                font->DrawString(batch, L"v", position, DirectX::Colors::Magenta, 0.0f, { 0.0f, 0.0f }, 0.8f);
                position.y += paramYInc;
            }

            return position.y;
        }

        UIParam*                    m_params[UI_PARAM_COUNT];
        bool                        m_useBigMenu;
        bool                        m_processInput;
        uint32_t                    m_activeUIParameter;   // Which UI item is affected by l/r input
        uint32_t                    m_visibleUIStart;
        uint32_t                    m_visibleUICount;
    };
} // namespace FramePacingUtils
