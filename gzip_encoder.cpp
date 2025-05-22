#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Options
{
    bool store_filename;
    string file_name;
};

struct Match
{
    size_t distance;
    size_t length;
    char next_char;

    Match(size_t d, size_t l) : distance(d), length(l), next_char('\0') {}
    Match(size_t d, size_t l, char ch) : distance(d), length(l), next_char(ch) {}
};

constexpr size_t BLOCK_SIZE = 65536;
constexpr size_t WINDOW_SIZE = 32768;
constexpr uint16_t MIN_MATCH_LEN = 3;
constexpr uint16_t MAX_MATCH_LEN = 258;
constexpr uint8_t BYTE_SIZE = 8;
constexpr size_t BUF_SIZE = WINDOW_SIZE + MAX_MATCH_LEN;
const string BTYPE_FIXED = "10";
const string END_OF_BLOCK = "0000000";

constexpr uint16_t LITERAL_CODE_OFFSET_1 = 0x30;
constexpr uint16_t LITERAL_CODE_OFFSET_2 = 0x190;
constexpr uint16_t LITERAL_THRESHOLD = 144;

map<uint16_t, tuple<uint16_t, uint8_t, string>> len_table = {
    {3, {257, 0, "0000001"}},    {4, {258, 0, "0000010"}},    {5, {259, 0, "0000011"}},    {6, {260, 0, "0000100"}},
    {7, {261, 0, "0000101"}},    {8, {262, 0, "0000110"}},    {9, {263, 0, "0000111"}},    {10, {264, 0, "0001000"}},
    {11, {265, 1, "0001001"}},   {13, {266, 1, "0001010"}},   {15, {267, 1, "0001011"}},   {17, {268, 2, "0001100"}},
    {21, {269, 2, "0001101"}},   {25, {270, 2, "0001110"}},   {29, {271, 2, "0001111"}},   {33, {272, 3, "0010000"}},
    {41, {273, 3, "0010001"}},   {49, {274, 3, "0010010"}},   {57, {275, 3, "0010011"}},   {65, {276, 4, "0010100"}},
    {81, {277, 4, "0010101"}},   {97, {278, 4, "0010110"}},   {113, {279, 4, "0010111"}},  {129, {280, 5, "11000000"}},
    {161, {281, 5, "11000001"}}, {193, {282, 5, "11000010"}}, {225, {283, 5, "11000011"}}, {257, {284, 5, "11000100"}},
    {258, {285, 0, "11000101"}},
};

map<uint16_t, tuple<uint8_t, uint8_t, string>> dist_table = {
    {1, {0, 0, "00000"}},       {2, {1, 0, "00001"}},       {3, {2, 0, "00010"}},      {4, {3, 0, "00011"}},
    {5, {4, 1, "00100"}},       {7, {5, 1, "00101"}},       {9, {6, 2, "00110"}},      {13, {7, 2, "00111"}},
    {17, {8, 3, "01000"}},      {25, {9, 3, "01001"}},      {33, {10, 4, "01010"}},    {49, {11, 4, "01011"}},
    {65, {12, 5, "01100"}},     {97, {13, 5, "01101"}},     {129, {14, 6, "01110"}},   {193, {15, 6, "01111"}},
    {257, {16, 7, "10000"}},    {385, {17, 7, "10001"}},    {513, {18, 8, "10010"}},   {769, {19, 8, "10011"}},
    {1025, {20, 9, "10100"}},   {1537, {21, 9, "10101"}},   {2049, {22, 10, "10110"}}, {3073, {23, 10, "10111"}},
    {4097, {24, 11, "11000"}},  {6145, {25, 11, "11001"}},  {8193, {26, 12, "11010"}}, {12289, {27, 12, "11011"}},
    {16385, {28, 13, "11100"}}, {24577, {29, 13, "11101"}},
};

