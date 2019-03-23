#pragma once
#include <iostream>
#include <string>
using std::string;

enum Lverbosity : size_t { L0 = 0, L1 = 1, L2 = 2, L3 = 3, L4 = 4, LDEBUG = 10};

class Logger
{
private:

public:
    Logger(){};
    ~Logger(){};

    void log(string info, Lverbosity level = L0);
};
