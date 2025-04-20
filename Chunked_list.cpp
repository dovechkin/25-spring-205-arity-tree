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

    void insert(size_t index, const T &value)
    {
        if (index > total_size) {
            throw std::out_of_range("Index out of range");
        }
        if (index == total_size) {
            push_back(value);
        } else {
            size_t numchunk = index / chunk_size;
            size_t index_in_chunk = index % chunk_size;
            Chunk *current = head;
            for (int i = 0; i < numchunk; ++i) {
                current = current->next;
            }
            if (index_in_chunk > current->current_size) {
                current->data[index_in_chunk] = value;
                current->current_size += 1;
            } else if (current->current_size < chunk_size) {
                if (current->current_size < total_size)
                    for (int i = index_in_chunk; i < current->current_size;
                         ++i) {
                        current->data[i + 1] = current->data[i];
                        current->current_size += 1;
                    }
                current->data[index_in_chunk] = value;
                current->current_size += 1;
            } else {
                Chunk *new_chunk = new Chunk();
                size_t elements_to_move =
                    current->current_size - index_in_chunk;

                for (size_t i = 0; i < elements_to_move; ++i) {
                    new_chunk->data[i] = current->data[index_in_chunk + i];
                    new_chunk->current_size++;
                }
                current->current_size = index_in_chunk;

                current->data[current->current_size++] = value;
                total_size++;

                new_chunk->next = current->next;
                current->next = new_chunk;

                if (current == tail) {
                    tail = new_chunk;
                }
            }
        }
    }
    void erase(size_t index)
    {
        if (index >= total_size) {
            throw std::out_of_range("Index out of range");
        }

        size_t numchunk = index / chunk_size;
        size_t index_in_chunk = index % chunk_size;
        Chunk *current = head;
        for (int i = 0; i < numchunk; ++i) {
            current = current->next;
        }

        for (size_t i = index_in_chunk; i < current->current_size - 1; ++i) {
            current->data[i] = current->data[i + 1];
        }
        current->current_size--;
        total_size--;

        if (current->current_size == 0) {
            Chunk *prev = nullptr;
            Chunk *tmp = head;
            while (tmp != current) {
                prev = tmp;
                tmp = tmp->next;
            }

            if (prev) {
                prev->next = current->next;
            } else {
                head = current->next;
            }

            if (current == tail) {
                tail = prev;
            }

            delete current;
        }
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
    chunked_list<3, int> numbers;

    for (int i = 0; i < 10; ++i) {
        numbers.push_back(i * 10);
    }

    std::cout << "Total elements: " << numbers.size() << "\n\n"; // 10

    std::cout << "Elements: ";
    for (size_t i = 0; i < numbers.size(); ++i) {
        std::cout << numbers[i] << " "; // 0 10 20 30 40 50 60 70 80 90
    }
    std::cout << "\n\n";

    numbers.erase(3);

    numbers.insert(1, 30);
    numbers.insert(10, 100);

    std::cout << "Total elements: " << numbers.size() << "\n\n"; // 10

    std::cout << "Elements: ";
    for (size_t i = 0; i < numbers.size(); ++i) {
        std::cout << numbers[i] << " "; // 0 10 20 30 40 50 60 70 80 90
    }
    std::cout << "\n\n";

    chunked_list<3, int> numbers_copy = numbers;
    numbers_copy.push_back(100);
    std::cout << "Copied list last element: " << numbers_copy[10]
              << "\n"; // 100
    std::cout << "Original list size: " << numbers.size() << "\n\n"; // 10

    chunked_list<2, std::string> words;
    words.push_back("Hello");
    words.push_back("World");
    words.push_back("Chunked");
    words.push_back("List");

    std::cout << "Strings: ";
    for (size_t i = 0; i < words.size(); ++i) {
        std::cout << words[i] << " ";
    }
    std::cout << "\n\n";

    try {
        std::cout << "Trying to access index 100: ";
        std::cout << numbers[100] << "\n";
    } catch (const std::out_of_range &e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    return 0;
}