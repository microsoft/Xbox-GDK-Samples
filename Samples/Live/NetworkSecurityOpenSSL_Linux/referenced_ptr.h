// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

template <typename TInterface>
struct referenced_ptr
{
    referenced_ptr() noexcept
        : _ptr(nullptr)
    {}

    explicit referenced_ptr(TInterface* ptr) noexcept
        : _ptr(ptr)
    {
        add_ref();
    }

    referenced_ptr(const referenced_ptr& other) noexcept
        : _ptr(other.get())
    {
        add_ref();
    }

    referenced_ptr(referenced_ptr&& other) noexcept
        : _ptr(other.release())
    {}

    referenced_ptr& operator=(const referenced_ptr& other) noexcept
    {
        if (this == &other) { return *this; }

        reset();
        _ptr = other.get();
        add_ref();

        return *this;
    }

    referenced_ptr& operator=(referenced_ptr&& other) noexcept
    {
        if (this == &other) { return *this; }

        reset();
        _ptr = other.release();

        return *this;
    }

    ~referenced_ptr() noexcept
    {
        reset();
    }
    
    TInterface* get() const noexcept
    {
        return _ptr;
    }
    
    TInterface* release() noexcept
    {
        TInterface* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }

    void reset() noexcept
    {
        if (_ptr) _ptr->Release();
        _ptr = nullptr;
    }

    explicit operator bool() const noexcept
    {
        return _ptr != nullptr;
    }

    TInterface* operator->() const noexcept
    {
        return _ptr;
    }

    TInterface** address_of() noexcept
    {
        reset();
        return &_ptr;
    }

private:
    void add_ref() noexcept
    {
        if (_ptr) _ptr->AddRef();
    }

    TInterface* _ptr = nullptr;
};

template <typename TInterface>
bool operator==(referenced_ptr<TInterface>& p, std::nullptr_t)
{
    return p.get() == nullptr;
}

template <typename TInterface>
bool operator!=(referenced_ptr<TInterface>& p, std::nullptr_t)
{
    return p.get() != nullptr;
}
