#include <stdexcept>
#include <cstring>
#include <memory>
#include <cmath>
using namespace std;

template<class T>
class ring_iterator {
    size_t idx;
    size_t cycle;
    T* buffer;
public:
    ring_iterator(size_t idx, size_t cycle, T* buffer) : idx(idx), cycle(cycle), buffer(buffer) {}
    
    ring_iterator operator+(int offset) const {
        ring_iterator copy(*this);
        copy += offset;
        return copy;
    }
    
    ring_iterator operator-(int offset) const {
        ring_iterator copy(*this);
        copy -= offset;
        return copy;
    }
    
    ring_iterator& operator++() {
        idx = (idx + 1) % cycle;
        return *this;
    }
    
    ring_iterator& operator--() {
        idx = (idx == 0) ? cycle-1 : idx-1;
        return *this;
    }
    
    ring_iterator operator++(int) {
        ring_iterator copy(*this);
        operator++();
        return copy;
    }
    
    ring_iterator operator--(int) {
        ring_iterator copy(*this);
        operator--();
        return copy;
    }
    
    ring_iterator& operator+=(int offset) {
        idx = (idx + offset) % cycle;
        return *this;
    }
    
    ring_iterator& operator-=(int offset) {
        int tmpidx = static_cast<int>(idx);
        tmpidx -= offset;
        while (tmpidx < 0)
            tmpidx = cycle - (abs(tmpidx) - 1);
        idx = tmpidx;
        return *this;
    }
    
    T& operator*() {
        return buffer[idx];
    }
    
    T const& operator*() const {
        return buffer[idx];
    }
    
    friend bool operator==(ring_iterator const& lhs, ring_iterator const& rhs) {
        return lhs.idx == rhs.idx;
    }
    
    friend bool operator!=(ring_iterator const& lhs, ring_iterator const& rhs) {
        return !(lhs == rhs);
    }
};

template<class T>
class RingBuffer {
public:
    using iterator = ring_iterator<T>;
public:
    RingBuffer() = default;
    RingBuffer(size_t);
    RingBuffer(size_t, T const&);
    T get() const;
    void put(T const&);
    void pop();
    // access
    T& front();
    T const& front() const;
    T& back();
    T const& back() const;
    T& at(size_t idx);
    T const& at(size_t) const;
    T& operator[](size_t idx);
    T const& operator[](size_t idx) const;
    // iterators
    iterator begin();
    iterator const begin() const;
    iterator end();
    iterator const end() const;
    // size
    size_t size() const;
    bool empty() const;
    bool full() const;
private:
    void put(T const&, true_type);
    void put(T const&, false_type);
    int overflow(int n) const;
private:
    shared_ptr<T> buffer = nullptr;
    mutable size_t read, write, length, capacity = 0;
    friend int main();
};

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer(new T[capacity])
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
        write = overflow(write + 1);
        ++length;
    }
}

template<class T>
T RingBuffer<T>::get() const {
    if (!empty()) {
        T tmp = at(read);
        read = overflow(read + 1);
        --length;
        return tmp;
    }
    throw logic_error("empty buffer");
}

template<class T>
void RingBuffer<T>::put(T const& value, true_type) {
    memcpy(reinterpret_cast<void*>(&*end()), reinterpret_cast<void*>(const_cast<T*>(&value)), sizeof(value));
}

template<class T>
void RingBuffer<T>::put(T const& value, false_type) {
    *end() = value;
}

template<class T>
void RingBuffer<T>::pop() {
    if (!empty()) {
        read = overflow(read + 1);
        --length;
    } else {
        throw logic_error("empty");
    }
}

template<class T>
auto RingBuffer<T>::begin() const -> iterator const {
    return iterator(read, capacity, buffer.get());
}

template<class T>
auto RingBuffer<T>::begin() -> iterator {
    return static_cast<RingBuffer<T> const&>(*this).begin();
}
 
template<class T>
auto RingBuffer<T>::end() -> iterator {
    return iterator(write, capacity, buffer.get());
}

template<class T>
auto RingBuffer<T>::end() const -> iterator const {
    return const_cast<RingBuffer<T>&>(*this).end();
}

template<class T>
T& RingBuffer<T>::front() {
    return *begin();
}

template<class T>
T const& RingBuffer<T>::front() const {
    return const_cast<RingBuffer<T>&>(*this).front();
}

template<class T>
T& RingBuffer<T>::back() {
    if (size() <= 1)
        return *begin();
    return *(end() - 1);
}

template<class T>
T const& RingBuffer<T>::back() const {
    return const_cast<RingBuffer<T>&>(*this).back();
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
    return length == 0;
}

template<class T>
T& RingBuffer<T>::operator[](size_t idx) {
    return *(buffer.get() + overflow(read + idx));
}

template<class T>
T const& RingBuffer<T>::operator[](size_t idx) const {
    return const_cast<RingBuffer<T>&>(*this)[idx];
}

template<class T>
T& RingBuffer<T>::at(size_t idx) {
    if (!empty())
        return *(buffer.get() + overflow(read + idx));
    throw out_of_range("index too large");
}

template<class T>
T const& RingBuffer<T>::at(size_t idx) const {
    return const_cast<RingBuffer&>(*this).at(idx);
}

template<class T>
int RingBuffer<T>::overflow(int n) const {
    return n % capacity;
}