string get_literal_fixed_code(char ch)
{
    size_t code;
    string result;
    if (static_cast<uint8_t>(ch) < LITERAL_THRESHOLD)
    {
        code = LITERAL_CODE_OFFSET_1 + ch;
        result = bitset<8>(code).to_string();
    }
    else
    {
        code = LITERAL_CODE_OFFSET_2 + (ch - LITERAL_THRESHOLD);
        result = bitset<9>(code).to_string();
    }

    return result;
}

string get_distance_fixed_code(uint16_t dist)
{
    auto it = dist_table.upper_bound(dist);
    --it;

    uint16_t base_dist = it->first;
    auto [code, bits, huff] = it->second;

    string code_str = huff;
    uint16_t dist_diff = dist - base_dist;
    for (uint8_t i = 0; i < bits; ++i)
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

    uint16_t base_len = it->first;
    auto [code, bits, huff] = it->second;

    string code_str = huff;
    uint8_t len_diff = len - base_len;
    for (uint8_t i = 0; i < bits; ++i)
    {
        char bit = ((len_diff >> i) & 1) ? '1' : '0';
        code_str.push_back(bit);
    }

    return code_str;
}

uint32_t get_current_unix_time()
{
    auto now = chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto seconds = chrono::duration_cast<std::chrono::seconds>(epoch);
    return static_cast<uint32_t>(seconds.count());
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

void append_binary_data(ostream &out, uint8_t data, uint8_t &byte, uint8_t &bit_shift)
{
    for (uint8_t j = BYTE_SIZE; j > 0; ++j)
    {
        byte |= (((data >> (j - 1)) & 1) << bit_shift);

        if (++bit_shift == BYTE_SIZE)
        {
            out.put(byte);
            byte = 0;
            bit_shift = 0;
        }
    }
}

void append_string_data(ostream &out, const string &src, uint8_t &byte, uint8_t &bit_shift)
{
    for (uint8_t j = 0; j < src.size(); ++j)
    {
        uint8_t bit = (src[j] == '1') ? 1 : 0;
        byte |= (bit << bit_shift);

        if (++bit_shift == BYTE_SIZE)
        {
            out.put(byte);
            byte = 0;
            bit_shift = 0;
        }
    }
}

void write_compressed_data(istream &in, ostream &out, uint32_t &crc, uint32_t &isize)
{
    size_t pos = 0;
    size_t front = 0;

    char buffer[BUF_SIZE];
    uint8_t byte = 0;
    uint8_t bit_shift = 0;

    uint32_t crc_table[256];
    generate_crc32_table(crc_table);

    in.read(buffer, MAX_MATCH_LEN);
    front = in.gcount();

    crc = 0xFFFFFFFF;
    for (size_t i = 0; i < front; ++i)
        crc = (crc >> 8) ^ crc_table[(crc ^ buffer[i]) & 0xFF];

    vector<Match> block;
    size_t curr_block_len = 0;

    char symbol;
    while (pos < front)
    {
        size_t window_start = (pos > WINDOW_SIZE) ? (pos - WINDOW_SIZE) : 0;

        size_t best_match_len = 1;
        size_t best_match_dist = 0;
        for (size_t start = window_start; start < pos; ++start)
        {
            size_t match_len = 0;
            while (match_len <= MAX_MATCH_LEN && (pos + match_len < front) &&
                   buffer[(start + match_len) % BUF_SIZE] == buffer[(pos + match_len) % BUF_SIZE])
                ++match_len;

            if (match_len >= MIN_MATCH_LEN && match_len > best_match_len)
            {
                best_match_len = match_len;
                best_match_dist = pos - start;
            }
        }

        if (curr_block_len + best_match_dist > BLOCK_SIZE)
        {
            append_string_data(out, "010", byte, bit_shift);

            for (const auto &[dist, len, ch] : block)
            {
                if (dist == 0)
                {
                    string code = get_literal_fixed_code(ch);
                    append_string_data(out, code, byte, bit_shift);
                }
                else
                {
                    string len_code = get_length_fixed_code(len);
                    string dist_code = get_distance_fixed_code(dist);

                    append_string_data(out, len_code, byte, bit_shift);
                    append_string_data(out, dist_code, byte, bit_shift);
                }
            }

            append_string_data(out, END_OF_BLOCK, byte, bit_shift);

            block.clear();
            curr_block_len = 0;
        }

        if (best_match_dist == 0)
        {
            // печатаем сам символ
            block.emplace_back(0, 1, buffer[pos % BUF_SIZE]);

            ++pos;
            ++curr_block_len;

            if (in >> symbol)
            {
                buffer[front++ % BUF_SIZE] = symbol;
                crc = (crc >> 8) ^ crc_table[(crc ^ symbol) & 0xFF];
            }
        }
        else
        {
            // кодируем длину и расстояние
            block.emplace_back(best_match_dist, best_match_len);

            pos += best_match_len;
            curr_block_len += best_match_len;

            for (size_t i = 0; i < best_match_len; ++i)
                if (in >> symbol)
                {
                    buffer[front++ % BUF_SIZE] = symbol;
                    crc = (crc >> 8) ^ crc_table[(crc ^ symbol) & 0xFF];
                }
        }
    }

    // Упаковываем последний блок
    append_string_data(out, "110", byte, bit_shift);

    for (const auto &[dist, len, ch] : block)
    {
        if (dist == 0)
        {
            string code = get_literal_fixed_code(ch);
            append_string_data(out, code, byte, bit_shift);
        }
        else
        {
            string len_code = get_length_fixed_code(len);
            string dist_code = get_distance_fixed_code(dist);

            append_string_data(out, len_code, byte, bit_shift);
            append_string_data(out, dist_code, byte, bit_shift);
        }
    }

    append_string_data(out, END_OF_BLOCK, byte, bit_shift);

    if (bit_shift > 0)
        out.put(byte);

    crc ^= 0xFFFFFFFF;
    isize = pos;
}

void encode(istream &in, ostream &out, Options options)
{
    // 1. Заголовок GZip
    out.put(0x1F); // ID1
    out.put(0x8B); // ID2
    out.put(0x08); // CM = DEFLATE

    uint8_t flags;
    if (options.store_filename)
        flags |= (1 << 3); // сохранить имя файла

    out.put(flags); // FLG

    // MTIME (время модификации, 0 = неизвестно)
    uint32_t mtime = get_current_unix_time();
    for (int i = 0; i < 4; i++)
        out.put((mtime >> 8 * i) & 0xFF);

    out.put(0x00); // XFL (максимальное сжатие)
    out.put(0xFF); // OS

    // сохраняем имя файла
    if (options.store_filename)
    {
        for (auto ch : options.file_name)
            out.put(ch);
        out.put(0x00);
    }

    uint32_t crc;
    uint32_t isize;

    // 2. Добавляем сжатые данные и считаем контрольные суммы
    write_compressed_data(in, out, crc, isize);

    // 3. CRC-32 исходных данных (little-endian)
    for (int i = 0; i < 4; i++)
        out.put((crc >> 8 * i) & 0xFF);

    // 4. ISIZE (размер исходных данных, little-endian)
    for (int i = 0; i < 4; i++)
        out.put((isize >> 8 * i) & 0xFF);
}

string cut_name(string filename)
{
    size_t pos = filename.find_last_of("/");
    if (pos != string::npos)
        return filename.substr(pos + 1);

    return filename;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Передано не верное количество аргументов!" << endl;
        return 1;
    }

    string filename = argv[1];

    ifstream input_file(filename);
    if (!input_file.is_open())
    {
        cerr << "Не удалось открыть файл для чтения!" << endl;
        return 1;
    }

    ofstream output_file(filename + ".gz");
    if (!output_file.is_open())
    {
        cerr << "Не удалось открыть файл для записи!" << endl;
        return 1;
    }

    Options options = {true, cut_name(filename)};

    encode(input_file, output_file, options);

    input_file.close();
    output_file.close();

    cout << "Файл успешно обработан!" << endl;
}