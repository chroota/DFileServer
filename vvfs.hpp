#pragma once
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <queue>
#include <map>
#include <sys/inotify.h>
#include "logger.hpp"
#include "string.h"
#include <sstream>
// #include <openssl/md5.h>
#include "common.hpp"
#include "md5pkg.hpp"

using  namespace std;

enum FileType{
    VFT_TXT = 0,
    VFT_BIN = 1,
    VFT_DIR = 2
};

// work mode
enum VvFsMode{
    VFM_CMD,
    VFM_WATCH,
    VFM_BOTH    
};

struct VFRelation{
    int idx;
    int fa_idx;
    int next_bro_idx = -1;  //next brother idx
    int prev_bro_idx = -1;  //previous brother idx
    int first_son_idx = -1; //first son idx
    int last_son_idx = -1;  //id of last son idx     

    VFRelation(){}
    VFRelation(int idx, int fa_idx, int prev_bro_idx, int next_bro_idx, int first_son_idx, int last_son_idx){
        this->idx = idx;
        this->prev_bro_idx = prev_bro_idx;
        this->first_son_idx = first_son_idx;
        this->next_bro_idx = next_bro_idx;
        this->last_son_idx = last_son_idx;
    }

    ~VFRelation(){}
};

class VFile
{
private:
    FileType _type = VFT_TXT;
    string _name = "";
    string _disk_path = "";
    // string _disk_name = "";   // todo diskname
    off_t _size = 0;       // for dir: number of chidren; for file: size of file
    string _hash = "";       // todo
    struct timespec _mtime;
    bool status = 0;        // 1:active 0:dead
public:
    // int idx;
    // int fa_idx = -1;        //father idx
    // int next_bro_idx = -1;  //net_brother_idx
    // int first_son_idx = -1; //first_son_idx

    VFile(){};
    ~VFile(){};
    VFile(const string & name, FileType type, const string & path, off_t size, 
    struct timespec mtime, int fa_idx, int idx, int prev_bro_idx, int next_bro_idx, int first_son_idx, int last_son_idx)
        :_name(name), _type(type), _disk_path(path), _size(size),
        _mtime(mtime)
        {
            //todo hash compute
            _hash = Hash(type, path, size, mtime, fa_idx, idx, next_bro_idx, first_son_idx, last_son_idx);
        };    
    
    
    // VFile factory(const string &name, FileType, const string &path){
    //     return VFile(
            
    //     );
    // }

    string getName(){
        return _name;
    }

    FileType getType(){
        return _type;
    }

    bool active(){
        status = 1;
        return true;
    }

    bool setType(FileType type){
        // todo exception
        _type = type;
        return true;
    }

    bool setSize(off_t size){
        //todo exception atomic
        _size = size;
        return true;
    }

    // size increment 1
    off_t incSize(){
        //todo atomic
        _size += 1;
        return _size;
    }

    // size decrement 1
    off_t decSize(){
        //todo atomic
        _size -= 1;
        return _size;
    }

    //file destory 
    bool destory(){
        status = 0;
        return true;
    }

    const string & getHash(){
        return _hash;
    }


    string Hash(FileType type, const string & path, off_t size, struct timespec mtime, int _fa_idx,
            int _idx, int next_bro_idx, int first_son_idx, int last_son_idx); //get hash todo
};

class Vvfs
{
private:
    int num;   //file nums
    vector<VFile> vFiles;
    // map<string, int> fileNameIdxMap;
    string vFRConfigFile = "";
    map<string, VFRelation> vRelations;
    string hash = "";
    Logger logger;
    MD5pkg mpkg;
public:
    Vvfs(){};
    ~Vvfs(){};
    string vfsPath;
    Vvfs(const string & path){
        initConfig();
        this->vfsPath = path;
    };

    int getNum(){
        return num;
    }

    int allocIdx(){
        return vFiles.size();
    }
    bool buildVFS();
    bool refreshVFS();
    bool buildVFSByScanDir();
    bool buildVFsReserve();
    // bool buildVFs();
    bool updateVF(const string &name);
    bool initHash();
    bool watchVFS();
    bool buildVFR();   //build vfs relation
    bool initConfig();
    bool mkVDir(const string &name, string &err);   //create dir
    bool newVF(const string &name, string &err, FileType type=VFT_TXT); //create file
    bool rmVF(const string &name, string &err);
    bool mvVF(const string &srcPath, const string &dstPath, string &err);
    bool lsVF(const string &name, string & fileList, string &err);
    bool activeVF(const string &name, string &err);
    bool createRootVF();
};