#include <iostream>
#include <iterator>
#include <memory>
#include <cstring>
#include <cmath>
using namespace std;

template<class T>
class RingBuffer {
public:
    RingBuffer() = default;
    RingBuffer(size_t);
    RingBuffer(size_t, T const&);
    T get() const;
    void put(T const&);
    // access
    T& front();
    T const& front() const;
    T& back();
    T const& back() const;
    // iterators
    T* begin();
    T* const begin() const;
    T* end();
    T* const end() const;
    // size
    size_t size() const;
    bool empty() const;
    bool full() const;
private:
    void put(T const&, true_type);
    void put(T const&, false_type);
    void set_next_write_position() const;
    void set_next_read_position() const;
private:
    unique_ptr<T[]> m_buffer = nullptr;
    size_t read, write;
    size_t m_capacity = 0;
};

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : m_buffer(make_unique<T[]>(capacity+1))
    , read(0)
    , write(0)
    , m_capacity(capacity+1)
{}

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity, T const& value)
    : RingBuffer(capacity)
{
    for (int i = 0; i < capacity; ++i)
        put(value);
}

template<class T>
void RingBuffer<T>::put(T const& value) {
    if (!full()) {
        put(value, is_trivially_copyable<T>{});
        write = (write + 1) % m_capacity;
    }
}

template<class T>
T RingBuffer<T>::get() const {
    T tmp = m_buffer[read];
    read = (read + 1) % m_capacity;
    return tmp;
}

template<class T>
void RingBuffer<T>::put(T const& value, true_type) {
    memcpy(reinterpret_cast<void*>(&m_buffer[write]), reinterpret_cast<void*>(const_cast<T*>(&value)), sizeof(value));
}

template<class T>
void RingBuffer<T>::put(T const& value, false_type) {
    m_buffer[write] = value;
}

template<class T>
T* const RingBuffer<T>::begin() const {
    return &m_buffer[read];
}

template<class T>
T* RingBuffer<T>::begin() {
    return static_cast<RingBuffer<T> const&>(*this).begin();
}

template<class T>
T* RingBuffer<T>::end() {
    return &m_buffer[write];
}

template<class T>
T* const RingBuffer<T>::end() const {
    return static_cast<RingBuffer<T> const&>(*this).end();
}

template<class T>
T const& RingBuffer<T>::front() const {
    return static_cast<RingBuffer<T> const&>(*this).front();
}

template<class T>
T& RingBuffer<T>::front() {
    return *begin();
}

template<class T>
T& RingBuffer<T>::back() {
    if (size() <= 1)
        return *begin();
    return *(end() - 1);
}

template<class T>
T const& RingBuffer<T>::back() const {
    return static_cast<RingBuffer<T> const&>(*this).back();
}

template<class T>
size_t RingBuffer<T>::size() const {
    return abs(write - read);
}

template<class T>
bool RingBuffer<T>::full() const {
    return read == (write + 1) % m_capacity;
}

template<class T>
bool RingBuffer<T>::empty() const {
    return write == read;
}
