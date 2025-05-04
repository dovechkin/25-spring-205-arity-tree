#include <algorithm>
#include <stdexcept>
#include <utility>

template <typename T, size_t min_size> class Vector
{
    static_assert(min_size > 0, "min_size must be greater than zero");

    T static_data[min_size];
    struct DynamicData {
        T *ptr = nullptr;
        size_t capacity = 0;
    } dynamic_data;
    size_t current_size = 0;
    bool is_dynamic = false;

  public:
    Vector() = default;

    ~Vector()
    {
        if (is_dynamic) {
            delete[] dynamic_data.ptr;
        }
    }

    Vector(const Vector &other)
        : current_size(other.current_size), is_dynamic(other.is_dynamic)
    {
        if (is_dynamic) {
            dynamic_data.capacity = other.dynamic_data.capacity;
            dynamic_data.ptr = new T[dynamic_data.capacity];
            std::copy(other.dynamic_data.ptr,
                      other.dynamic_data.ptr + current_size, dynamic_data.ptr);
        } else {
            std::copy(other.static_data, other.static_data + current_size,
                      static_data);
        }
    }

    Vector &operator=(const Vector &other)
    {
        if (this != &other) {
            Vector temp(other);
            swap(temp);
        }
        return *this;
    }

    Vector(Vector &&other) noexcept
        : current_size(other.current_size), is_dynamic(other.is_dynamic)
    {
        if (is_dynamic) {
            dynamic_data = other.dynamic_data;
            other.dynamic_data = {nullptr, 0};
        } else {
            std::move(other.static_data, other.static_data + current_size,
                      static_data);
        }
        other.current_size = 0;
    }

    Vector &operator=(Vector &&other) noexcept
    {
        if (this != &other) {
            if (is_dynamic) {
                delete[] dynamic_data.ptr;
            }
            current_size = other.current_size;
            is_dynamic = other.is_dynamic;
            if (is_dynamic) {
                dynamic_data = other.dynamic_data;
                other.dynamic_data = {nullptr, 0};
            } else {
                std::move(other.static_data, other.static_data + current_size,
                          static_data);
            }
            other.current_size = 0;
        }
        return *this;
    }

    void swap(Vector &other) noexcept
    {
        using std::swap;
        if (is_dynamic && other.is_dynamic) {
            swap(dynamic_data.ptr, other.dynamic_data.ptr);
            swap(dynamic_data.capacity, other.dynamic_data.capacity);
        } else if (!is_dynamic && !other.is_dynamic) {
            for (size_t i = 0; i < std::max(current_size, other.current_size);
                 ++i) {
                swap(static_data[i], other.static_data[i]);
            }
        } else {
            Vector &dynamic_vec = is_dynamic ? *this : other;
            Vector &static_vec = is_dynamic ? other : *this;
            DynamicData temp_dynamic = dynamic_vec.dynamic_data;
            dynamic_vec.dynamic_data = {nullptr, 0};
            dynamic_vec.is_dynamic = false;
            for (size_t i = 0; i < static_vec.current_size; ++i) {
                static_vec.static_data[i] = std::move(
                    static_vec.is_dynamic ? static_vec.dynamic_data.ptr[i]
                                          : static_vec.static_data[i]);
            }
            static_vec.is_dynamic = true;
            static_vec.dynamic_data = temp_dynamic;
        }
        swap(current_size, other.current_size);
        swap(is_dynamic, other.is_dynamic);
    }

    size_t size() const { return current_size; }
    size_t capacity() const
    {
        return is_dynamic ? dynamic_data.capacity : min_size;
    }

    void push_back(const T &value)
    {
        if (current_size < capacity()) {
            if (is_dynamic) {
                dynamic_data.ptr[current_size] = value;
            } else {
                static_data[current_size] = value;
            }
            ++current_size;
        } else {
            if (!is_dynamic) {
                size_t new_cap = min_size * 2;
                T *new_ptr = new T[new_cap];
                std::copy(static_data, static_data + current_size, new_ptr);
                new_ptr[current_size] = value;
                dynamic_data.ptr = new_ptr;
                dynamic_data.capacity = new_cap;
                is_dynamic = true;
                ++current_size;
            } else {
                size_t new_cap = dynamic_data.capacity * 2;
                T *new_ptr = new T[new_cap];
                std::copy(dynamic_data.ptr, dynamic_data.ptr + current_size,
                          new_ptr);
                new_ptr[current_size] = value;
                delete[] dynamic_data.ptr;
                dynamic_data.ptr = new_ptr;
                dynamic_data.capacity = new_cap;
                ++current_size;
            }
        }
    }

    void pop_back()
    {
        if (current_size == 0)
            return;
        --current_size;
        if (is_dynamic && current_size < min_size) {
            T *old_ptr = dynamic_data.ptr;
            std::copy(old_ptr, old_ptr + current_size, static_data);
            delete[] old_ptr;
            dynamic_data = {nullptr, 0};
            is_dynamic = false;
        }
    }

    T &operator[](size_t index)
    {
        if (index >= current_size)
            throw std::out_of_range("Index out of range");
        return is_dynamic ? dynamic_data.ptr[index] : static_data[index];
    }

    const T &operator[](size_t index) const
    {
        if (index >= current_size)
            throw std::out_of_range("Index out of range");
        return is_dynamic ? dynamic_data.ptr[index] : static_data[index];
    }

    void reserve(size_t new_cap)
    {
        if (new_cap <= capacity())
            return;
        if (!is_dynamic) {
            T *new_ptr = new T[new_cap];
            std::copy(static_data, static_data + current_size, new_ptr);
            dynamic_data.ptr = new_ptr;
            dynamic_data.capacity = new_cap;
            is_dynamic = true;
        } else {
            T *new_ptr = new T[new_cap];
            std::copy(dynamic_data.ptr, dynamic_data.ptr + current_size,
                      new_ptr);
            delete[] dynamic_data.ptr;
            dynamic_data.ptr = new_ptr;
            dynamic_data.capacity = new_cap;
        }
    }

    void resize(size_t new_size, const T &value = T())
    {
        if (new_size < current_size) {
            current_size = new_size;
            if (is_dynamic && current_size < min_size) {
                T *old_ptr = dynamic_data.ptr;
                std::copy(old_ptr, old_ptr + current_size, static_data);
                delete[] old_ptr;
                dynamic_data = {nullptr, 0};
                is_dynamic = false;
            }
        } else if (new_size > current_size) {
            reserve(new_size);
            while (current_size < new_size) {
                if (is_dynamic) {
                    dynamic_data.ptr[current_size] = value;
                } else {
                    static_data[current_size] = value;
                }
                ++current_size;
            }
        }
    }

    void shrink_to_fit()
    {
        if (!is_dynamic || current_size == 0)
            return;
        if (current_size <= min_size) {
            T *old_ptr = dynamic_data.ptr;
            std::copy(old_ptr, old_ptr + current_size, static_data);
            delete[] old_ptr;
            dynamic_data = {nullptr, 0};
            is_dynamic = false;
        } else if (dynamic_data.capacity > current_size) {
            T *new_ptr = new T[current_size];
            std::copy(dynamic_data.ptr, dynamic_data.ptr + current_size,
                      new_ptr);
            delete[] dynamic_data.ptr;
            dynamic_data.ptr = new_ptr;
            dynamic_data.capacity = current_size;
        }
    }
};