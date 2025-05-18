#include <algorithm>
#include <bitset>
#include <cassert>
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

class Encoder
{
   private:
    const size_t BLOCK_SIZE = 65536;
    const size_t WINDOW_SIZE = 32768;
    const u_int16_t MIN_MATCH_LEN = 3;
    const u_int16_t MAX_MATCH_LEN = 258;
    const string BTYPE_FIXED = "10";  // reverse order for packing
    const string END_OF_BLOCK = "0000000";

    // base_len - code, +bits, huff_code
    map<u_int16_t, tuple<u_int16_t, u_int8_t, string>> len_table = {
        {3, {257, 0, "0000001"}},    {4, {258, 0, "0000010"}},    {5, {259, 0, "0000011"}},
        {6, {260, 0, "0000100"}},    {7, {261, 0, "0000101"}},    {8, {262, 0, "0000110"}},
        {9, {263, 0, "0000111"}},    {10, {264, 0, "0001000"}},   {11, {265, 1, "0001001"}},
        {13, {266, 1, "0001010"}},   {15, {267, 1, "0001011"}},   {17, {268, 2, "0001100"}},
        {21, {269, 2, "0001101"}},   {25, {270, 2, "0001110"}},   {29, {271, 2, "0001111"}},
        {33, {272, 3, "0010000"}},   {41, {273, 3, "0010001"}},   {49, {274, 3, "0010010"}},
        {57, {275, 3, "0010011"}},   {65, {276, 4, "0010100"}},   {81, {277, 4, "0010101"}},
        {97, {278, 4, "0010110"}},   {113, {279, 4, "0010111"}},  {129, {280, 5, "11000000"}},
        {161, {281, 5, "11000001"}}, {193, {282, 5, "11000010"}}, {225, {283, 5, "11000011"}},
        {257, {284, 5, "11000100"}}, {258, {285, 0, "11000101"}},
    };

    // base_dist - code, +bits, huff_code
    map<u_int16_t, tuple<u_int8_t, u_int8_t, string>> dist_table = {
        {1, {0, 0, "00000"}},       {2, {1, 0, "00001"}},       {3, {2, 0, "00010"}},      {4, {3, 0, "00011"}},
        {5, {4, 1, "00100"}},       {7, {5, 1, "00101"}},       {9, {6, 2, "00110"}},      {13, {7, 2, "00111"}},
        {17, {8, 3, "01000"}},      {25, {9, 3, "01001"}},      {33, {10, 4, "01010"}},    {49, {11, 4, "01011"}},
        {65, {12, 5, "01100"}},     {97, {13, 5, "01101"}},     {129, {14, 6, "01110"}},   {193, {15, 6, "01111"}},
        {257, {16, 7, "10000"}},    {385, {17, 7, "10001"}},    {513, {18, 8, "10010"}},   {769, {19, 8, "10011"}},
        {1025, {20, 9, "10100"}},   {1537, {21, 9, "10101"}},   {2049, {22, 10, "10110"}}, {3073, {23, 10, "10111"}},
        {4097, {24, 11, "11000"}},  {6145, {25, 11, "11001"}},  {8193, {26, 12, "11010"}}, {12289, {27, 12, "11011"}},
        {16385, {28, 13, "11100"}}, {24577, {29, 13, "11101"}},
    };

    vector<Match> find_matches(const string& src)
    {
        vector<Match> matches;

        size_t pos = 0;
        while (pos < src.size())
        {
            Match best_match(0, 1, src[pos]);
            size_t start = (pos >= WINDOW_SIZE) ? pos - WINDOW_SIZE : 0;

            for (size_t i = start; i < pos; ++i)
            {
                size_t match_len = 0;
                while (match_len < MAX_MATCH_LEN && pos + match_len < src.size() &&
                       src[start + match_len] == src[pos + match_len])
                    ++match_len;

                if (match_len >= MIN_MATCH_LEN && match_len > best_match.length)
                {
                    best_match = Match(pos - start, match_len, src[pos]);
                }
            }

            if (best_match.length >= MIN_MATCH_LEN)
            {
                matches.push_back(best_match);
                pos += best_match.length;
            }
            else
            {
                matches.emplace_back(0, 1, src[pos]);
                ++pos;
            }
        }

        return matches;
    }

    vector<vector<Match>> split_into_blocks(const vector<Match>& matches)
    {
        vector<vector<Match>> blocks;

        size_t curr_size = 0;
        vector<Match> block;
        for (const auto& match : matches)
        {
            if (curr_size + match.length > BLOCK_SIZE)
            {
                blocks.push_back(block);
                block.clear();
                curr_size = 0;
            }

            block.push_back(match);
            curr_size += match.length;
        }

        if (curr_size > 0)
        {
            blocks.push_back(block);
        }

        return blocks;
    }

