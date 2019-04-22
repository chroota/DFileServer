#pragma once
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include "msg.pb.h"
#include "defines.hpp"
#include "logger.hpp"
#include "common.hpp"


#define NEW_FILE "new"
#define RM_FILE "rm"
#define MV_FILE "mv"
#define GET_FILE "get"
#define LS_FILE "ls"
#define CP_FILE "cp"
#define MK_DIR "mkdir"

using namespace std;

class VvfsTp
{
private:
    Logger logger;
    string host;
    int port;
    
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
    bool newVF(const string & localPath, const string & rmotePath, Msg::FileType type=Msg::FT_FILE);
    bool rmVF(const string & remotePath);
    bool lsVF(const string & remotePath);
    bool getVF(const string & remotePath, const string & localPath);
    bool mvVF(const string & remoteSrcPath, const string & remoteDstPath);
    bool mkDir(const string & remotePath);
    bool cpVF(const string & remoteSrcPath, const string & remoteDstPath);
    bool init();
};
