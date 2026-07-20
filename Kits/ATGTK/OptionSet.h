//--------------------------------------------------------------------------------------
// File: OptionSet.h
//
// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <utility>


namespace ATG
{
    template <size_t Size>
    struct OptionSet
    {
        struct Option
        {
            union Value
            {
                float floatVal;
                uint32_t uintVal;
                uint8_t byteVal;
                bool boolVal;
            };

            enum Type
            {
                Float,
                Uint,
                Byte,
                Bool,
                Enum,
            };

        private:
            Value m_value{};
            Value m_minVal{};
            Value m_maxVal{};
            Value m_increment{};
            const wchar_t* m_name = L"Unnamed option";
            std::vector<const wchar_t*> m_optionNames{};
            Type m_type = Bool;
            bool m_isDisabled = false;

        public:
            Option() noexcept
            {
            }

            Option(float val, float minV, float maxV, float incr, const wchar_t* n) noexcept
            {
                m_value.floatVal = val;
                m_minVal.floatVal = minV;
                m_maxVal.floatVal = maxV;
                m_increment.floatVal = incr;
                m_name = n;
                m_type = Float;
            }

            Option(uint32_t val, uint32_t minV, uint32_t maxV, uint32_t incr, const wchar_t* n) noexcept
            {
                m_value.uintVal = val;
                m_minVal.uintVal = minV;
                m_maxVal.uintVal = maxV;
                m_increment.uintVal = incr;
                m_name = n;
                m_type = Uint;
            }

            Option(uint8_t val, uint8_t minV, uint8_t maxV, uint8_t incr, const wchar_t* n) noexcept
            {
                m_value.byteVal = val;
                m_minVal.byteVal = minV;
                m_maxVal.byteVal = maxV;
                m_increment.byteVal = incr;
                m_name = n;
                m_type = Byte;
            }

            Option(uint32_t val, uint32_t maxV, std::vector<const wchar_t*>& optionNames, const wchar_t* n) noexcept(false)
            {
                m_value.uintVal = val;
                m_minVal.uintVal = 0;
                m_maxVal.uintVal = maxV;
                m_increment.uintVal = 1;
                m_name = n;
                m_optionNames = std::move(optionNames);
                m_type = Enum;
            }

            Option(bool val, const wchar_t* n) noexcept
            {
                m_value.boolVal = val;
                m_name = n;
                m_type = Bool;
            }

            Option(const Option&) = delete;
            Option& operator=(const Option&) = delete;

            Option(Option&&) = default;
            Option& operator=(Option&&) = default;

            void Set(float val) noexcept
            {
                m_value.floatVal = val;
            }

            void Set(uint32_t val) noexcept
            {
                m_value.uintVal = val;
            }

            void Set(uint8_t val) noexcept
            {
                m_value.byteVal = val;
            }

            void Set(bool val) noexcept
            {
                m_value.boolVal = val;
            }

            void SetIsDisabled(bool isDisabled) noexcept
            {
                m_isDisabled = isDisabled;
            }

            float GetFloat() const noexcept
            {
                return m_value.floatVal;
            }

            uint32_t GetUint() const noexcept
            {
                return m_value.uintVal;
            }

            uint8_t GetByte() const noexcept
            {
                return m_value.byteVal;
            }

            bool GetBool() const noexcept
            {
                return m_value.boolVal;
            }

            Type GetType() const noexcept
            {
                return m_type;
            }

            const wchar_t* GetName() const noexcept
            {
                return m_name;
            }

            const wchar_t* GetOptionName(uint32_t index) const noexcept(false)
            {
                return m_optionNames[index];
            }

            bool GetIsDisabled() const noexcept
            {
                return m_isDisabled;
            }

            void Increment()
            {
                switch (m_type)
                {
                case Float:
                    if (m_value.floatVal >= m_maxVal.floatVal)
                    {
                        m_value.floatVal = m_minVal.floatVal;
                    }
                    else
                    {
                        m_value.floatVal += m_increment.floatVal;
                    }
                    break;
                case Uint:
                    if (m_value.uintVal == m_maxVal.uintVal)
                    {
                        m_value.uintVal = m_minVal.uintVal;
                    }
                    else
                    {
                        m_value.uintVal += m_increment.uintVal;
                    }
                    break;
                case Enum:
                    if (m_value.uintVal == m_maxVal.uintVal - 1)
                    {
                        m_value.uintVal = m_minVal.uintVal;
                    }
                    else
                    {
                        m_value.uintVal += m_increment.uintVal;
                    }
                    break;
                case Byte:
                    if (m_value.byteVal == m_maxVal.byteVal)
                    {
                        m_value.byteVal = m_minVal.byteVal;
                    }
                    else
                    {
                        m_value.byteVal += m_increment.byteVal;
                    }
                    break;
                case Bool:
                    m_value.boolVal = !m_value.boolVal;
                }
            }

