#include <iostream>
#include <stdexcept>

template <size_t chunk_size, typename T> class chunked_list
{
    static_assert(chunk_size > 0, "chunk_size must be positive");

  private:
    struct Chunk {
        T data[chunk_size];
        Chunk *next;
        size_t current_size;

        Chunk() : next(nullptr), current_size(0) {}
    };

    Chunk *head;
    Chunk *tail;
    size_t total_size;

  public:
    chunked_list() : head(nullptr), tail(nullptr), total_size(0) {}

    ~chunked_list()
    {
        Chunk *current = head;
        Chunk *previous = head;
        while (current) {
            current = current->next;
            delete previous;
            previous = current;
        }
    }

    chunked_list(const chunked_list &other)
        : head(nullptr), tail(nullptr), total_size(0)
    {
        Chunk *other_current = other.head;
        while (other_current != nullptr) {
            for (int i = 0; i < other_current->current_size; ++i) {
                push_back(other_current->data[i]);
            }
            other_current = other_current->next;
        }
    }
    chunked_list &operator=(const chunked_list &other)
    {
        if (this != &other) {
            Chunk *current = head;
            while (current) {
                Chunk *next = current->next;
                delete current;
                current = next;
            }
            head = tail = nullptr;
            total_size = 0;

            Chunk *other_current = other.head;
            while (other_current) {
                for (size_t i = 0; i < other_current->current_size; ++i) {
                    push_back(other_current->data[i]);
                }
                other_current = other_current->next;
            }
        }
        return *this;
    }

    void push_back(const T &value)
    {
        if (!tail) {
            head = tail = new Chunk();
        }
        if (tail->current_size == chunk_size) {
            tail->next = new Chunk();
            tail = tail->next;
        }
        tail->data[tail->current_size++] = value;
        ++total_size;
    }
    size_t size() const { return total_size; }
    T &operator[](size_t index)
    {
        if (index >= total_size) {
            throw std::out_of_range("Index out of range");
        }
        Chunk *current = head;
        size_t remaining = index;
        while (current && remaining >= current->current_size) {
            remaining -= current->current_size;
            current = current->next;
        }
        return current->data[remaining];
    }

    const T &operator[](size_t index) const
    {
        if (index >= total_size) {
            throw std::out_of_range("Index out of range");
        }
        Chunk *current = head;
        size_t remaining = index;
        while (current && remaining >= current->current_size) {
            remaining -= current->current_size;
            current = current->next;
        }
        return current->data[remaining];
    }
};

int main()
{
    // Создаем список с чанками по 3 элемента
    chunked_list<3, int> numbers;

    // Добавляем элементы (больше, чем размер чанка)
    for (int i = 0; i < 10; ++i) {
        numbers.push_back(i * 10);
    }

    // Проверяем размер списка
    std::cout << "Total elements: " << numbers.size() << "\n\n"; // 10

    // Выводим элементы через индекс
    std::cout << "Elements: ";
    for (size_t i = 0; i < numbers.size(); ++i) {
        std::cout << numbers[i] << " "; // 0 10 20 30 40 50 60 70 80 90
    }
    std::cout << "\n\n";

    // Тест копирования
    chunked_list<3, int> numbers_copy = numbers;
    numbers_copy.push_back(100);
    std::cout << "Copied list last element: " << numbers_copy[10]
              << "\n"; // 100
    std::cout << "Original list size: " << numbers.size() << "\n\n"; // 10

    // Тест с другим типом данных
    chunked_list<2, std::string> words;
    words.push_back("Hello");
    words.push_back("World");
    words.push_back("Chunked");
    words.push_back("List");

    std::cout << "Strings: ";
    for (size_t i = 0; i < words.size(); ++i) {
        std::cout << words[i] << " "; // Hello World Chunked List
    }
    std::cout << "\n\n";

    // Тест исключения
    try {
        std::cout << "Trying to access index 100: ";
        std::cout << numbers[100] << "\n"; // Исключение
    } catch (const std::out_of_range &e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    return 0;
}