#include <stdexcept>
#include <cstring>
#include <memory>
#include <cmath>
using namespace std;

// An iterator for a cirular before (returned by the begin() and end() methods)
template<class T>
class ring_iterator {
    size_t idx; // current index
    size_t cycle; // index where there is a complete cycle
    T* buffer; // pointer to the element where idx lies (i.e buffer[idx])
public:
    ring_iterator(size_t idx, size_t cycle, T* buffer) : idx(idx), cycle(cycle), buffer(buffer) {}
    
    // operator+ delegates work to operator+=
    ring_iterator operator+(int offset) const {
        ring_iterator copy(*this);
        copy += offset;
        return copy;
    }
    
    // operator- delegates work to operator-=
    ring_iterator operator-(int offset) const {
        ring_iterator copy(*this);
        copy -= offset;
        return copy;
    }
    
    // preincrement: increase the index by 1
    ring_iterator& operator++() {
        idx = (idx + 1) % cycle;
        return *this;
    }
    
    // predecrement: if decrementing idx would yield a negative number, return cycle-1
    // otherwise subtract 1
    ring_iterator& operator--() {
        idx = (idx == 0) ? cycle-1 : idx-1;
        return *this;
    }
    
    // postincrement: create a copy, modify the current object, return the old value
    ring_iterator operator++(int) {
        ring_iterator copy(*this);
        operator++();
        return copy;
    }
    
    // postdecrement: create a copy, modify the current object, return the old value
    ring_iterator operator--(int) {
        ring_iterator copy(*this);
        operator--();
        return copy;
    }
    
    // += is simple: increase the index by the specified amount and use % to wrap around
    ring_iterator& operator+=(int offset) {
        idx = (idx + offset) % cycle;
        return *this;
    }
    
    // With -= we need to handle going past 0 and onto the other side of the ring.
    ring_iterator& operator-=(int offset) {
        int tmpidx = static_cast<int>(idx); // tmpidx will store idx as an integer in order to get negative values
        tmpidx -= offset; // decrease by offset
        // if the decrement yeiled a negative number, continually subtract |tmpidx|-1 from cycle
        while (tmpidx < 0)
            tmpidx = cycle - (abs(tmpidx) - 1);
        // store new value
        idx = tmpidx;
        return *this;
    }
    
    // the element in the buffer
    T& operator*() const {
        return buffer[idx];
    }
    
    // the element in the buffer
    T const& operator*() const {
        return buffer[idx];
    }
    
    // operator==
    friend bool operator==(ring_iterator const& lhs, ring_iterator const& rhs) {
        return lhs.idx == rhs.idx;
    }
    
    // operator!=
    friend bool operator!=(ring_iterator const& lhs, ring_iterator const& rhs) {
        return !(lhs == rhs);
    }
};

template<class T>
class RingBuffer {
public:
    // generic iterator tag denoting internal iterator type
    using iterator = ring_iterator<T>;
public:
    RingBuffer() = default;
    // capacity constructor
    RingBuffer(size_t);
    // capacity + initial value constructor
    RingBuffer(size_t, T const&);
    // retrieves the next element to be read
    T get() const;
    // insertion
    void put(T const&);
    // deletion
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
    // helper functions:
    // put overloads to check for trivial types
    void put(T const&, true_type);
    void put(T const&, false_type);
    // returns n (mod capacity)
    int overflow(int n) const;
private:
    // RAII circular buffer
    shared_ptr<T> buffer = nullptr;
    // read, write -> indicies into the buffer
    // length, capacity -> current size and capacity of the buffer
    mutable size_t read, write, length, capacity = 0;
};

// capacity constructor: construct buffer and set initial values
template<class T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer(new T[capacity], default_delete<T[]>{})
    , read(0)
    , write(0)
    , length(0)
    , capacity(capacity+1) // +1 for modulo
{}
 
template<class T>
RingBuffer<T>::RingBuffer(size_t capacity, T const& value)
    : RingBuffer(capacity) // delegate common work to previous constructor
{
    // fill array
    for (int i = 0; i < capacity; ++i)
        put(value);
}