    string get_literal_fixed_code(char ch)
    {
        uint code;
        string result;
        if (static_cast<u_int8_t>(ch) < 144)
        {
            code = 0x30 + ch;
            result = bitset<8>(code).to_string();
        }
        else
        {
            code = 0x190 + (ch - 144);
            result = bitset<9>(code).to_string();
        }

        return result;
    }

    string get_distance_fixed_code(u_int16_t dist)
    {
        auto it = dist_table.upper_bound(dist);
        --it;

        u_int16_t base_dist = it->first;
        auto [code, bits, huff] = it->second;

        string code_str = huff;
        u_int16_t dist_diff = dist - base_dist;
        for (u_int8_t i = 0; i < bits; ++i)
        {
            char bit = ((dist_diff >> i) & 1) ? '1' : '0';
            code_str.push_back(bit);
        }

        return code_str;
    }

    string get_length_fixed_code(size_t len)
    {
        auto it = len_table.upper_bound(len);
        --it;

        u_int16_t base_len = it->first;
        auto [code, bits, huff] = it->second;

        string code_str = huff;
        u_int8_t len_diff = len - base_len;
        for (u_int8_t i = 0; i < bits; ++i)
        {
            char bit = ((len_diff >> i) & 1) ? '1' : '0';
            code_str.push_back(bit);
        }

        return code_str;
    }

    string fixed_huffman_encode(const vector<Match>& matches, size_t& size)
    {
        ostringstream oss;

        oss << BTYPE_FIXED;
        size += 2;

        for (const Match& match : matches)
        {
            if (match.distance == 0)
            {
                string lit_code = get_literal_fixed_code(match.next_char);
                oss << lit_code;
                size += lit_code.size();
            }
            else
            {
                string len_code = get_length_fixed_code(match.length);
                string dist_code = get_distance_fixed_code(match.distance);
                oss << len_code << dist_code;
                size += len_code.size() + dist_code.size();
            }
        }

        oss << END_OF_BLOCK;
        size += 7;

        return oss.str();
    }

    string pack(const string& src)
    {
        string packed;

        size_t pos = 0;
        u_int8_t data = 0;

        while (pos < src.size())
        {
            if (pos % 8 == 0 && pos != 0)
            {
                packed.push_back(data);
                data = 0;
            }

            if (src[pos] == '1')
                data |= (1 << (pos % 8));

            ++pos;
        }
        packed.push_back(data);

        return packed;
    }

   public:
    string encode(const string& src)
    {
        // находим все совпадения и переводим исходную строку в вектор Match
        vector<Match> matches = find_matches(src);

        // разбиваем полученный вектор на вектор блоков по длине исходной последовательности
        vector<vector<Match>> blocks = split_into_blocks(matches);

        // cout << "1:\t";
        // for (const auto& b : blocks)
        // {
        //     for (const auto& m : b)
        //         if (m.distance == 0)
        //             cout << m.next_char;
        //         else
        //             cout << "<" << m.length << "," << m.distance << ">";
        //     cout << " ";
        // }
        // cout << '\n';

        // буфер выходной строки
        ostringstream oss;

        // добавляем в выходную строку заголовок gzip и добавляем блоки
        oss << "";  // gzip codes

        size_t size = 0;

        // каждый блок кодируем статическим кодом Хаффмана
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            char BFINAL = '0';
            if (i + 1 == blocks.size())
            {
                BFINAL = '1';
            }
            oss << BFINAL;
            ++size;

            oss << fixed_huffman_encode(blocks[i], size);
        }

        while (size++ % 8 != 0)
            oss << '0';

        // считаем контрольную сумму и добавляем конец
        oss << "";  // crc-32

        string result = pack(oss.str());

        return result;
    }
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        return 1;
    }

    string file_name = argv[1];

    ifstream inputFile(file_name);
    if (!inputFile.is_open())
    {
        cerr << "Не удалось открыть файл для чтения!" << endl;
        return 1;
    }

    string src((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    inputFile.close();

    Encoder encoder;
    string encoded = encoder.encode(src);

    ofstream outputFile(file_name + ".pk");
    if (!outputFile.is_open())
    {
        cerr << "Не удалось открыть файл для записи!" << endl;
        inputFile.close();
        return 1;
    }

    outputFile << encoded << endl;

    outputFile.close();

    cout << "Файл успешно упакован!" << endl;
}