#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>

/* --------------- RawMemory --------------- */

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    explicit RawMemory(size_t capacity);
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept;
    RawMemory& operator=(RawMemory&& rhs) noexcept;
    ~RawMemory();

    T* operator+(size_t offset) noexcept;
    const T* operator+(size_t offset) const noexcept;

    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    void Swap(RawMemory& other) noexcept;

    const T* GetAddress() const noexcept;
    T* GetAddress() noexcept;

    size_t Capacity() const;

private:
    static T* Allocate(size_t n);
    static void Deallocate(T* buf) noexcept;

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

/* ---------- RawMemory Realise ---------- */

template <typename T>
RawMemory<T>::RawMemory(size_t capacity)
    : buffer_(Allocate(capacity))
    , capacity_(capacity) { }

template <typename T>
RawMemory<T>::RawMemory(RawMemory&& other) noexcept {
    Swap(other);
}

template <typename T>
RawMemory<T>& RawMemory<T>::operator=(RawMemory&& rhs) noexcept {
    if (this != &rhs) {
        Swap(rhs);
    }
    return *this;
}

template <typename T>
RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}

template <typename T>
T* RawMemory<T>::operator+(size_t offset) noexcept {
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template <typename T>
const T* RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory&>(*this) + offset;
}

template <typename T>
const T& RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory&>(*this)[index];
}

template <typename T>
T& RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}

template <typename T>
void RawMemory<T>::Swap(RawMemory& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template <typename T>
const T* RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}

template <typename T>
T* RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}

template <typename T>
size_t RawMemory<T>::Capacity() const {
    return capacity_;
}

template <typename T>
T* RawMemory<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
}

template <typename T>
void RawMemory<T>::Deallocate(T* buf) noexcept {
    operator delete(buf);
}

/* --------------- Vector --------------- */

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector(Vector&& other) noexcept;
    Vector& operator=(const Vector& rhs);
    Vector& operator=(Vector&& rhs) noexcept;
    ~Vector();

    void Resize(size_t new_size);
    void PushBack(const T& value);
    void PushBack(T&& value);
    void PopBack();

    template <typename... Args> T& EmplaceBack(Args&&... args);
    template <typename... Args> iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos);
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);

    size_t Size() const noexcept;
    size_t Capacity() const noexcept;

    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    void Swap(Vector& other) noexcept;
    void Reserve(size_t new_capacity);

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

/* ---------- Vector Realise ---------- */

template <typename T>
Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template <typename T>
Vector<T>::iterator Vector<T>::end() noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return data_.GetAddress();
}

template <typename T>
Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return data_.GetAddress();
}

template <typename T>
Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size)
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template <typename T>
Vector<T>::Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_)
{
    std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept
    : data_(std::move(other.data_))
    , size_(other.size_)
{
    other.size_ = 0;
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs) {
    if (this != &rhs) {
        if (rhs.size_ > data_.Capacity()) {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        }
        else {
            std::copy(
                rhs.data_.GetAddress(),
                rhs.data_.GetAddress() + std::min(rhs.size_, size_),
                data_.GetAddress());
            if (rhs.size_ < size_) {
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            }
            else {
                std::uninitialized_copy_n(
                    rhs.data_.GetAddress() + size_,
                    rhs.size_ - size_,
                    data_.GetAddress() + size_);
            }
            size_ = rhs.size_;
        }
    }

    return *this;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    if (this != &rhs) {
        Swap(rhs);
    }
    return *this;
}

template <typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template <typename T>
void Vector<T>::Resize(size_t new_size) {
    if (new_size < size_) {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        size_ = new_size;
    }
    else if (new_size > size_) {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        size_ = new_size;
    }
}

template <typename T>
void Vector<T>::PushBack(const T& value) {
    EmplaceBack(value);
}

template <typename T>
void Vector<T>::PushBack(T&& value) {
    EmplaceBack(std::forward<T>(value));
}

template <typename T>
void Vector<T>::PopBack() {
    if (size_ > 0) {
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        --size_;
    }
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
    return *(Emplace(cend(), std::forward<Args>(args)...));
}

template <typename T>
template <typename... Args>
Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
    assert(pos >= begin() && pos <= begin() + size_);
    if (size_ < data_.Capacity()) {

        if (pos == cend()) {
            new (data_.GetAddress() + (pos - cbegin())) T(std::forward<Args>(args)...);
        }
        else {
            T temp(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);
            }
            std::move_backward(data_.GetAddress() + (pos - cbegin()), end() - 1, end());
            *(data_.GetAddress() + (pos - cbegin())) = std::move(temp);
        }

        ++size_;
        return data_.GetAddress() + (pos - cbegin());
    }
    else {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        size_t diff = pos - cbegin();
        new (new_data.GetAddress() + diff) T(std::forward<Args>(args)...);

        try {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), diff, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), diff, new_data.GetAddress());
            }
        }
        catch (...) {
            std::destroy_n(new_data.GetAddress() + diff, 1);
            throw;
        }

        try {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + diff, size_ - diff, new_data.GetAddress() + diff + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress() + diff, size_ - diff, new_data.GetAddress() + diff + 1);
            }
        }
        catch (...) {
            std::destroy_n(new_data.GetAddress(), diff + 1);
            throw;
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);

        ++size_;
        return data_.GetAddress() + diff;
    }
}

template <typename T>
Vector<T>::iterator Vector<T>::Erase(const_iterator pos) {
    assert(pos >= begin() && pos < begin() + size_);
    size_t diff = pos - cbegin();
    std::move(data_.GetAddress() + diff + 1, end(), data_.GetAddress() + diff);
    std::destroy_n(end() - 1, 1);
    --size_;
    return data_.GetAddress() + diff;
}

template <typename T>
Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value) {
    return Emplace(pos, value);
}

template <typename T>
Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
    return Emplace(pos, std::forward<T>(value));
}

template <typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template <typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template <typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}

template <typename T>
T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template <typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }

    RawMemory<T> new_data(new_capacity);

    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }

    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}