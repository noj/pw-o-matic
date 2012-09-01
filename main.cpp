// This code is Public Domain
#include "sha256.hpp"

#include <iostream>
#include <string>
#include <cstdio>
#include <array>

#include <termios.h>
#include <unistd.h>

typedef std::array<unsigned char, 32> sha256_sum;

std::string format_sum(const sha256_sum& sum)
{
    static const char c[64] =
    {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    };

    std::string res;
    for(unsigned char s : sum)
        res += c[s & 63];

    return res;
}

sha256_sum sha256(const void* in, std::size_t size)
{
    sha256_state st;

    sha_init(st);
    sha_process(st, in, size);

    sha256_sum res;
    sha_done(st, res.data());

    return res;
}

sha256_sum sha256_rounds(const std::string& input, int extra_rounds)
{
    sha256_sum sum = sha256(
        input.c_str(), input.size());

    for(int i = 0; i != extra_rounds; ++i)
        sum = sha256(sum.data(), sum.size());

    return sum;
}

std::string readpass()
{
   termios t;

   tcgetattr(0, &t);
   t.c_lflag &= ~ECHO;
   tcsetattr(0, TCSANOW, &t);

   std::string pass;
   std::getline(std::cin, pass);

   t.c_lflag |= ECHO;
   tcsetattr(0, TCSANOW, &t);

   std::cout << '\n';

   return pass;
}

int main(int argc, char** argv)
{
   std::string site, pass;

   std::cout << "Master: ";
   pass = readpass();

   std::cout << "Site: ";
   std::getline(std::cin, site);

   std::cout << "Pass: " << format_sum(sha256_rounds(site + "|" + pass, 1000)) << "\n";
}
