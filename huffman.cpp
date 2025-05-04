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

struct Node {
    int weight;
    int symbol;
    int number;
    Node *left;
    Node *right;
    Node *parent;

    Node(int w, int s, int n, Node *l = nullptr, Node *r = nullptr,
         Node *p = nullptr)
        : weight(w), symbol(s), number(n), left(l), right(r), parent(p)
    {
    }
};

class AdaptiveHuffmanVitter
{
  private:
    const int MAX_NUMBER = 512;

    Node *root;
    Node *NYT;
    map<int, Node *> symbolTable;
    map<int, Node *> numberTable;

    string getCode(Node *node)
    {
        string code;
        while (node->parent != nullptr) {
            if (node->parent->left == node) {
                code = "0" + code;
            } else {
                code = "1" + code;
            }
            node = node->parent;
        }
        return code;
    }

    Node *findLeaderInBlock(Node *node)
    {
        int weight = node->weight;
        Node *leader = node;

        for (auto &pair : numberTable) {
            Node *current = pair.second;
            if (current->weight == weight && current->number > leader->number) {
                leader = current;
            }
        }
        return leader;
    }

    void swapNodes(Node *node1, Node *node2)
    {
        if (node1->parent == nullptr || node2->parent == nullptr)
            return;

        if (node1->parent->left == node1) {
            node1->parent->left = node2;
        } else {
            node1->parent->right = node2;
        }

        if (node2->parent->left == node2) {
            node2->parent->left = node1;
        } else {
            node2->parent->right = node1;
        }

        Node *temp = node1->parent;
        node1->parent = node2->parent;
        node2->parent = temp;

        swap(node1->number, node2->number);
        numberTable[node1->number] = node1;
        numberTable[node2->number] = node2;
    }

    void updateTree(int symbol)
    {
        Node *nodeToUpdate;

        if (symbolTable.find(symbol) != symbolTable.end()) {
            nodeToUpdate = symbolTable[symbol];
        } else {
            Node *newSymbolNode = new Node(1, symbol, NYT->number - 1);
            Node *newNYT = new Node(0, -1, NYT->number - 2);

            newSymbolNode->parent = NYT;
            newNYT->parent = NYT;
            NYT->left = newNYT;
            NYT->right = newSymbolNode;
            NYT->symbol = -2;

            symbolTable[symbol] = newSymbolNode;
            numberTable[newSymbolNode->number] = newSymbolNode;
            numberTable[newNYT->number] = newNYT;

            nodeToUpdate = NYT;
            NYT = newNYT;
        }

        while (nodeToUpdate != nullptr) {
            Node *leader = findLeaderInBlock(nodeToUpdate);

            if (leader != nodeToUpdate && leader != nodeToUpdate->parent) {
                swapNodes(nodeToUpdate, leader);
            }

            nodeToUpdate->weight++;

            nodeToUpdate = nodeToUpdate->parent;
        }
    }

    void deleteTree(Node *node)
    {
        if (node == nullptr) {
            return;
        }

        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

  public:
    AdaptiveHuffmanVitter()
    {
        NYT = new Node(0, -1, MAX_NUMBER);
        root = NYT;
        numberTable[NYT->number] = NYT;
    }

    ~AdaptiveHuffmanVitter() { deleteTree(root); }

    string encode(int symbol)
    {
        string code;

        if (symbolTable.find(symbol) != symbolTable.end()) {
            Node *node = symbolTable[symbol];
            code = getCode(node);
        } else {
            code = getCode(NYT);
            bitset<8> bits(symbol);
            code += bits.to_string();
        }
        updateTree(symbol);

        return code;
    }

    vector<int> decode(const string &bitString)
    {
        vector<int> decodedSymbols;
        Node *currentNode = root;
        size_t pos = 0;

        while (pos < bitString.size()) {
            if (currentNode == NYT) {
                if (pos + 8 > bitString.size()) {
                    throw runtime_error(
                        "Invalid bit string - not enough bits for symbol");
                }

                string symbolBits = bitString.substr(pos, 8);
                pos += 8;

                int symbol = bitset<8>(symbolBits).to_ulong();
                decodedSymbols.push_back(symbol);
                updateTree(symbol);
                currentNode = root;
            } else if (currentNode->left == nullptr &&
                       currentNode->right == nullptr) {
                decodedSymbols.push_back(currentNode->symbol);

                updateTree(currentNode->symbol);
                currentNode = root;
            } else {
                if (bitString[pos] == '0') {
                    currentNode = currentNode->left;
                } else {
                    currentNode = currentNode->right;
                }
                pos++;
            }
        }

        if (currentNode != root && currentNode != NYT &&
            currentNode->left == nullptr && currentNode->right == nullptr) {
            decodedSymbols.push_back(currentNode->symbol);
            updateTree(currentNode->symbol);
        }

        return decodedSymbols;
    }
};

int main()
{
    AdaptiveHuffmanVitter encoder;

    string input = "absuuosuggugeghhsjgffdhhhhhsfccdvwgfdfgd";
    cout << "Encoding '" << input << "':" << endl;

    string encoded;
    for (char c : input) {
        string code = encoder.encode(c);
        encoded += code;
        cout << c << ": " << code << endl;
    }
    cout << "Full encoded: " << encoded << endl;

    AdaptiveHuffmanVitter decoder;
    vector<int> decoded = decoder.decode(encoded);

    stringstream output;
    for (int c : decoded) {
        output << static_cast<char>(c);
    }

    cout << "Decoded: " << output.str() << '\n';

    assert(input == output.str() && "Декодирование нарушено!");

    return 0;
}