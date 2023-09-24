#pragma once

#include <stdint.h>

template <class T>
class Span {
    T* m_data;
    uint32_t m_size;

public:
    Span(T* data = nullptr, uint32_t size = 0) : m_data(data), m_size(size) {}

    const T* data() const { return m_data; }
    T* data() { return m_data; }

    const T* begin() const { return m_data; }
    T* begin() { return m_data; }
    const T* end() const { return m_data + m_size; }
    T* end() { return m_data + m_size; }

    const T& operator[](uint32_t i) const { return m_data[i]; }
    T& operator[](uint32_t i) { return m_data[i]; }

    uint32_t size() const { return m_size; }
    int32_t ssize() const { return (int32_t)m_size; }
};