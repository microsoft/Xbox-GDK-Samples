#pragma once

#include <XTaskQueue.h>
#include <XUser.h>

#include <xsapi-c/services_c.h>

namespace ATG
{
    template <typename HandleType>
    struct HandleTraits
    {
    };

#define SETHANDLETYPETRAITS(handleType, duplicateMethod, releaseMethod) \
    template <> struct HandleTraits<handleType>                         \
    {                                                                   \
        static constexpr auto ReleaseHandle = releaseMethod;            \
        static constexpr auto DuplicateHandle = duplicateMethod;        \
    };

    SETHANDLETYPETRAITS(XTaskQueueHandle, &XTaskQueueDuplicateHandle, &XTaskQueueCloseHandle);
    SETHANDLETYPETRAITS(XUserHandle, &XUserDuplicateHandle, &XUserCloseHandle);
    SETHANDLETYPETRAITS(XblContextHandle, &XblContextDuplicateHandle, &XblContextCloseHandle);
#undef SETHANDLETYPETRAITS

    template <typename HandleType>
    class XboxHandle
    {
    public:
        using ThisType = XboxHandle<HandleType>;
        using Traits = HandleTraits<HandleType>;

        XboxHandle() = default;

        ~XboxHandle() noexcept
        {
            clear();
        }

        explicit XboxHandle(HandleType source) noexcept :
            m_handle{source}
        {
        }

        XboxHandle(const ThisType &source) noexcept
        {
            if (source.m_handle)
            {
                Traits::DuplicateHandle(source.m_handle, &m_handle);
            }
        }

        ThisType &operator=(const ThisType &source) noexcept
        {
            clear();

            if (source.m_handle)
            {
                Traits::DuplicateHandle(source.m_handle, &m_handle);
            }
            return *this;
        }

        XboxHandle(ThisType &&moveFrom) noexcept :
            m_handle{moveFrom.m_handle}
        {
            moveFrom.m_handle = nullptr;
        }

        ThisType &operator=(ThisType &&moveFrom) noexcept
        {
            clear();

            m_handle = moveFrom.m_handle;
            moveFrom.m_handle = nullptr;

            return *this;
        }

        HandleType get() const noexcept
        {
            return m_handle;
        }

        operator bool() const noexcept
        {
            return m_handle != nullptr;
        }

        void reset(HandleType handle) noexcept
        {
            clear();

            m_handle = handle;
        }

        void clear() noexcept
        {
            if (m_handle)
            {
                Traits::ReleaseHandle(m_handle);
                m_handle = nullptr;
            }
        }

        HandleType *get_address_of() noexcept
        {
            return &m_handle;
        }

        HandleType *release_and_get_address_of() noexcept
        {
            clear();
            return &m_handle;
        }
    private:
        HandleType m_handle{ nullptr };
    };
}
