#include <iostream>
#include <cstring>
#include <memory>
using namespace std;

template<class T>
class RingBuffer {
public:
    RingBuffer() = delete;
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
    unique_ptr<T[]> buffer = nullptr;
    mutable size_t read, write, length, capacity = 0;
};

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer(make_unique<T[]>(capacity+1))
    , read(0)
    , write(0)
    , length(0)
    , capacity(capacity+1)
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
        write = (write + 1) % capacity;
        ++length;
    }
}

template<class T>
T RingBuffer<T>::get() const {
    T tmp = buffer[read];
    if (!empty()) {
        read = (read + 1) % capacity;
        --length;
    }
    return tmp;
}

template<class T>
void RingBuffer<T>::put(T const& value, true_type) {
    memcpy(reinterpret_cast<void*>(&buffer[write]), reinterpret_cast<void*>(const_cast<T*>(&value)), sizeof(value));
}

template<class T>
void RingBuffer<T>::put(T const& value, false_type) {
    buffer[write] = value;
}

template<class T>
T* const RingBuffer<T>::begin() const {
    return &buffer[read];
}

template<class T>
T* RingBuffer<T>::begin() {
    return static_cast<RingBuffer<T> const&>(*this).begin();
}

template<class T>
T* RingBuffer<T>::end() {
    return &buffer[write];
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
    return length;
}

template<class T>
bool RingBuffer<T>::full() const {
    return read == (write + 1) % capacity;
}

template<class T>
bool RingBuffer<T>::empty() const {
    return size == 0;
}
