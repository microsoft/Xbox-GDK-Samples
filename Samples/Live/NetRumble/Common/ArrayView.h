#pragma once

// Adapter while we wait for C++20 std::span
namespace ATG
{
    template<typename T>
    class ArrayView
    {
    public:
        ArrayView(T* data, size_t size) : m_data(data), m_size(size) { }

        T* begin() { return m_data; }
        T* end() { return m_data + m_size; }

        const T* begin() const { return m_data; }
        const T* end() const { return m_data + m_size; }

        T &operator[](size_t index) { return m_data[index]; }
        const T &operator[](size_t index) const { return m_data[index]; }

        size_t size() const { return m_size; }
    private:
        T *m_data;
        size_t m_size;
    };
}