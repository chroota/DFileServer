#pragma once
#include <string>
#include "defines.hpp"

using std::string;

class Node
{
private:
    NODE_STATUS status;
    char file_hash[];
    int file_num;
public:
    Node(){};
    Node(const std::string & name,const std::string & ip){
        this->ip = ip;
        this->name = name;
        status = NODE_STATUS_CREATE;
    }
    ~Node(){};

    string ip;
    string name;

    string get_name()
    {
        return name;
    };
    string get_ip()
    {
        return ip;   
    }
    int get_status();

    bool change_status(NODE_STATUS status){
         this->status = status;
    }
};