#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void decode(istream& in, ostream& out)
{
    char ch;
    while (in.read(&ch, 1))
    {
        int8_t byte = ch;
        if (byte < 0)
        {
            while (byte < 0)
            {
                in.read(&ch, 1);
                out.put(ch);
                ++byte;
            }
        }
        else
        {
            in.read(&ch, 1);
            while (byte > 0)
            {
                out.put(ch);
                --byte;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << "Не верное количество аргументов!" << endl;
    }

    ifstream input_file(argv[1]);
    ofstream output_file(argv[2]);

    if (!input_file || !output_file)
    {
        cerr << "Ошибка открытия файлов!" << endl;
        return 1;
    }

    decode(input_file, output_file);

    input_file.close();
    output_file.close();

    cout << "Готово!" << endl;
}