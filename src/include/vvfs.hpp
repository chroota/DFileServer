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
#include <list>
// #include <openssl/md5.h>
#include "common.hpp"
#include <msg.hpp>
#include <md5pkg.hpp>


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
    VFRelation(int idx, int fa_idx, int prev_bro_idx, int next_bro_idx, int first_son_idx, int last_son_idx)
    {
        this->idx = idx;
        this->fa_idx = fa_idx;
        this->prev_bro_idx = prev_bro_idx;
        this->first_son_idx = first_son_idx;
        this->next_bro_idx = next_bro_idx;
        this->last_son_idx = last_son_idx;
    }

    ~VFRelation(){}
};


/*
 * representation of file operation
 * 1. create new file
 * 2. rm file
 * 3. mv file
 * 4. copy file
*/
struct FileOperation
{
    Msg::FileOpType type;
    string srcPath;
    string dstPath;
    string stateHash;
    long long opTime;

    FileOperation(){}

    FileOperation(Msg::FileOpType type, const string & srcPath, const string &dstPath, const string & stateHash, long long opTime)
    {
        this->type = type;
        this->srcPath = srcPath;
        this->dstPath = dstPath;
        this->stateHash = stateHash;
        this->opTime = opTime;
    }
    ~FileOperation(){}
};

class VFile
{
private:
    FileType _type = VFT_FILE;
    string _name = "";
    string _disk_path = "";
    string _dirPath = "";
    off_t _size = 0;            // for dir: number of chidren; for file: size of file
    string _hash = "";          // todo
    // struct timespec _mtime;
    time_t _tv_sec;              // tv_sec
    bool status = false;        // 1:active 0:dead
    string _ext = "";
public:
    VFile(){};
    ~VFile(){};
    VFile(const string & name, FileType type, const string & dirPath, off_t size, time_t tv_sec)
        :_name(name), _type(type), _dirPath(dirPath), _size(size), _tv_sec(tv_sec)
    {
        string fullPath = dirPath + name;
        if(type == VFT_FILE)
        {
            ostringstream oss;
            oss << dirPath << name << size << tv_sec;
            _disk_path = getBufMD5(oss.str().c_str(), oss.str().size());
            int extIdx = name.find_last_of(".");
            if(extIdx != -1) 
                _ext = name.substr(extIdx+1);
        }
    };

    // struct timespec getMtime()
    // {
    //     return _mtime;
    // }

    bool setDirPath(const string &path)
    {
        _dirPath = path;
        return true;
    }

    string getName()
    {
        return _name;
    }

    string getVfrKey()
    {
        if(_dirPath == "/" && _name == "root") return "/";
        if(_dirPath == "/") return "/" + _name;
        return _dirPath + "/" + _name;
    }

    string getDiskPath()
    {
        return _disk_path;
    }

    string getDirPath()
    {
        return _dirPath;
    }

    string getFullPath()
    {
        if(_dirPath == "/") return "/" + _name;
        return _dirPath + "/" + _name;
    }

    FileType getType()
    {
        return _type;
    }

    long getTvSec()
    {
        return _tv_sec;
    }

    long getTvNsec()
    {
        return 0;
        // return _mtime.tv_nsec;
    }

    bool active()
    {
        status = true;
        return true;
    }

    bool isActive()
    {
        return status;
    }

    bool setType(FileType type)
    {
        // todo exception
        _type = type;
        return true;
    }

    bool setSize(off_t size)
    {
        //todo exception atomic
        _size = size;
        return true;
    }

    off_t getSize()
    {
        return _size;
    }

    // size increment 1
    off_t incSize()
    {
        //todo atomic
        _size += 1;
        return _size;
    }

    // size decrement 1
    off_t decSize()
    {
        //todo atomic
        _size -= 1;
        return _size;
    }

    //file destory 
    bool destory()
    {
       status = 0;
        return true;
    }

    const string & getHash()
    {
        return _hash;
    }

    bool updateHash(FileType type, const string & path, off_t size, struct timespec mtime); //get hash todo
    bool updateHash(VFRelation &vfr);
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
    atomic<bool> isVFRelationsStored;
    list<FileOperation> operationList;

public:
    string vfsPath;
    int getNum()
    {
        return num;
    }

