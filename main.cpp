// This code is Public Domain
#include "pwgen.hpp"

#include <cstdio>
#include <iostream>
#include <termios.h>
#include <unistd.h>

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

   std::cout << gen_password("sig", pass).substr(4, 4) << "...\n";

   return pass;
}

int main(int argc, char** argv)
{
   std::string site, pass;

   std::cout << "Master: ";
   pass = readpass();

   std::cout << "Site: ";
   std::getline(std::cin, site);

   std::cout << "Pass: " << gen_password(site, pass) << "\n";
}
