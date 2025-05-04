#include <iostream>
#include <stdexcept>

template <typename T> class Vector
{
  private:
    T *data; // Указатель на массив данных
    size_t capacity; // Вместимость массива
    size_t size; // Текущий размер массива

    // Увеличение вместимости массива
    void resize(size_t new_capacity)
    {
        T *new_data = new T[new_capacity];
        for (size_t i = 0; i < size; ++i) {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
        capacity = new_capacity;
    }

  public:
    // Конструктор по умолчанию
    Vector() : data(nullptr), capacity(0), size(0) {}

    // Конструктор с начальной вместимостью
    explicit Vector(size_t initial_capacity)
        : capacity(initial_capacity), size(0)
    {
        data = new T[capacity];
    }

    // Конструктор копирования
    Vector(const Vector &other) : capacity(other.capacity), size(other.size)
    {
        data = new T[capacity];
        for (size_t i = 0; i < size; ++i) {
            data[i] = other.data[i];
        }
    }

    // Конструктор перемещения
    Vector(Vector &&other) noexcept
        : data(other.data), capacity(other.capacity), size(other.size)
    {
        other.data = nullptr;
        other.capacity = 0;
        other.size = 0;
    }

    // Деструктор
    ~Vector() { delete[] data; }

    // Оператор присваивания копированием
    Vector &operator=(const Vector &other)
    {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            size = other.size;
            data = new T[capacity];
            for (size_t i = 0; i < size; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    // Оператор присваивания перемещением
    Vector &operator=(Vector &&other) noexcept
    {
        if (this != &other) {
            delete[] data;
            data = other.data;
            capacity = other.capacity;
            size = other.size;
            other.data = nullptr;
            other.capacity = 0;
            other.size = 0;
        }
        return *this;
    }

    // Добавление элемента в конец
    void push_back(const T &value)
    {
        if (size >= capacity) {
            resize(capacity == 0 ? 1 : capacity * 2);
        }
        data[size++] = value;
    }

    // Добавление элемента по индексу
    void insert(size_t index, const T &value)
    {
        if (index > size) {
            throw std::out_of_range("Index out of range");
        } else if (size >= capacity) {
            // resize(capacity == 0 ? 1 : capacity * 2);
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
            ++size;
            capacity = 2 * capacity;
        } else {
            for (size_t i = size; i > index; --i) {
                data[i] = data[i - 1];
            }
            data[index] = value;
            ++size;
        }
    }

    // Удаление элемента с конца
    void pop_back()
    {
        if (size == 0) {
            throw std::out_of_range("Vector is empty");
        }
        --size;
    }

    // Удаление элемента по индексу
    void erase(size_t index)
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        for (size_t i = index; i < size - 1; ++i) {
            data[i] = data[i + 1];
        }
        --size;
    }

    // Доступ к элементу по индексу
    T &operator[](size_t index)
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    // Константный доступ к элементу по индексу
    const T &operator[](size_t index) const
    {
        if (index >= size) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    // Получение текущего размера
    size_t get_size() const { return size; }

    // Получение текущей вместимости
    size_t get_capacity() const { return capacity; }

    class Iterator
    {
      private:
        T *ptr;

      public:
        explicit Iterator(T *p) : ptr(p) {}
        T &operator*() const { return *ptr; }
        T &operator->() const { return ptr; }

        Iterator &operator++()
        {
            ++ptr;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++ptr;
            return tmp;
        }

        bool operator==(Iterator &other) { return ptr == other.ptr; }
        bool operator!=(Iterator &other) { return ptr != other.ptr; }
    };

    class ConstIterator
    {
      private:
        const T *ptr;

      public:
        explicit ConstIterator(const T *p) : ptr(p) {}
        const T &operator*() const { return *ptr; }

        const T *operator->() const
        {
            return ptr;
        } // Исправлено: возвращаем указатель

        ConstIterator &operator++()
        {
            ++ptr;
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            ++ptr;
            return tmp; // Исправлено: возвращаем временный объект
        }

        // Добавляем const и правильные параметры
        bool operator==(const ConstIterator &other) const
        {
            return ptr == other.ptr;
        }
        bool operator!=(const ConstIterator &other) const
        {
            return ptr != other.ptr;
        } // Исправлено на !=
    };
    Iterator begin() { return Iterator(data); }
    Iterator end() { return Iterator(data + size); }
    ConstIterator begin() const { return ConstIterator(data); }

    ConstIterator end() const { return ConstIterator(data + size); }

    ConstIterator cbegin() const { return ConstIterator(data); }

    ConstIterator cend() const { return ConstIterator(data + size); }
};

int main()
{

    Vector<int> vec;

    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    std::cout << "Vector elements: ";
    for (const auto &elem : vec) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;

    const Vector<int> const_vec(vec);
    std::cout << "Const vector elements: ";
    for (const auto &elem : const_vec) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;

    std::cout << "Vector after push_back: ";
    for (size_t i = 0; i < vec.get_size(); ++i) {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;

    vec.insert(1, 15);
    std::cout << "Vector after insert at index 1: ";
    for (size_t i = 0; i < vec.get_size(); ++i) {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;

    vec.pop_back();
    std::cout << "Vector after pop_back: ";
    for (size_t i = 0; i < vec.get_size(); ++i) {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;

    vec.erase(1);
    std::cout << "Vector after erase at index 1: ";
    for (size_t i = 0; i < vec.get_size(); ++i) {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;

    try {
        std::cout << "Element at index 2: " << vec[2] << std::endl;
    } catch (const std::out_of_range &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    try {
        std::cout << "Element at index 10: " << vec[10] << std::endl;
    } catch (const std::out_of_range &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}