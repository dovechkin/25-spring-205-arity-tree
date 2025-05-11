#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
const string BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+\\";

string encode(const string &s)
{
    uint32_t x = 0;
    size_t k = 0;
    string result;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char ch = s[i];
        if (i % 3 == 0 && k != 0)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                uint32_t idx = (x << (8 + j * 6)) >> 26;
                result.push_back(BASE64[idx]);
            }
            x = 0;
            k = 0;
        }

        ++k;
        x = x | (static_cast<uint32_t>(ch) << (8 * (2 - i % 3)));
    }

    for (size_t j = 0; j < k + 1; ++j)
    {
        uint32_t idx = (x << (8 + j * 6)) >> 26;
        result.push_back(BASE64[idx]);
    }
    for (size_t i = 0; i < 3 - k; ++i)
    {
        result.push_back('=');
    }

    return result;
}

string decode(const string &s)
{
    unordered_map<char, uint32_t> code;
    for (size_t i = 0; i < BASE64.size(); ++i)
    {
        code[BASE64[i]] = i;
    }

    uint32_t x = 0;
    size_t k = 0;
    string result;

    int cnt = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char ch = s[i];
        if (ch == '=')
        {
            ++cnt;
            continue;
        }

        if (i % 4 == 0 && k != 0)
        {
            for (size_t j = 0; j < 3; ++j)
            {
                char ch = static_cast<char>((x << (8 + j * 8)) >> 24);
                result.push_back(ch);
            }
            x = 0;
            k = 0;
        }

        ++k;
        x = x | (static_cast<uint32_t>(code[ch]) << (6 * (3 - i % 4)));
    }

    for (int i = 0; i < 3 - cnt; ++i)
    {
        char ch = static_cast<char>((x << (8 + i * 8)) >> 24);
        result.push_back(ch);
    }

    return result;
}

int main()
{
    std::string s;
    std::cin >> s;

    string encoded = encode(s);
    string decoded = decode(encoded);

    cout << encoded << " : " << decoded << '\n';
    cout << s.size() << " ? " << decoded.size() << '\n';

    assert(s == decoded);

    return 0;
}
