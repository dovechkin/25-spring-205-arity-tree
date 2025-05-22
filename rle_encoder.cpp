#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void encode(istream &in, ostream &out)
{
    vector<pair<int64_t, char>> seq;

    char byte;
    size_t n_repeat = 0;
    char curr_char = 0;
    while (in.read(&byte, 1))
    {
        if (curr_char != byte && curr_char != '\0')
        {
            seq.emplace_back(n_repeat, curr_char);
            n_repeat = 0;
        }

        curr_char = byte;
        ++n_repeat;
    }

    if (curr_char != '\0')
        seq.emplace_back(n_repeat, curr_char);

    vector<pair<int64_t, string>> dense_seq;
    int64_t len;
    string uniq;
    for (auto [sz, ch] : seq)
    {
        if (sz > 1)
        {
            if (!uniq.empty())
            {
                len = -uniq.size();
                dense_seq.emplace_back(len, uniq);
                uniq.clear();
            }

            string str_;
            str_.push_back(ch);
            dense_seq.emplace_back(sz, str_);
        }
        else
        {
            uniq.push_back(ch);
        }
    }

    if (!uniq.empty())
    {
        len = -uniq.size();
        dense_seq.emplace_back(len, uniq);
    }

    const int8_t MIN = -128;
    const int8_t MAX = 127;
    for (auto [sz, str] : dense_seq)
    {
        while (sz != 0)
        {
            int8_t code;
            if (sz < 0)
                code = (sz < MIN) ? MIN : sz;
            else
                code = (sz > MAX) ? MAX : sz;
            sz -= code;

            if (code > 0)
            {
                out << code << str;
            }
            else
            {
                out << code << str.substr(0, -code);
                str = str.substr(-code);
            }
        }
    }
}

int main(int argc, char *argv[])
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

    encode(input_file, output_file);

    input_file.close();
    output_file.close();

    cout << "Готово!" << endl;
}