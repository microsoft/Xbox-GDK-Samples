//--------------------------------------------------------------------------------------
// File: BCryptHandle.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

// Wrapper classes for BCRYPT handles

namespace ATG
{
    ///////////////////////////////////////////////////////////////////////////
    //
    class BCryptKeyHandle
    {
    public:
        BCryptKeyHandle() :
            m_handle{ nullptr }
        {
        }

        ~BCryptKeyHandle()
        {
            clear();
        }

        explicit BCryptKeyHandle(BCRYPT_KEY_HANDLE source) :
            m_handle{ source }
        {
        }

        BCryptKeyHandle(const BCryptKeyHandle& source) :
            m_handle{ nullptr }
        {
            if (source.m_handle)
            {
                m_data.resize(source.m_data.size());

                auto hr = BCryptDuplicateKey(source.m_handle, &m_handle, m_data.data(), (ULONG)m_data.size(), 0);
                if (FAILED(hr))
                {
                    throw DtlsException(hr, "BCryptDuplicateKey");
                }
            }
        }

        BCryptKeyHandle& operator=(const BCryptKeyHandle& source)
        {
            clear();

            if (source.m_handle)
            {
                m_data.resize(source.m_data.size());

                auto hr = BCryptDuplicateKey(source.m_handle, &m_handle, m_data.data(), (ULONG)m_data.size(), 0);
                if (FAILED(hr))
                {
                    throw DtlsException(hr, "BCryptDuplicateKey");
                }
            }

            return *this;
        }

        BCryptKeyHandle(BCryptKeyHandle&& moveFrom) noexcept :
            m_handle{ moveFrom.m_handle },
            m_data{ std::move(moveFrom.m_data) }
        {
            moveFrom.m_handle = nullptr;
        }

        BCryptKeyHandle& operator=(BCryptKeyHandle&& moveFrom) noexcept
        {
            clear();

            m_handle = moveFrom.m_handle;
            m_data = std::move(moveFrom.m_data);

            moveFrom.m_handle = nullptr;

            return *this;
        }

        void reset(BCRYPT_KEY_HANDLE handle)
        {
            clear();
            m_handle = handle;
        }

        void clear()
        {
            if (m_handle)
            {
                BCryptDestroyKey(m_handle);
                m_handle = nullptr;
                m_data.clear();
            }
        }

        BCRYPT_KEY_HANDLE* reset_and_get_address_of()
        {
            clear();
            return &m_handle;
        }

        BCRYPT_KEY_HANDLE release()
        {
            BCRYPT_KEY_HANDLE temp = m_handle;
            m_handle = nullptr;
            m_data.clear();
            return temp;
        }

        BCRYPT_KEY_HANDLE* operator&() { return &m_handle; }
        operator BCRYPT_KEY_HANDLE() { return m_handle; }
        operator bool() const { return m_handle != nullptr; }
        operator uint8_t*() { return m_data.data(); }

        BCRYPT_KEY_HANDLE get() const { return m_handle; }
        BCRYPT_KEY_HANDLE* get_address_of() { return &m_handle; }
        void set_data_size(size_t size) { m_data.resize(size); }
        std::vector<uint8_t>& data() { return m_data; }