            void Decrement()
            {
                switch (m_type)
                {
                case Float:
                    if (m_value.floatVal <= m_minVal.floatVal)
                    {
                        m_value.floatVal = m_maxVal.floatVal;
                    }
                    else
                    {
                        m_value.floatVal -= m_increment.floatVal;
                    }
                    break;
                case Uint:
                    if (m_value.uintVal == m_minVal.uintVal)
                    {
                        m_value.uintVal = m_maxVal.uintVal;
                    }
                    else
                    {
                        m_value.uintVal -= m_increment.uintVal;
                    }
                    break;
                case Enum:
                    if (m_value.uintVal == m_minVal.uintVal)
                    {
                        m_value.uintVal = m_maxVal.uintVal - 1;
                    }
                    else
                    {
                        m_value.uintVal -= m_increment.uintVal;
                    }
                    break;
                case Byte:
                    if (m_value.byteVal == m_minVal.byteVal)
                    {
                        m_value.byteVal = m_maxVal.byteVal;
                    }
                    else
                    {
                        m_value.byteVal -= m_increment.byteVal;
                    }
                    break;
                case Bool:
                    m_value.boolVal = !m_value.boolVal;
                }
            }
        };

        Option& operator[] (size_t index)
        {
            return m_options[index];
        }

        const Option& operator[] (size_t index) const
        {
            return m_options[index];
        }

        void Increment()
        {
            m_options[m_selectedIndex].Increment();
        }

        void Decrement()
        {
            m_options[m_selectedIndex].Decrement();
        }

        void SelectPrevious() noexcept
        {
            m_selectedIndex = ((m_selectedIndex == 0) ? Size - 1 : m_selectedIndex - 1);
        }

        void SelectNext() noexcept
        {
            m_selectedIndex = ((m_selectedIndex == Size - 1) ? 0 : m_selectedIndex + 1);
        }

        void Print(_Out_writes_z_(bufferSize) wchar_t* optionsText, _In_ size_t bufferSize, _In_z_ const wchar_t* title) const
        {
            int offset = swprintf_s(optionsText, bufferSize, L"%ls\n", title);
            for (size_t i = 0; i < Size; ++i)
            {
                switch (m_options[i].GetType())
                {
                case Option::Float:
                    offset += swprintf_s(&optionsText[offset], bufferSize - offset, L"%ls%ls%ls: %0.2f\n", i == m_selectedIndex ? L" * " : L"   ", m_options[i].GetIsDisabled() ? L"(Disabled) " : L"", m_options[i].GetName(), m_options[i].GetFloat());
                    break;
                case Option::Uint:
                    offset += swprintf_s(&optionsText[offset], bufferSize - offset, L"%ls%ls%ls: %u\n", i == m_selectedIndex ? L" * " : L"   ", m_options[i].GetIsDisabled() ? L"(Disabled) " : L"", m_options[i].GetName(), m_options[i].GetUint());
                    break;
                case Option::Byte:
                    offset += swprintf_s(&optionsText[offset], bufferSize - offset, L"%ls%ls%ls: %u\n", i == m_selectedIndex ? L" * " : L"   ", m_options[i].GetIsDisabled() ? L"(Disabled) " : L"", m_options[i].GetName(), m_options[i].GetByte());
                    break;
                case Option::Bool:
                    offset += swprintf_s(&optionsText[offset], bufferSize - offset, L"%ls%ls%ls: %ls\n", i == m_selectedIndex ? L" * " : L"   ", m_options[i].GetIsDisabled() ? L"(Disabled) " : L"", m_options[i].GetName(), m_options[i].GetBool() ? L"true" : L"false");
                    break;
                case Option::Enum:
                    offset += swprintf_s(&optionsText[offset], bufferSize - offset, L"%ls%ls%ls: %ls\n", i == m_selectedIndex ? L" * " : L"   ", m_options[i].GetIsDisabled() ? L"(Disabled) " : L"", m_options[i].GetName(), m_options[i].GetOptionName(m_options[i].GetUint()));
                    break;
                default:
                    break;
                }
            }
        }

    private:
        Option m_options[Size];
        uint32_t m_selectedIndex = 0;
    };
}
