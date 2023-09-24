#pragma once

#define MAX(a, b) (((a)>(b))? (a):(b))

#include <type_traits>
#include <initializer_list>
#include <cstring>

#include "span.h"
#include "log.h"

template <class T>
class Vector {
    uint32_t m_capacity, m_size;
    T* m_data = nullptr;

public:
    Vector(uint32_t size = 0) : m_capacity(size), m_size(size), m_data(size ? new T[size] : nullptr) {}

    Vector(uint32_t size, const T& item) {
        for (uint32_t i = 0; i < m_size; i++) {
            m_data[i] = item;
        }
    }

    ~Vector() {
        if (m_data) {
            delete[] m_data;
        }
    }

    Vector(const Vector& other) : m_capacity(other.m_capacity), m_size(other.m_size),
                                  m_data(other.m_capacity ? new T[other.m_capacity] : nullptr)
    {
        copy(other.m_data, other.m_size, m_data);
    }

    friend void swap(Vector& first, Vector& second) noexcept {
        using std::swap;

        swap(first.m_capacity, second.m_capacity);
        swap(first.m_size, second.m_size);
        swap(first.m_data, second.m_data);
    }

    Vector(Vector&& other) noexcept {
        swap(*this, other);
    }

    Vector& operator=(Vector other) {
        swap(*this, other);
        return *this;
    }

    Vector(std::initializer_list<T> lst) : m_capacity(lst.size()), m_size(lst.size()),
                                           m_data(lst.size() ? new T[lst.size()] : nullptr)
    {
        copy(std::data(lst), lst.size(), m_data);
    }

    operator Span<T>() {
        return {m_data, m_size};
    }

    operator Span<const T>() const {
        return {m_data, m_size};
    }

    uint32_t size() const { return m_size; }
    int32_t ssize() const { return (int32_t)m_size; }

    uint32_t capacity() const { return m_capacity; }

    const T* data() const { return m_data; }
    T* data() { return m_data; }

    const T* begin() const { return m_data; }
    T* begin() { return m_data; }

    const T* end() const { return m_data + m_size; }
    T* end() { return m_data + m_size; }

    const T& operator[](uint32_t i) const {
        log_assert(i < m_size);
        return m_data[i];
    };
    T& operator[](uint32_t i) {
        log_assert(i < m_size);
        return m_data[i];
    }

    const T& front() const { return m_data[0]; }
    T& front() { return m_data[0]; }

    const T& back() const { return m_data[m_size - 1]; }
    T& back() { return m_data[m_size - 1]; }

    bool empty() const { return m_size == 0; }

    void push_back(T elem) {
        ensure_capacity(m_size + 1);
        m_data[m_size++] = elem;
    }

    T& push_empty() {
        ensure_capacity(m_size + 1);
        return m_data[m_size++];
    }

    template <class ...Args>
    void emplace_back(Args&&... args) {
        ensure_capacity(m_size + 1);
        new (m_data + m_size++) T(args...);
    }

    T pop_back() {
        return m_data[--m_size];
    }

    void resize(uint32_t new_size) {
        T* new_data = new T[new_size];
        copy(m_data, m_size, new_data);
        delete[] m_data;
        m_data = new_data;
        m_capacity = m_size = new_size;
    }

    void reserve(uint32_t new_capacity) {
        if (new_capacity <= m_capacity) return;
        T* new_data = new T[new_capacity];
        copy(m_data, m_size, new_data);
        delete[] m_data;
        m_data = new_data;
        m_capacity = new_capacity;
    }

    void clear() {
        if (m_data) {
            delete[] m_data;
            m_data = nullptr;
        }
        m_capacity = m_size = 0;
    }

    T* find(const T& item) {
        for (uint32_t i = 0; i < m_size; i++) {
            if (m_data[i] == item) return &m_data[i];
        }
        return nullptr;
    }

    template <class Predicate>
    T* find_if(Predicate&& predicate) {
        for (uint32_t i = 0; i < m_size; i++) {
            if (predicate(m_data[i])) return &m_data[i];
        }
        return nullptr;
    }

private:
    static void copy(const T* src, uint32_t n, T* dst) {
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy(dst, src, sizeof(T) * n);
        }
        else {
            for (uint32_t i = 0; i < n; i++) {
                dst[i] = src[i];
            }
        }
    }

    void ensure_capacity(uint32_t min_capacity) {
        if (min_capacity <= m_capacity) return;
        size_t new_capacity = MAX(m_capacity * 2, min_capacity);
        T* new_data = new T[new_capacity];
        copy(m_data, m_size, new_data);
        delete[] m_data;
        m_data = new_data;
        m_capacity = new_capacity;
    }
};

#undef MAX