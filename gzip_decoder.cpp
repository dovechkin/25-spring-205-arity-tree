#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Match
{
    size_t distance;
    size_t length;
    char next_char;

    Match(size_t d, size_t l) : distance(d), length(l), next_char('\0') {}
    Match(size_t d, size_t l, char ch) : distance(d), length(l), next_char(ch) {}
};

const string BTYPE_FIXED = "10";
const string END_OF_BLOCK = "0000000";
constexpr uint32_t BUF_SIZE = 65536;
constexpr uint8_t BYTE_SIZE = 8;

constexpr uint16_t LITERAL_CODE_OFFSET_1 = 0x30;
constexpr uint16_t LITERAL_CODE_OFFSET_2 = 0x190;
constexpr uint16_t LITERAL_THRESHOLD = 144;

unordered_map<string, tuple<uint16_t, uint8_t, uint16_t>> len_table = {
    {"0000001", {257, 0, 3}},    {"0000010", {258, 0, 4}},    {"0000011", {259, 0, 5}},    {"0000100", {260, 0, 6}},
    {"0000101", {261, 0, 7}},    {"0000110", {262, 0, 8}},    {"0000111", {263, 0, 9}},    {"0001000", {264, 0, 10}},
    {"0001001", {265, 1, 11}},   {"0001010", {266, 1, 13}},   {"0001011", {267, 1, 15}},   {"0001100", {268, 2, 17}},
    {"0001101", {269, 2, 21}},   {"0001110", {270, 2, 25}},   {"0001111", {271, 2, 29}},   {"0010000", {272, 3, 33}},
    {"0010001", {273, 3, 41}},   {"0010010", {274, 3, 49}},   {"0010011", {275, 3, 57}},   {"0010100", {276, 4, 65}},
    {"0010101", {277, 4, 81}},   {"0010110", {278, 4, 97}},   {"0010111", {279, 4, 113}},  {"11000000", {280, 5, 129}},
    {"11000001", {281, 5, 161}}, {"11000010", {282, 5, 193}}, {"11000011", {283, 5, 225}}, {"11000100", {284, 5, 257}},
    {"11000101", {285, 0, 258}},
};

unordered_map<string, tuple<uint8_t, uint8_t, uint16_t>> dist_table = {
    {"00000", {0, 0, 1}},       {"00001", {1, 0, 2}},       {"00010", {2, 0, 3}},      {"00011", {3, 0, 4}},
    {"00100", {4, 1, 5}},       {"00101", {5, 1, 7}},       {"00110", {6, 2, 9}},      {"00111", {7, 2, 13}},
    {"01000", {8, 3, 17}},      {"01001", {9, 3, 25}},      {"01010", {10, 4, 33}},    {"01011", {11, 4, 49}},
    {"01100", {12, 5, 65}},     {"01101", {13, 5, 97}},     {"01110", {14, 6, 129}},   {"01111", {15, 6, 193}},
    {"10000", {16, 7, 257}},    {"10001", {17, 7, 385}},    {"10010", {18, 8, 513}},   {"10011", {19, 8, 769}},
    {"10100", {20, 9, 1025}},   {"10101", {21, 9, 1537}},   {"10110", {22, 10, 2049}}, {"10111", {23, 10, 3073}},
    {"11000", {24, 11, 4097}},  {"11001", {25, 11, 6145}},  {"11010", {26, 12, 8193}}, {"11011", {27, 12, 12289}},
    {"11100", {28, 13, 16385}}, {"11101", {29, 13, 24577}},
};

unordered_map<string, char> lit_table;

void fill_lit_codes()
{
    lit_table.clear();

    for (uint8_t i = 0; i < LITERAL_THRESHOLD; ++i)
    {
        string code = bitset<8>(LITERAL_CODE_OFFSET_1 + i).to_string();
        lit_table[code] = static_cast<char>(i);
    }

    for (uint16_t i = LITERAL_THRESHOLD; i < 256; ++i)
    {
        string code = bitset<9>(LITERAL_CODE_OFFSET_2 + (i - LITERAL_THRESHOLD)).to_string();
        lit_table[code] = static_cast<char>(i);
    }
}

size_t get_distance(const string &src, size_t &pos)
{
    auto [_, extra_bits, dist] = dist_table[src.substr(pos, 5)];
    pos += 5;

    for (size_t i = 0; i < extra_bits; ++i)
        if (src[pos + i] == '1')
            dist += (1 << i);

    pos += extra_bits;

    return dist;
}

Match get_match(const string &src, size_t &pos, uint8_t huff_len)
{
    auto [_, extra_bits, len] = len_table[src.substr(pos, huff_len)];
    pos += 7;

    for (size_t i = 0; i < extra_bits; ++i)
        if (src[pos + i] == '1')
            len += (1 << i);

    pos += extra_bits;

    size_t dist = get_distance(src, pos);

    return Match(dist, len);
}