// put delegates to 1 of 2 constructors based on whether T is a trivial type
template<class T>
void RingBuffer<T>::put(T const& value) {
    if (!full()) {
        put(value, is_trivially_copyable<T>{}); // call other overload
        write = overflow(write + 1); // increase position
        ++length; // increase length
    }
}

// get returns the next object to be read (which is stored in the index at read)
template<class T>
T RingBuffer<T>::get() const {
    if (!empty()) {
        T tmp = at(read); // get element
        read = overflow(read + 1); // increase read
        --length; // decrement size
        return tmp;
    }
    // if the buffer is empty this operation cannot work
    throw logic_error("empty buffer");
}

// trivial put function calls memcpy and copies bytes directly into the buffer
template<class T>
void RingBuffer<T>::put(T const& value, true_type) {
    memcpy(reinterpret_cast<void*>(&*end()), reinterpret_cast<void*>(const_cast<T*>(&value)), sizeof(value));
}

// if T is not trivially copyable simply call copy-assignment operator
template<class T>
void RingBuffer<T>::put(T const& value, false_type) {
    *end() = value;
}

// removes the pending element
// (similar to get())
template<class T>
void RingBuffer<T>::pop() {
    if (!empty()) {
        // increase read
        read = overflow(read + 1);
        // decrement length
        --length;
    } else {
        // if the buffer is empty this operation cannot be performed
        throw logic_error("empty");
    }
}

// returns a ring_iterator into the position in the array to be read from
template<class T>
auto RingBuffer<T>::begin() const -> iterator const {
    return iterator(read, capacity, buffer.get());
}

// non-const version: same as above
template<class T>
auto RingBuffer<T>::begin() -> iterator {
    return iterator(read, capacity, buffer.get());
}

// returns a ring_iterator into the position in the array to be written to
template<class T>
auto RingBuffer<T>::end() const -> iterator const {
    return iterator(write, capacity, buffer.get());
}

// non-const version: same as above
template<class T>
auto RingBuffer<T>::end() -> iterator {
    return iterator(write, capacity, buffer.get());
}

// returns the element to be read 
template<class T>
T& RingBuffer<T>::front() {
    return *begin();
}

// const-version: same as above
template<class T>
T const& RingBuffer<T>::front() const {
    return *begin();
}

// returns the element to be written to
template<class T>
T& RingBuffer<T>::back() {
    // if there is 1 element return the first one in the buffer
    if (size() <= 1)
        return *begin();
    // otherwise return the last element which is buffer[write]
    return *(end() - 1);
}

// const-version: same as above
template<class T>
T const& RingBuffer<T>::back() const {
    // const_cast is safe here as back() is returned and bound to a 
    // reference to const
    return const_cast<RingBuffer<T>&>(*this).back();
}

template<class T>
size_t RingBuffer<T>::size() const {
    return length;
}

// array is full if the condition below is satisifed.
template<class T>
bool RingBuffer<T>::full() const {
    return read == (write + 1) % capacity;
}

template<class T>
bool RingBuffer<T>::empty() const {
    return length == 0;
}

// accesses an element at idx in a ring-like fashion.
template<class T>
T& RingBuffer<T>::operator[](size_t idx) {
    return *(buffer.get() + overflow(read + idx));
}

// const-version: same as above
template<class T>
T const& RingBuffer<T>::operator[](size_t idx) const {
    return const_cast<RingBuffer<T>&>(*this)[idx];
}

// at is the same operator[] except it does bounds checking
template<class T>
T& RingBuffer<T>::at(size_t idx) {
    if (empty() || !(0 <= idx && idx < length))
        return operator[](idx);
    throw out_of_range("index too large");
}

// const-version: same as above
template<class T>
T const& RingBuffer<T>::at(size_t idx) const {
    if (empty() || !(0 <= idx && idx < length))
        return operator[](idx);
    throw out_of_range("index too large");
}

// returns n (mod capacity)
template<class T>
int RingBuffer<T>::overflow(int n) const {
    return n % capacity;
}