    string getHash()
    {
        return hash;
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
    VFile &getVFByVfr(VFRelation &vfr);
    VFRelation &getVfrByVf(VFile &vf);
    VFRelation &getVfrByIdx(int idx);
    int getIdxByVf(VFile &vf);
    VFRelation &getNextVfrByVfr(VFRelation &vfr);
    int getNextIdxByIdx(int idx);
    VFRelation &getPrevVfrByVfr(VFRelation &vfr);
    VFRelation &getLastSonVfrByVfr(VFRelation &vfr);
    VFRelation &getFirstSonVfrByVfr(VFRelation &vfr);
    
    /*
     * vf control
    */
    // create root vf
    bool createRootVF(time_t opTime = -1);
    //create dir
    bool mkVDir(const string &path, string &err, time_t opTime = -1);
    // create new vf
    bool newVF(const string &path, off_t totalSize, string &err, string &diskPath, FileType type = VFT_FILE, time_t opTime = -1); //create file
    // remove vf
    bool rmVF(const string &path, string &err, time_t opTime = -1);
    bool rmSubfolder(VFile &vf, string &err);
    // move vf
    bool mvVF(const string &srcPath, const string &dstPath, string &err, time_t opTime = -1);
    // cp vf
    bool cpVF(const string &srcPath, const string &dstPath, string &err, time_t opTime = -1);
    // list vf info
    bool lsVF(const string & path, Msg::Message &msg);
    // active vf when new file is upload complete
    bool activeVF(const string &path, string &err);

   
    bool syncNewVfOp(const string &path, const string &hash, time_t opTime);
    bool syncNewVDirOp(const string &path, const string &hash, time_t opTime);
    bool syncRmVfOp(const string &path, const string &hash, time_t opTime);
    bool syncMvVfOp(const string &srcPath, const string &dstPath, const string &hash, time_t opTime);
    bool syncCpVfOp(const string &srcPath, const string &dstPath, const string &hash, time_t opTime);

    // copy disk path
    bool copyDiskFile(const string& srcPath, const string &dstPath);

    //
    int pushVf2vFiles(const string & name, FileType type, const string & dirPath, off_t size, time_t opTime = -1);
    bool pushbackVf(VFRelation & dirVfr, VFile &vf);
    bool removeVfRelation(VFile &vf, bool isVfDestory=true);
    bool changeVfDirRelation(VFile &vf, VFRelation &newDirVfr, const string &newDirPath, bool isVfDestory = true);

    string getVFPhasicalPath(VFile & vf);
    string getVFPhasicalPath(string &vfDiskPath);
    bool getDirPathAndName(const string &path, string &dirPath, string &name, string &err);

    // operation event fired
    bool newFileEndingWork(const string &path, time_t opTime = -1);
    bool newDirEndingWork(const string &path, time_t opTime = -1);
    bool rmFileEndingWork(const string &path, time_t opTime = -1);
    bool mvFileEndingWork(const string &srcPath, const string &dstPath, time_t opTime = -1);
    bool cpFileEndingWork(const string &srcPath, const string &dstPath, time_t opTime = -1);

    // append new operation to file operation list
    bool appenNewFileOperation(FileOperation && op);

    /*
     * update hash when file operation event fired
     * Vvfs state hash: Hash(old_hash, op+path + now_time)
    */
    // string updateHashByNewFileOp(const string &path);
    string updateHashByNewFileOp(const string &path, time_t opTime);
    // string updateHashByRmFileOp(const string &path);
    string updateHashByRmFileOp(const string &path, time_t opTime);
    // string updateHashByMvFileOp(const string &srcPath, const string &dstPath);
    string updateHashByMvFileOp(const string &srcPath, const string &dstPath, time_t opTime);
    // string updateHashByCpFileOp(const string &srcPath, const string &dstPath);
    string updateHashByCpFileOp(const string &srcPath, const string &dstPath, time_t opTime);
    bool updateHash(const string &newestHash);

    /*
     * op log writter
    */
    bool writeNewFileOpLog(const string &path, const string &hash);
    bool writeRmFileOpLog(const string &path, const string &hash);
    bool writeMvFileOpLog(const string &srcPath, const string &dstPath, const string &hash);
    bool writeCpFileOpLog(const string &srcPath, const string &dstPath, const string &hash);
    bool writeOpLog(Msg::FileOpType op, const string & pathString, const string &hash);
    bool writeMvFileOpLogTest();

    // restore state from operation log file
    bool restoreStateFromOpLog();

    // get left file ops from given hash and iter
    bool getLeftFileOpsToMsg(Msg::Message &msg, const string &hash);

    // rollback to previous state
    bool rollback();

    // demaon for Vvfs operations of backend
    bool deamon();

    Vvfs()
    {
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