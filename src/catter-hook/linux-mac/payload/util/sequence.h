#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace catter {

// a subtitude for vector/string
template <typename T>
class Seq {
public:
    Seq(size_t reserved = 4) : _size(0), _capacity(reserved) {
        _data = (reserved > 0) ? alloc(reserved) : nullptr;
    }

    Seq(const Seq&) = delete;
    Seq& operator= (const Seq&) = delete;

    Seq(Seq&& other) noexcept : _data(other._data), _size(other._size), _capacity(other._capacity) {
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    template <typename Arg>
    Seq& append(Arg&& arg) {
        if(_size >= _capacity) {
            grow();
        }
        new (&_data[_size]) T(static_cast<Arg&&>(arg));
        _size++;
        return *this;
    }

    void pop_back() {
        if(_size > 0) {
            _data[_size - 1].~T();
            _size--;
        }
    }

    ~Seq() {
        for(size_t i = 0; i < _size; ++i) {
            _data[i].~T();
        }
        free(_data);
    }

private:
    void grow() {
        size_t new_cap = (_capacity == 0) ? 1 : _capacity * 2;
        T* new_data = alloc(new_cap);

        for(size_t i = 0; i < _size; ++i) {
            new (&new_data[i]) T(static_cast<T&&>(_data[i]));
            _data[i].~T();
        }

        free(_data);
        _data = new_data;
        _capacity = new_cap;
    }

    static T* alloc(size_t sz) {
        return static_cast<T*>(std::aligned_alloc(alignof(T), sizeof(T) * sz));
    }

private:
    T* _data;
    size_t _size;
    size_t _capacity;

public:
    struct Iterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        Iterator(pointer ptr) : m_ptr(ptr) {}

        reference operator* () const {
            return *m_ptr;
        }

        pointer operator->() {
            return m_ptr;
        }

        Iterator& operator++ () {
            m_ptr++;
            return *this;
        }

        Iterator operator++ (int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator== (const Iterator& a, const Iterator& b) {
            return a.m_ptr == b.m_ptr;
        };

        friend bool operator!= (const Iterator& a, const Iterator& b) {
            return a.m_ptr != b.m_ptr;
        };

    private:
        pointer m_ptr;
    };

    Iterator begin() {
        return Iterator(_data);
    }

    Iterator end() {
        return Iterator(_data + _size);
    }
};

}  // namespace catter
