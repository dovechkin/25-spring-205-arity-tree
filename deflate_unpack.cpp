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

struct Match {
  size_t distance;
  size_t length;
  char next_char;

  Match(size_t d, size_t l) : distance(d), length(l), next_char('\0') {}
  Match(size_t d, size_t l, char ch) : distance(d), length(l), next_char(ch) {}
};

class Decoder {
private:
  const string BTYPE_FIXED = "10"; // reverse order for packing
  const string END_OF_BLOCK = "0000000";

  unordered_map<string, tuple<u_int16_t, u_int8_t, u_int16_t>> len_table = {
      {"0000001", {257, 0, 3}},    {"0000010", {258, 0, 4}},
      {"0000011", {259, 0, 5}},    {"0000100", {260, 0, 6}},
      {"0000101", {261, 0, 7}},    {"0000110", {262, 0, 8}},
      {"0000111", {263, 0, 9}},    {"0001000", {264, 0, 10}},
      {"0001001", {265, 1, 11}},   {"0001010", {266, 1, 13}},
      {"0001011", {267, 1, 15}},   {"0001100", {268, 2, 17}},
      {"0001101", {269, 2, 21}},   {"0001110", {270, 2, 25}},
      {"0001111", {271, 2, 29}},   {"0010000", {272, 3, 33}},
      {"0010001", {273, 3, 41}},   {"0010010", {274, 3, 49}},
      {"0010011", {275, 3, 57}},   {"0010100", {276, 4, 65}},
      {"0010101", {277, 4, 81}},   {"0010110", {278, 4, 97}},
      {"0010111", {279, 4, 113}},  {"11000000", {280, 5, 129}},
      {"11000001", {281, 5, 161}}, {"11000010", {282, 5, 193}},
      {"11000011", {283, 5, 225}}, {"11000100", {284, 5, 257}},
      {"11000101", {285, 0, 258}},
  };

  unordered_map<string, tuple<u_int8_t, u_int8_t, u_int16_t>> dist_table = {
      {"00000", {0, 0, 1}},       {"00001", {1, 0, 2}},
      {"00010", {2, 0, 3}},       {"00011", {3, 0, 4}},
      {"00100", {4, 1, 5}},       {"00101", {5, 1, 7}},
      {"00110", {6, 2, 9}},       {"00111", {7, 2, 13}},
      {"01000", {8, 3, 17}},      {"01001", {9, 3, 25}},
      {"01010", {10, 4, 33}},     {"01011", {11, 4, 49}},
      {"01100", {12, 5, 65}},     {"01101", {13, 5, 97}},
      {"01110", {14, 6, 129}},    {"01111", {15, 6, 193}},
      {"10000", {16, 7, 257}},    {"10001", {17, 7, 385}},
      {"10010", {18, 8, 513}},    {"10011", {19, 8, 769}},
      {"10100", {20, 9, 1025}},   {"10101", {21, 9, 1537}},
      {"10110", {22, 10, 2049}},  {"10111", {23, 10, 3073}},
      {"11000", {24, 11, 4097}},  {"11001", {25, 11, 6145}},
      {"11010", {26, 12, 8193}},  {"11011", {27, 12, 12289}},
      {"11100", {28, 13, 16385}}, {"11101", {29, 13, 24577}},
  };

  unordered_map<string, char> lit_table;

  void fill_lit_codes() {
    lit_table.clear();

    for (u_int8_t i = 0; i < 144; ++i) {
      string code = bitset<8>(0x30 + i).to_string();
      lit_table[code] = static_cast<char>(i);
    }

    for (u_int16_t i = 144; i < 256; ++i) {
      string code = bitset<9>(0x190 + (i - 144)).to_string();
      lit_table[code] = static_cast<char>(i);
    }
  }

  string unpack(const string &src) {
    string unpacked;

    for (auto ch : src)
      for (u_int8_t i = 0; i < 8; ++i)
        if (ch & (1 << i))
          unpacked.push_back('1');
        else
          unpacked.push_back('0');

    return unpacked;
  }

  size_t get_distance(const string &src, size_t &pos) {
    auto [_, extra_bits, dist] = dist_table[src.substr(pos, 5)];
    pos += 5;

    for (size_t i = 0; i < extra_bits; ++i)
      if (src[pos + i] == '1')
        dist += (1 << i);

    pos += extra_bits;

    return dist;
  }

  Match get_match(const string &src, size_t &pos, u_int8_t huff_len) {
    auto [_, extra_bits, len] = len_table[src.substr(pos, huff_len)];
    pos += 7;

    for (size_t i = 0; i < extra_bits; ++i)
      if (src[pos + i] == '1')
        len += (1 << i);

    pos += extra_bits;

    size_t dist = get_distance(src, pos);

    return Match(dist, len);
  }

  vector<Match> get_matches(const string &src) {
    vector<Match> matches;

    size_t pos = 0;

    bool is_final_block;
    string btype;
    string huff_code;
    do {
      is_final_block = (src[pos] == '1');
      pos += 1;
      btype = src.substr(pos, 2);
      pos += 2;

      while (src.substr(pos, 7) != END_OF_BLOCK) {
        if (len_table.find(src.substr(pos, 7)) != len_table.end()) {
          Match match = get_match(src, pos, 7);
          matches.push_back(match);
        } else if (lit_table.find(src.substr(pos, 8)) != lit_table.end()) {
          matches.emplace_back(0, 1, lit_table[src.substr(pos, 8)]);
          pos += 8;
        } else if (len_table.find(src.substr(pos, 8)) != len_table.end()) {
          Match match = get_match(src, pos, 8);
          matches.push_back(match);
        } else if (lit_table.find(src.substr(pos, 9)) != lit_table.end()) {
          matches.emplace_back(0, 1, lit_table[src.substr(pos, 9)]);
          pos += 9;
        }
      }

      pos += 7;

    } while (!is_final_block);

    return matches;
  }

public:
  Decoder() { fill_lit_codes(); }

  string decode(const string &src) {
    string bits = unpack(src);

    // read gzip header
    // string header = src.substr(pos, 8);
    // string footer = src. substr(src.size()-8, 8);
    // string deflate_str = src.substr(8, src.size() - 16);

    string deflate_str = bits;

    vector<Match> matches = get_matches(deflate_str);

    string result;
    for (auto [dist, len, ch] : matches) {
      if (dist == 0)
        result.push_back(ch);
      else {
        size_t start = result.size() - dist;
        for (size_t i = 0; i < len; ++i)
          result.push_back(result[start + i % dist]);
      }
    }

    return result;
  }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }

  string file_name = argv[1];

  ifstream inputFile(file_name);
  if (!inputFile.is_open()) {
    cerr << "Не удалось открыть файл для чтения!" << endl;
    return 1;
  }

  string src((istreambuf_iterator<char>(inputFile)),
             istreambuf_iterator<char>());
  inputFile.close();

  Decoder decoder;
  string decoded = decoder.decode(src);

  string unpack_file_name;
  if (file_name.substr(file_name.size() - 3, 3) == ".pk")
    unpack_file_name = file_name.substr(0, file_name.size() - 3);
  else
    unpack_file_name = file_name + ".unpk";

  ofstream outputFile(unpack_file_name);
  if (!outputFile.is_open()) {
    cerr << "Не удалось открыть файл для записи!" << endl;
    inputFile.close();
    return 1;
  }

  outputFile << decoded << endl;

  outputFile.close();

  cout << "Файл успешно распакован!" << endl;
}
