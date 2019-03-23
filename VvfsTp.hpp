#pragma once
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "msg.pb.h"


#define NEW_FILE "new"
#define RM_FILE "rm"
#define MV_FILE "mv"
#define GET_FILE "get"
#define LS_FILE "ls"

using namespace std;

class VvfsTp
{
private:

public:
    VvfsTp(){};
    ~VvfsTp(){};

    void help(){
        cout<<"VvfsTP instruction argv1 argv2 ..."<<endl
            <<"++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl
            <<"VvfsTP new local_filepath remote_filepath"<<endl
            <<"VvfsTP rm remote_filepath"<<endl
            <<"VvfsTP mv remote_filepath_src remote_filepath_dst"<<endl
            <<"VvfsTP ls remote_filepath"<<endl
            <<"VvfsTP get remote_filepath loacl_filepath"<<endl;

        exit(0);
    }

    void run(int argc, char *argv[]);
};
