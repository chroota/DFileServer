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
#include <unistd.h>
#include <sys/inotify.h>
#include "logger.hpp"
#include "string.h"
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
// #include <openssl/md5.h>
#include "common.hpp"
#include "md5pkg.hpp"

using  namespace std;

enum FileType{
    VFT_FILE = 1,
    // VFT_BIN = 1,
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
        this->fa_idx = fa_idx;
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
    FileType _type = VFT_FILE;
    string _name = "";
    // string _disk_path = "";
    string _dirPath = "";
    off_t _size = 0;       // for dir: number of chidren; for file: size of file
    string _hash = "";       // todo
    struct timespec _mtime;
    bool status = false;        // 1:active 0:dead
public:
    VFile(){};
    ~VFile(){};
    VFile(const string & name, FileType type, const string & dirPath, off_t size, 
    struct timespec mtime, int fa_idx, int idx, int prev_bro_idx, int next_bro_idx, int first_son_idx, int last_son_idx)
        :_name(name), _type(type), _dirPath(dirPath), _size(size),
        _mtime(mtime)
        {
            //todo hash compute
            _hash = Hash(type, dirPath, size, mtime, fa_idx, idx, next_bro_idx, first_son_idx, last_son_idx);
        };    
    
    
    // VFile factory(const string &name, FileType, const string &path){
    //     return VFile(
            
    //     );
    // }

    string getName()
    {
        return _name;
    }

    string getVfrKey(){
        if(_dirPath == "/" && _name == "root") return "/";
        return _dirPath + _name;
    }

    string getDirPath(){
        return _dirPath;
    }

    string getFullPath(){
        return _dirPath + "/" + _name;
    }

    FileType getType(){
        return _type;
    }

    long getTvSec(){
        return _mtime.tv_sec;
    }

    long getTvNsec(){
        return _mtime.tv_nsec;
    }

    bool active(){
        status = true;
        return true;
    }

    bool isActive(){
        return status;
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

    off_t getSize(){
        return _size;
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


struct FileOp
{
    Msg::FileOpType type;
    string path;
    string dstPath;
};


class Vvfs
{
private:
    //file nums
    int num;
    vector<VFile> vFiles;
    // map<string, int> fileNameIdxMap;
    string vFRLogFile = "";
    string vFOpLogFile = "";
    map<string, VFRelation> vRelations;
    string hash = "";
    Logger logger;
    MD5pkg hashpkg;
    ofstream *pVFOpLogFileFstream = nullptr;
    bool writeOpLog(Msg::FileOpType op, const string & pathString);
    atomic<bool> isVFRelationsStored;
public:
    string vfsPath;
    int getNum(){
        return num;
    }

    int allocIdx();
    bool buildVFS();
    bool refreshVFS();
    bool buildVFSByScanDir();
    bool buildVFSReserve();
    bool updateVF(const string &name);

    /*
     * store VFRelations automatically
    */
    bool storeVFRelations();
    void storeVFRelationsBackend();

    bool openVFOpLogFile();
    // compute state hash from op log file
    bool updateHashFromVFOpLogFile();
    bool watchVFS();
    // config init
    bool initConfig
    (
        const string & vfsPath,
        const string & vFRLogFile = "./test_dir/remote/vfr.txt", 
        const string & vFOpLogFile="./test_dir/remote/oplog.txt"
    );


    /*
     * access vfile & vfrelation mutual
    */
    VFile &getVFByVfr(VFRelation & vfr);
    VFRelation &getVFRByVF(VFile &vf);
    VFRelation &getVFRByIdx(int idx);

    
    /*
     * vf control
    */
    //create dir
    bool mkVDir(const string &name, string &err);
    // create new vf
    bool newVF(const string &name, off_t totalSize, string &err, FileType type = VFT_FILE); //create file
    // remove vf
    bool rmVF(const string &name, string &err);
    // move vf
    bool mvVF(const string &srcPath, const string &dstPath, string &err);
    // list vf info
    bool lsVF(const string & path, Msg::Message &msg);
    // active vf when new file is upload complete
    bool activeVF(const string &name, string &err);

    /*
     * create root vf
    */
    bool createRootVF();



    /*
     * update hash when file operation event fired
     * Vvfs state hash: Hash(old_hash, op+path+now_time)
    */
    bool updateHashByNewFileOp(const string &path);
    bool updateHashByNewFileOp(const string &path, long long opTime);
    bool updateHashByRMFileOp(const string &path);
    bool updateHashByRMFileOp(const string &path, long long opTime);
    bool updateHashByMvFileOp(const string &srcPath, const string &dstPath);
    bool updateHashByMvFileOp(const string &srcPath, const string &dstPath, long long opTime);

    /*
     * op log writter
    */
    bool writeNewFileOpLog(const string &path);
    bool writeRMFileOpLog(const string &path);
    bool writeMvFileOpLog(const string &srcPath, const string &dstPath);
    bool writeMvFileOpLogTest();

    // demaon for Vvfs operations of backend
    bool deamon();

    Vvfs(){
        isVFRelationsStored = true;
    };
    ~Vvfs()
    {
        logger.debug("Vvfs deconstructor");
        if(pVFOpLogFileFstream)
        {
            delete pVFOpLogFileFstream;
            pVFOpLogFileFstream = nullptr;
        }
        logger.debug("after Vvfs debug");
    };
};