string unpack(const string &src)
{
    string unpacked;

    for (auto ch : src)
        for (uint8_t i = 0; i < 8; ++i)
            if (ch & (1 << i))
                unpacked.push_back('1');
            else
                unpacked.push_back('0');

    return unpacked;
}

vector<Match> get_matches(const string &src)
{
    vector<Match> matches;

    size_t pos = 0;

    bool is_final_block;
    string btype;
    string huff_code;
    do
    {
        is_final_block = (src[pos] == '1');
        pos += 1;
        btype = src.substr(pos, 2);
        pos += 2;

        while (src.substr(pos, 7) != END_OF_BLOCK)
        {
            if (len_table.find(src.substr(pos, 7)) != len_table.end())
            {
                Match match = get_match(src, pos, 7);
                matches.push_back(match);
            }
            else if (lit_table.find(src.substr(pos, 8)) != lit_table.end())
            {
                matches.emplace_back(0, 1, lit_table[src.substr(pos, 8)]);
                pos += 8;
            }
            else if (len_table.find(src.substr(pos, 8)) != len_table.end())
            {
                Match match = get_match(src, pos, 8);
                matches.push_back(match);
            }
            else if (lit_table.find(src.substr(pos, 9)) != lit_table.end())
            {
                matches.emplace_back(0, 1, lit_table[src.substr(pos, 9)]);
                pos += 9;
            }
        }

        pos += 7;

    } while (!is_final_block);

    return matches;
}

void generate_crc32_table(uint32_t table[256])
{
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        table[i] = crc;
    }
}

void read_header(istream &in, string &filename)
{
    char buffer[BUF_SIZE];
    in.read(buffer, 10);

    size_t pos = 10;
    if ((buffer[3] & (1 << 3)) != 0)
    {
        filename = "";
        char symbol;
        in >> symbol;
        while (symbol != '\x00')
        {
            filename.push_back(symbol);
            in >> symbol;
            ++pos;
        }
    }
}

void decode(istream &in, ostream &out)
{
    fill_lit_codes();

    streampos start_pos = in.tellg();
    in.seekg(0, std::ios::end);
    streampos end_of_file = in.tellg();
    size_t bytes_to_read = end_of_file - start_pos - 8;
    in.seekg(start_pos, std::ios::beg);

    char byte;
    string src;
    for (size_t i = 0; i < bytes_to_read; ++i)
    {
        in.read(&byte, 1);
        for (uint8_t j = 0; j < BYTE_SIZE; ++j)
            if (((byte >> j) & 1) == 1)
                src.push_back('1');
            else
                src.push_back('0');
    }

    cout << src << '\n';

    vector<Match> matches = get_matches(src);

    string result;
    for (auto [dist, len, ch] : matches)
    {
        if (dist == 0)
            result.push_back(ch);
        else
        {
            size_t start = result.size() - dist;
            for (size_t i = 0; i < len; ++i)
                result.push_back(result[start + i % dist]);
        }
    }

    uint32_t input_crc = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        char data;
        in.read(&data, 1);
        input_crc |= ((data & 0xFF) << i * 8);
    }

    uint32_t input_isize = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        char data;
        in.read(&data, 1);
        input_isize |= ((data & 0xFF) << i * 8);
    }

    uint32_t crc_table[256];
    generate_crc32_table(crc_table);

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < result.size(); ++i)
        crc = (crc >> 8) ^ crc_table[(crc ^ result[i]) & 0xFF];

    crc ^= 0xFFFFFFFF;

    cout << result << '\n';

    uint32_t isize = result.size();

    assert(crc == input_crc);
    assert(isize == input_isize);

    out << result;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Передано не верное количество аргументов!" << endl;
        return 1;
    }

    string filename = argv[1];

    ifstream input_file(filename, ios::binary);
    if (!input_file.is_open())
    {
        cerr << "Не удалось открыть файл для чтения!" << endl;
        return 1;
    }

    string output_name;
    read_header(input_file, output_name);

    if (output_name.empty())
        output_name = filename + ".ungz";
    else
    {
        size_t pos = filename.find_last_of('/');
        if (pos != string::npos)
            output_name = filename.substr(0, pos + 1) + output_name;
    }

    ofstream output_file(output_name);
    if (!output_file.is_open())
    {
        cerr << "Не удалось открыть файл для записи!" << endl;
        return 1;
    }

    decode(input_file, output_file);

    input_file.close();
    output_file.close();

    cout << "Файл успешно обработан!" << endl;
}