    private:
        BCRYPT_KEY_HANDLE m_handle;
        std::vector<uint8_t> m_data;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class BCryptSecretHandle
    {
    public:
        BCryptSecretHandle() :
            m_handle{ nullptr }
        {
        }

        ~BCryptSecretHandle()
        {
            clear();
        }

        explicit BCryptSecretHandle(BCRYPT_SECRET_HANDLE source) :
            m_handle{ source }
        {
        }

        BCryptSecretHandle(const BCryptSecretHandle& source) = delete;
        BCryptSecretHandle& operator=(const BCryptSecretHandle& source) = delete;

        BCryptSecretHandle(BCryptSecretHandle&& moveFrom) noexcept :
            m_handle{ moveFrom.m_handle },
            m_data{ std::move(moveFrom.m_data) }
        {
            moveFrom.m_handle = nullptr;
        }

        BCryptSecretHandle& operator=(BCryptSecretHandle&& moveFrom) noexcept
        {
            clear();

            m_handle = moveFrom.m_handle;
            m_data = std::move(moveFrom.m_data);

            moveFrom.m_handle = nullptr;

            return *this;
        }

        void reset(BCRYPT_SECRET_HANDLE handle)
        {
            clear();
            m_handle = handle;
        }

        void clear()
        {
            if (m_handle)
            {
                BCryptDestroySecret(m_handle);
                m_handle = nullptr;
                m_data.clear();
            }
        }

        BCRYPT_SECRET_HANDLE* reset_and_get_address_of()
        {
            clear();
            return &m_handle;
        }

        BCRYPT_SECRET_HANDLE release()
        {
            BCRYPT_SECRET_HANDLE temp = m_handle;
            m_handle = nullptr;
            m_data.clear();
            return temp;
        }

        BCRYPT_SECRET_HANDLE* operator&() { return &m_handle; }
        operator BCRYPT_SECRET_HANDLE() { return m_handle; }
        operator bool() const { return m_handle != nullptr; }
        operator uint8_t* () { return m_data.data(); }

        BCRYPT_SECRET_HANDLE get() const { return m_handle; }
        BCRYPT_SECRET_HANDLE* get_address_of() { return &m_handle; }
        void set_data_size(size_t size) { m_data.resize(size); }
        std::vector<uint8_t>& data() { return m_data; }

    private:
        BCRYPT_SECRET_HANDLE m_handle;
        std::vector<uint8_t> m_data;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class BCryptHashHandle
    {
    public:
        BCryptHashHandle() :
            m_handle{ nullptr }
        {
        }

        ~BCryptHashHandle()
        {
            clear();
        }

        explicit BCryptHashHandle(BCRYPT_HASH_HANDLE source) :
            m_handle{ source }
        {
        }

        BCryptHashHandle(const BCryptHashHandle& source) :
            m_handle{ nullptr }
        {
            if (source.m_handle)
            {
                m_data.resize(source.m_data.size());

                auto hr = BCryptDuplicateHash(source.m_handle, &m_handle, m_data.data(), (ULONG)m_data.size(), 0);
                if (FAILED(hr))
                {
                    throw DtlsException(hr, "BCryptDuplicateHash");
                }
            }
        }

        BCryptHashHandle& operator=(const BCryptHashHandle& source)
        {
            clear();

            if (source.m_handle)
            {
                m_data.resize(source.m_data.size());

                auto hr = BCryptDuplicateHash(source.m_handle, &m_handle, m_data.data(), (ULONG)m_data.size(), 0);
                if (FAILED(hr))
                {
                    throw DtlsException(hr, "BCryptDuplicateHash");
                }
            }

            return *this;
        }

        BCryptHashHandle(BCryptHashHandle&& moveFrom) noexcept :
            m_handle{ moveFrom.m_handle },
            m_data{ moveFrom.m_data }
        {
            moveFrom.m_handle = nullptr;
        }

        BCryptHashHandle& operator=(BCryptHashHandle&& moveFrom) noexcept
        {
            clear();

            m_handle = moveFrom.m_handle;
            m_data = std::move(moveFrom.m_data);

            moveFrom.m_handle = nullptr;

            return *this;
        }

        void reset(BCRYPT_HASH_HANDLE handle)
        {
            clear();
            m_handle = handle;
        }

        void clear()
        {
            if (m_handle)
            {
                BCryptDestroyHash(m_handle);
                m_handle = nullptr;
                m_data.clear();
            }
        }

        BCRYPT_HASH_HANDLE* reset_and_get_address_of()
        {
            clear();
            return &m_handle;
        }

        BCRYPT_HASH_HANDLE release()
        {
            BCRYPT_HASH_HANDLE temp = m_handle;
            m_handle = nullptr;
            m_data.clear();
            return temp;
        }

        BCRYPT_HASH_HANDLE* operator&() { return &m_handle; }
        operator BCRYPT_HASH_HANDLE() { return m_handle; }
        operator bool() const { return m_handle != nullptr; }
        operator uint8_t* () { return m_data.data(); }

        BCRYPT_HASH_HANDLE get() const { return m_handle; }
        BCRYPT_HASH_HANDLE* get_address_of() { return &m_handle; }
        void set_data_size(size_t size) { m_data.resize(size); }
        std::vector<uint8_t>& data() { return m_data; }

    private:
        BCRYPT_HASH_HANDLE m_handle;
        std::vector<uint8_t> m_data;
    };
}
