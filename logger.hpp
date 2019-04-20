#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include "defines.hpp"
#include <stdarg.h>
#include <sstream>
#include <fstream>
using namespace std;


enum verbosity : size_t { L0 = 0, L1 = 1, L2 = 2, L3 = 3, L4 = 4, LFATAL = 4, LDEBUG = 5};

class Logger
{
private:
    ofstream *pOutput = nullptr;
public:
    Logger(){};
    ~Logger(){
        if(pOutput) {
            delete pOutput;
            pOutput = nullptr;
        }
    };
    void log(string info, verbosity level = L0);
    void log(verbosity verbose, const char* fmt, ...);
    void debug(const string & info);
    void fatal(const string & info);
    void errnoInfo(int errno);
    void debugAction(const string & action);
    string getTimeFmtString();
    bool saveLog(const string & info);
};