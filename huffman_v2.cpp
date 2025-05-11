#include <bitset>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

// Узел дерева Хаффмана
struct Node
{
    int weight;   // Вес узла (частота)
    int symbol;   // Символ (для листьев)
    int number;   // Номер узла (по алгоритму Vitter)
    Node *left;   // Левый потомок
    Node *right;  // Правый потомок
    Node *parent; // Родитель

    Node(int w, int s, int n, Node *l = nullptr, Node *r = nullptr, Node *p = nullptr)
        : weight(w), symbol(s), number(n), left(l), right(r), parent(p)
    {
    }
};

// Класс для адаптивного кодирования Хаффмана (Vitter)
class AdaptiveHuffmanVitter
{
  private:
    const int MAX_NUMBER = 512; // Кол-во узлов в дереве = 2 * размер алфавита

    Node *root;                   // Корень дерева
    Node *NYT;                    // Специальный NYT-узел (Not Yet Transmitted)
    map<int, Node *> symbolTable; // Таблица символов
    map<int, Node *> numberTable; // Таблица блоков

    // Вспомогательная функция для получения кода символа
    string getCode(Node *node)
    {
        string code;
        while (node->parent != nullptr)
        {
            if (node->parent->left == node)
            {
                code = "0" + code;
            }
            else
            {
                code = "1" + code;
            }
            node = node->parent;
        }
        return code;
    }

    // Найти узел с наибольшим номером в блоке с тем же весом
    Node *findLeaderInBlock(Node *node)
    {
        int weight = node->weight;
        Node *leader = node;

        // Ищем узел с максимальным номером в том же блоке весов
        for (auto &pair : numberTable)
        {
            Node *current = pair.second;
            if (current->weight == weight && current->number > leader->number)
            {
                leader = current;
            }
        }
        return leader;
    }

    // Обменять два узла в дереве (кроме их потомков)
    void swapNodes(Node *node1, Node *node2)
    {
        if (node1->parent == nullptr || node2->parent == nullptr)
            return;

        // Меняем местами в родительских узлах
        if (node1->parent->left == node1)
        {
            node1->parent->left = node2;
        }
        else
        {
            node1->parent->right = node2;
        }

        if (node2->parent->left == node2)
        {
            node2->parent->left = node1;
        }
        else
        {
            node2->parent->right = node1;
        }

        // Меняем родителей
        Node *temp = node1->parent;
        node1->parent = node2->parent;
        node2->parent = temp;

        // Меняем номера
        swap(node1->number, node2->number);
        numberTable[node1->number] = node1;
        numberTable[node2->number] = node2;
    }

    // Обновить дерево после добавления символа
    void updateTree(int symbol)
    {
        Node *nodeToUpdate;

        if (symbolTable.find(symbol) != symbolTable.end())
        {
            // Символ уже встречался
            nodeToUpdate = symbolTable[symbol];
        }
        else
        {
            // Новый символ - создаем новый узел
            Node *newSymbolNode = new Node(1, symbol, NYT->number - 1);
            Node *newNYT = new Node(0, -1, NYT->number - 2);

            // Настраиваем связи
            newSymbolNode->parent = NYT;
            newNYT->parent = NYT;
            NYT->left = newNYT;
            NYT->right = newSymbolNode;
            NYT->symbol = -2; // Теперь это не NYT узел

            // Обновляем таблицы
            symbolTable[symbol] = newSymbolNode;
            numberTable[newSymbolNode->number] = newSymbolNode;
            numberTable[newNYT->number] = newNYT;

            nodeToUpdate = NYT;
            NYT = newNYT;
        }

        // Проходим по дереву снизу вверх
        while (nodeToUpdate != nullptr)
        {
            // Находим лидера в блоке
            Node *leader = findLeaderInBlock(nodeToUpdate);

            // Если лидер не текущий узел и не родитель, меняем их местами
            if (leader != nodeToUpdate && leader != nodeToUpdate->parent)
            {
                swapNodes(nodeToUpdate, leader);
            }

            // Увеличиваем вес
            nodeToUpdate->weight++;

            // Переходим к родителю
            nodeToUpdate = nodeToUpdate->parent;
        }
    }

    // Рекурсивное удаление дерева
    void deleteTree(Node *node)
    {
        if (node == nullptr)
        {
            return;
        }

        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

  public:
    AdaptiveHuffmanVitter()
    {
        // Инициализация с NYT-узлом
        NYT = new Node(0, -1, MAX_NUMBER); // Начинаем с максимального номера
        root = NYT;
        numberTable[NYT->number] = NYT;
    }

    ~AdaptiveHuffmanVitter() { deleteTree(root); }

    // Кодирование символа
    string encode(int symbol)
    {
        string code;

        if (symbolTable.find(symbol) != symbolTable.end())
        {
            // Символ уже встречался
            Node *node = symbolTable[symbol];
            code = getCode(node);
        }
        else
        {
            // Новый символ - код NYT + бинарное представление символа
            code = getCode(NYT);
            bitset<8> bits(symbol);
            code += bits.to_string();
        }

        // Обновляем дерево
        updateTree(symbol);

        return code;
    }

    // Декодирование строки битов
    vector<int> decode(const string &bitString)
    {
        vector<int> decodedSymbols;
        Node *currentNode = root;
        size_t pos = 0;

        while (pos < bitString.size())
        {
            if (currentNode == NYT)
            {
                // Декодируем новый символ
                if (pos + 8 > bitString.size())
                {
                    throw runtime_error("Invalid bit string - not enough bits for symbol");
                }

                string symbolBits = bitString.substr(pos, 8);
                pos += 8;

                int symbol = bitset<8>(symbolBits).to_ulong();
                decodedSymbols.push_back(symbol);

                // Обновляем дерево
                updateTree(symbol);
                currentNode = root;
            }
            else if (currentNode->left == nullptr && currentNode->right == nullptr)
            {
                // Лист - нашли символ
                decodedSymbols.push_back(currentNode->symbol);

                // Обновляем дерево
                updateTree(currentNode->symbol);
                currentNode = root;
            }
            else
            {
                // Продолжаем идти по дереву
                if (bitString[pos] == '0')
                {
                    currentNode = currentNode->left;
                }
                else
                {
                    currentNode = currentNode->right;
                }
                pos++;
            }
        }

        // Проверяем, если остановились на листе
        if (currentNode != root && currentNode != NYT && currentNode->left == nullptr && currentNode->right == nullptr)
        {
            decodedSymbols.push_back(currentNode->symbol);
            updateTree(currentNode->symbol);
        }

        return decodedSymbols;
    }
};

int main()
{
    const char KEY = '#';

    AdaptiveHuffmanVitter encoder;

    string input;
    char ch;
    while ((ch = getchar()) != KEY)
    {
        input.push_back(ch);
    }

    // Пример кодирования
    cout << "Encoding '" << input << "':" << endl;

    string encoded;
    for (char c : input)
    {
        string code = encoder.encode(c);
        encoded += code;
        cout << c << ": " << code << endl;
    }
    cout << "Full encoded: " << encoded << endl;

    // Пример декодирования
    AdaptiveHuffmanVitter decoder;
    vector<int> decoded = decoder.decode(encoded);

    stringstream output;
    for (int c : decoded)
    {
        output << static_cast<char>(c);
    }

    cout << "Decoded: " << output.str() << '\n';

    assert(input == output.str() && "Декодирование нарушено!");

    return 0;
}