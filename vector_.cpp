

#include <iostream>
#include <stdexcept>

template <typename T> class Vector
{
  private:
    T *data;
    size_t capacity;
    size_t size;

    void resize(size_t new_capacity)
    {
        T *new_data = new T[new_capacity];
        for (size_t i = 0; i < size; ++i) {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
        capacity = new_capacity;
        delete[] new_data;
    }

  public:
    Vector() : data(nullptr), capacity(0), size(0) {}

    explicit Vector(size_t initial_capacity)
        : capacity(initial_capacity), size(0)
    {
        data = new T[initial_capacity];
    }

    Vector(const Vector &other) : capacity(other.capacity), size(other.size)
    {
        data = new T[other.capacity];
        for (size_t i = 0; i < size; ++i) {
            data[i] = other.data[i];
        }
    }

    Vector(Vector &other) noexcept
        : data(other.data), capacity(other.capacity), size(other.size)
    {
        other.data = nullptr;
        other.capacity = 0;
        other.size = 0;
    }

    ~Vector() { delete[] data; }

    void push_back(const T &value)
    {
        if (size >= capacity) {
            resize(capacity == 0 ? 1 : 2 * capacity);
        }
        data[size++] = value;
    }

    void insert(size_t index, const T &value)
    {
        if (index > size) {
            throw std::out_of_range("Index out of range");
        }
        if (size >= capacity) {
            // resize(capacity == 0 ? 1 : 2 * capacity);
            T *new_data = new T[2 * capacity];
            for (size_t i = 0; i < size; ++i) {
                new_data[i] = data[i];
                if (i == index) {
                    new_data[i] = value;
                }
                if (i > index) {
                    new_data[i] = data[i - 1];
                }
            }
            delete[] data;
            data = new_data;
            capacity = 2 * capacity;
            delete[] new_data;
        }
        for (size_t i = size; i > index; --i) {
            data[i] = data[i - 1];
        }
        data[index] = value;
        ++size;
    }

    void pop_back()
    {
        if (size == 0) {
            throw std::out_of_range("Index out of range");
        }
        size--;
    }

    void erase(size_t index)
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        for (size_t i = size; i < size - 1; ++i) {
            data[i] = data[i + 1];
        }
        size--;
    }

    T &operator[](size_t index)
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    const T &operator[](size_t index) const
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    size_t get_size() const { return size; }

    size_t get_capacity() const { return capacity; }
};