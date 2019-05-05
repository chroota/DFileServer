#include "vvfs.hpp"

bool Vvfs::initConfig(const string & vfsPath, const string & vFRLogFile, const string &vFOpLogFile)
{
    string err;
    this->vfsPath = vfsPath;
    if(!mkdirIfNotExist(vfsPath, err))
    {
        logger.fatal("fail to create vfs path:" + err);
        return false;
    }
    this->vFRLogFile = vFRLogFile;
    this->vFOpLogFile = vFOpLogFile;
    return true;
}

bool VFile::updateHash(VFRelation &vfr)
{
    ostringstream oss;
    oss << _type << getFullPath() <<_size << _tv_sec 
        << vfr.fa_idx << vfr.idx << vfr.prev_bro_idx 
        << vfr.next_bro_idx << vfr.first_son_idx 
        << vfr.last_son_idx;

    _hash = getBufMD5(oss.str().c_str(), oss.str().size());
    return true;
}

bool Vvfs::restoreStateFromOpLog()
{
    ifstream ifs = ifstream(vFOpLogFile);
    int op;
    time_t opTime;
    string srcPath;
    string dstPath;
    string stateHash;
    while(!ifs.eof())
    {
        ifs>>op;
        if(ifs.fail() || ifs.eof()) 
            break;
        ifs >> op >> srcPath >> dstPath >> stateHash >> opTime;
        if(ifs.fail())
        {
            logger.fatal("read op log fail");
            break;
        }
        operationList.emplace_back((Msg::FileOpType)op, srcPath, dstPath, stateHash, opTime);
        updateHash(stateHash);
    }
    ifs.close();
    return true;
}

bool Vvfs::createRootVF(time_t opTime)
{
    string rootPath = "/";
    int rootIdx = 0;
    VFRelation rootVfr(rootIdx, -1, -1, -1, -1, -1);
    vRelations[rootPath] = rootVfr;
    if(opTime == -1)
        opTime = getSystemTime();

    if(vFiles.size() == 0)
    {
        vFiles.emplace_back("root", VFT_DIR, rootPath, 0, opTime);
    }else
    {
        logger.fatal("vFiles is not empty, can't create root vf");
    }

    string err;
    vFiles[0].active();
    if(!vFiles[0].updateHash(rootVfr))
        return false;
    if(!newDirEndingWork(rootPath, opTime))
    {
        logger.fatal("new file ending work error");
        return false;
    }
    logger.debug("success craete root vf");
    return true;
}

bool Vvfs::openVFOpLogFile()
{
    pVFOpLogFileFstream = new ofstream(vFOpLogFile,ios::app);
    if(pVFOpLogFileFstream->fail()) 
        return false;
    return true;
}

int Vvfs::allocIdx()
{
    for(int idx = 0; idx < vFiles.size(); idx++)
        if(!vFiles[idx].isActive()) return idx;
    return -1;
}

bool Vvfs::buildVFS()
{
    if(!restoreStateFromOpLog()) 
    {
        logger.fatal("hash init fail");
        return false;
    }

    if(!openVFOpLogFile())
    {
        logger.fatal("op log file open fail!");
        return false;
    }

    ifstream ifs(vFRLogFile);
    if(ifs.fail())
    {
        logger.log("vfr config file not exists, now ceate a new vfr config file!!");
        ofstream ofs(vFRLogFile);
        ofs << 0 << endl;
        if(ofs.fail())
        {
            logger.log(" cann't ceate a new vfr config file!!");
            return false;
        }
        ofs.close();
        if(!createRootVF())
        {
            logger.fatal("create root vf error");
        }
        return true;
    }


    // char name[MAXBUFSIZE], dirPath[MAXBUFSIZE];
    string name, dirPath;
    int idx, prev_bro_idx, next_bro_idx, fa_idx, first_son_idx, last_son_idx, type, count;
    long tv_sec, tv_nsec, size;
    // char hashBuf[17];
    string readHash;

    ifs>>count;
    if(count <= 0)
    {
        logger.log("there are no records in relations");
        createRootVF();
        return true;
    }

    vFiles.resize(count);

    int i = 0;
    logger.log("read file realations from " + vFRLogFile + ", file num:" + to_string(count));

    int isRootExists = false;
    while(i++ < count)
    {
        //idx; name; dirPath; fa_idx; prev_bro_idx; next_bro_idx; first_son_idx; last_son_idx; type:0 file 1 dir; tv_sec; tv_nsec; size; hash
        ifs >> idx >> name >> dirPath >> fa_idx 
            >> prev_bro_idx >> next_bro_idx >> first_son_idx 
            >> last_son_idx >> type >> tv_sec >> tv_nsec 
            >> size >> readHash;
            
        if(ifs.fail())
        {
            logger.fatal("read VFRelations error");
        }
        if(idx >= count)
        {
            logger.log("invalid file idx for file" + string(name) + " " + to_string(idx));
            continue;
        }

        VFRelation vfr(idx, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
        string fullPath = string(dirPath) + "/" + name;
        if(dirPath == "/" && name == "root")
        {
            fullPath = "/";
            isRootExists = true;
        }
        if (vRelations.count(fullPath) != 0) 
        {
            logger.fatal("file path repeat!");
            continue;
        }
        
        vRelations[fullPath] = vfr;
        // timespec tspc = {tv_sec, tv_nsec};
        vFiles[idx] = VFile(name, (FileType)type, dirPath, size, tv_sec);
        vFiles[idx].active();
    }

    ifs.close();
    if(!isRootExists)
    {
        logger.fatal("root path is not exists");
        return false;
    }

    // fclose(fp);
    return true;
}

bool Vvfs::mkVDir(const string &path, string & err, time_t opTime)
{
    string diskPath;
    return newVF(path, 0, err, diskPath, VFT_DIR, opTime);
}

int Vvfs::pushVf2vFiles(const string & name, FileType type, const string & dirPath, off_t size, time_t opTime)
{
    int newIdx = allocIdx();

    if(opTime = -1){
        opTime = getSystemTime();
    }

    if(newIdx == -1)
    {
        newIdx = vFiles.size();
        vFiles.emplace_back(name, type, dirPath, size, opTime);
    }else
    {
        vFiles[newIdx] = VFile(name, type, dirPath, size, opTime);
    }

    return newIdx;
}

bool Vvfs::newVF(const string &path, off_t totalSize, string &err, string &diskPath, FileType type, time_t opTime)
{
    if (path.size() == 0 || path == " ") 
    {
        err = "path error!";
        logger.log(err);
        return false;
    }

    // if file exists, use origin relations
    if (vRelations.count(path) != 0) 
    {
        err = "file:"+path+" exists!";
        logger.log(err);
        return true;
    }

    string dirPath, name;
    if(!getDirPathAndName(path, dirPath, name, err))
        return false;

    int newIdx = pushVf2vFiles(name, type, dirPath, totalSize, opTime);
    VFile &newVf = vFiles[newIdx];
    vRelations[path] = VFRelation(newIdx, -1, -1, -1, -1, -1);
    logger.debug(dirPath);
    if (vRelations.count(dirPath) == 0) 
    {
        err = "dir path not exist!";
        return false;
    }

    VFRelation &dirVfr = vRelations[dirPath];
    vRelations[path].fa_idx = dirVfr.idx;

    if(type == VFT_DIR)
        return activeVF(path, err);

    //todo recursive create
    err = "";
    diskPath = newVf.getDiskPath();
    return true;
}

bool Vvfs::pushbackVf(VFRelation & dirVfr, VFile &vf)
{
    VFRelation &vfr = getVfrByVf(vf);
    VFile & dirVf = vFiles[dirVfr.idx];

    if (dirVfr.last_son_idx != -1) 
    {
        VFRelation & lastSonvfr = getLastSonVfrByVfr(dirVfr);
        lastSonvfr.next_bro_idx = vfr.idx;
        vfr.prev_bro_idx = dirVfr.last_son_idx;
        dirVfr.last_son_idx = vfr.idx;
    }else 
        dirVfr.last_son_idx = vfr.idx;

    if(dirVfr.first_son_idx == -1) 
        dirVfr.first_son_idx = vfr.idx;
    dirVf.incSize();
    vf.updateHash(dirVfr);
    dirVf.updateHash(dirVfr);

    return true;
}

bool Vvfs::removeVfRelation(VFile &vf, bool isVfDestory)
{
    if(vf.getName() == "root")
        return false;
    VFRelation &vfr = getVfrByVf(vf);
    if(vfr.fa_idx == -1)
        return false;
    
    
    VFile & dirVf = vFiles[vfr.fa_idx];
    VFRelation &dirVfr = getVfrByVf(dirVf);

    // perent has one son
    if(vfr.idx == dirVfr.first_son_idx && dirVfr.first_son_idx == dirVfr.last_son_idx)
    {
        dirVfr.first_son_idx = dirVfr.last_son_idx = -1;
    }
    //first son
    else if(vfr.idx == dirVfr.first_son_idx)
    {
        VFRelation &nextVfr = getNextVfrByVfr(vfr);
        nextVfr.prev_bro_idx = -1;
        dirVfr.first_son_idx = nextVfr.idx;
    }
    // last son
    else if(vfr.idx == dirVfr.last_son_idx)
    {
        VFRelation &prevVfr = getPrevVfrByVfr(vfr);
        prevVfr.next_bro_idx = -1;
        dirVfr.last_son_idx = prevVfr.idx;
    }else
    {
        VFRelation &nextVfr = getNextVfrByVfr(vfr);
        VFRelation &prevVfr = getPrevVfrByVfr(vfr);
        prevVfr.next_bro_idx = nextVfr.idx;
        nextVfr.prev_bro_idx = prevVfr.idx;
    }

    if(isVfDestory)
        vf.destory();
    vRelations.erase(vf.getVfrKey());
    dirVf.decSize();
    return true;
}

bool Vvfs::changeVfDirRelation(VFile &vf, VFRelation &newDirVfr, const string &newDirPath, bool isVfDestory)
{
    removeVfRelation(vf, false);
    vf.setDirPath(newDirPath);
    pushbackVf(newDirVfr, vf);
    return true;
}


bool Vvfs::mvVF(const string &srcPath, const string &dstPath, string &err, time_t opTime)
{
    if (srcPath.size() == 0 || srcPath == " " || dstPath.size() == 0 || dstPath == " ") 
    {
        err = "move: file name error!";
        logger.log(err);
        return false;
    }
    if (vRelations.count(srcPath) == 0) 
    {
        err = "move: src file "+srcPath+" not exists!";
        logger.log(err);
        return false;
    }
    logger.debug("test move vf");
    
    string dstDirPath, dstName;
    string srcDirPath, srcName;
    if(!getDirPathAndName(dstPath, dstDirPath, dstName, err) || !getDirPathAndName(srcPath, srcDirPath, srcName, err))
        return false;

    if(vRelations.count(dstDirPath) == 0)
    {
        err = "move: dst path " + dstDirPath +" not exists!";
        logger.log(err);
        return false;
    }

    VFRelation &srcVfr    = vRelations[srcPath];
    VFRelation &srcDirVfr = vRelations[srcDirPath];
    VFRelation &dstDirVfr = vRelations[dstDirPath];
    VFile &srcVf          = vFiles[srcVfr.idx];
    VFile &dstDirVf       = vFiles[dstDirVfr.idx];
    VFile &srcDirVf       = vFiles[srcDirVfr.idx];

    vRelations[dstPath] = VFRelation(srcVfr.idx, dstDirVfr.idx, -1, -1, -1, -1);

    if(opTime == -1){
        opTime = getSystemTime();
    }

    if(!changeVfDirRelation(srcVf, dstDirVfr, dstDirPath))
    {
        err = "change vf relation error";
        return false;
    }

    if(!mvFileEndingWork(srcPath, dstPath))
    {
        logger.fatal("mv file ending work error");
        return false;
    }

    return true;
}


bool Vvfs::copyDiskFile(const string& srcPath, const string &dstPath)
{
    try
    {
        ifstream in(srcPath,ios::binary);
        ofstream out(dstPath,ios::binary);
        if (!in.is_open()) {
            cout << "error open file " << srcPath << endl;
            return false;
        }
        if (!out.is_open()) {
            cout << "error open file " << dstPath << endl;
            return false;
        }
        if (srcPath == dstPath) {
            cout << "the src file can't be same with dst file" << endl;
            return false;
        }
        char buf[2048];
        long long totalBytes = 0;
        while(in)
        {
            in.read(buf, 2048);    
            out.write(buf, in.gcount());    
            totalBytes += in.gcount();
        }
        in.close();
        out.close();
        return true;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << endl;
        return false;
    }
}

bool Vvfs::cpVF(const string &srcPath, const string &dstPath, string &err, time_t opTime)
{
    if (srcPath.size() == 0 || srcPath == " " || dstPath.size() == 0 || dstPath == " ") 
    {
        err = "cp: file name error!";
        logger.log(err);
        return false;
    }
    if (vRelations.count(srcPath) == 0) 
    {
        err = "cp: src file "+srcPath+" not exists!";
        logger.log(err);
        return false;
    }
    logger.debug("test cp vf");
    
    string dstDirPath, dstName;
    string srcDirPath, srcName;
    if(!getDirPathAndName(dstPath, dstDirPath, dstName, err) || !getDirPathAndName(srcPath, srcDirPath, srcName, err))
        return false;

    if(vRelations.count(dstDirPath) == 0)
    {
        err = "cp: dst path " + dstDirPath +" not exists!";
        logger.log(err);
        return false;
    }

    VFRelation &srcVfr    = vRelations[srcPath];
    VFRelation &srcDirVfr = vRelations[srcDirPath];
    VFRelation &dstDirVfr = vRelations[dstDirPath];
    VFile &srcVf          = vFiles[srcVfr.idx];
    VFile &dstDirVf       = vFiles[dstDirVfr.idx];
    VFile &srcDirVf       = vFiles[srcDirVfr.idx];

    int newIdx = pushVf2vFiles(srcVf.getName(), srcVf.getType(), dstDirPath, srcVf.getSize());
    VFile &newVf = vFiles[newIdx];
    vRelations[dstPath] = VFRelation(newIdx, dstDirVfr.idx, -1, -1, -1, -1);
    VFRelation & newVfr = vRelations[dstPath];
    if(!pushbackVf(dstDirVfr, newVf))
    {
        err = "cp: err at create new VF";
        return false;
    }

    // count use srcvf, pointer has changed
    if(!copyDiskFile(getVFPhasicalPath(vFiles[srcVfr.idx]), getVFPhasicalPath(newVf)))
    {
        err = "cp: err at copy disk file";
        return false;
    }
    newVf.active();

    if(opTime == -1){
        opTime = getSystemTime();
    }

    if(!cpFileEndingWork(srcPath, dstPath, opTime))
    {
        logger.fatal("cp file ending work error");
        return false;
    }

    return true;
}


bool Vvfs::activeVF(const string &path, string & err)
{
    if(!vRelations.count(path))
    {
        err = "no relation for file:"+path;
        return false;
    }

    VFRelation &vfr = vRelations[path];
    VFile &vf = vFiles[vfr.idx];

    VFile &dirVf = vFiles[vfr.fa_idx];
    VFRelation &dirVfr = getVfrByIdx(vfr.fa_idx);
    if (dirVfr.last_son_idx != -1) 
    {
        VFRelation & lastSonvfr = getLastSonVfrByVfr(dirVfr);
        lastSonvfr.next_bro_idx = vfr.idx;
        vfr.prev_bro_idx = dirVfr.last_son_idx;
        dirVfr.last_son_idx = vfr.idx;
    }else 
        dirVfr.last_son_idx = vfr.idx;

    if(dirVfr.first_son_idx == -1) 
        dirVfr.first_son_idx = vfr.idx;

    dirVf.incSize();
    vf.active();
    isVFRelationsStored = false;

    if(!vf.updateHash(vfr) || !dirVf.updateHash(dirVfr))
        return false;

    if(vf.getType() == VFT_FILE && !newFileEndingWork(path, vf.getTvSec()))
    {
        logger.fatal("new file ending work error");
        return false;
    }

    if(vf.getType() == VFT_DIR && !newDirEndingWork(path, vf.getTvSec()))
    {
        logger.fatal("new dir ending work error");
        return false;
    }

    logger.log(LDEBUG, "vFile size:%d, now active:%d", vFiles.size(), vfr.idx);
    logger.debug("new hash:" + hash);
    return true;
}

bool Vvfs::syncNewVfOp(const string &path, const string &hash, time_t opTime)
{
    // return newVF(path, );
}

bool Vvfs::syncNewVDirOp(const string &path, const string &hash, time_t opTime)
{
    string err;
    if(path == "/")
    {
        if(!createRootVF(opTime))
            logger.fatal("create root vf fail");
    }else
    {
        if(!mkVDir(path, err, opTime))
        {
            logger.fatal(err);
        }
    }

    if(hash != this->hash)
    {
        string tip =  "sync hash not same, src hash: " + hash + " after create dir hash: " + this->hash + ", now rollbacking....";
        logger.log(tip);
        if(!rollback()){
            logger.fatal("rollback error");
        }
        return false;
    }

    return true;
}

bool Vvfs::syncRmVfOp(const string &path, const string &hash, time_t opTime)
{

}

bool Vvfs::syncMvVfOp(const string &srcPath, const string &dstPath, const string &hash, time_t opTime)
{

}

bool Vvfs::syncCpVfOp(const string &srcPath, const string &dstPath, const string &hash, time_t opTime)
{

}


bool Vvfs::rollback()
{
    //todo
    return false;
}

string Vvfs::getVFPhasicalPath(VFile & vf)
{
    return vfsPath + "/" + vf.getDiskPath();
}

string Vvfs::getVFPhasicalPath(string &vfDiskPath)
{
    return vfsPath + "/" + vfDiskPath;
}

bool Vvfs::rmSubfolder(VFile &vf, string &err)
{
    if(vf.getSize() == 0) return true;

    queue<int> idxQ;
    VFRelation &vfr = getVfrByVf(vf);
    VFRelation &firstSonVfr = getFirstSonVfrByVfr(vfr);
    idxQ.push(firstSonVfr.idx);
    int nextIdx = firstSonVfr.next_bro_idx;
    while (nextIdx != -1)
    {
        idxQ.push(nextIdx);
        nextIdx = getNextIdxByIdx(nextIdx);
    }

    while (!idxQ.empty())
    {
        int idx = idxQ.front();
        idxQ.pop();
        VFile &vf = vFiles[idx];
        VFRelation &vfr = getVfrByIdx(idx);
        if(vfr.next_bro_idx  != -1) 
            idxQ.push(vfr.next_bro_idx);
        if(vfr.first_son_idx != -1) 
            idxQ.push(vfr.first_son_idx);
        vf.destory();
        vRelations.erase(vf.getVfrKey());
        string diskPath = getVFPhasicalPath(vf);

        //todo deal remove i/o backgground thread
        if(vf.getType() == VFT_FILE && remove(diskPath.c_str()) == -1)
        {
            logger.log("fail to remove " + diskPath);
            err = "fail to remove " + diskPath;
        }
    }
    return true;
}

bool Vvfs::rmVF(const string &path, string &err, time_t opTime)
{
    if (path.size() == 0 || path == " ") {
        err = "file name error!";
        return false;
    }
    if (vRelations.count(path) == 0) {
        err = "file:"+path+" not exists!";
        return false;
    }
    logger.debug("test rm vf");

    int spIdx = path.find_last_of("/");

    VFRelation &vfr = vRelations[path];
    VFile &vf = vFiles[vfr.idx];

    VFile &vfDir = vFiles[vfr.fa_idx];
    // VFRelation vfrDir = vRelations[vfDir.getName()];
    VFRelation &vfrDir = vRelations[vfDir.getVfrKey()];
    string diskPath = getVFPhasicalPath(vf);
    if(vf.getType() == VFT_FILE && remove(diskPath.c_str()) == -1)
    {
        // logger.log(string("fail to remove :")+name);
        err = string("fail to remove ")+path;
        return false;
    }

    if(vf.getType() == VFT_DIR && !rmSubfolder(vf, err))
        return false;

    // perent has one son
    if(vfr.idx == vfrDir.first_son_idx && vfrDir.first_son_idx == vfrDir.last_son_idx)
    {
        vfrDir.first_son_idx = vfrDir.last_son_idx = -1;
    }
    //first son
    else if(vfr.idx == vfrDir.first_son_idx)
    {
        VFRelation &nextVfr  = getNextVfrByVfr(vfr);
        nextVfr.prev_bro_idx = -1;
        vfrDir.first_son_idx = nextVfr.idx;
    }
    // last son
    else if(vfr.idx == vfrDir.last_son_idx)
    {
        VFRelation &prevVfr  = getPrevVfrByVfr(vfr);
        prevVfr.next_bro_idx = -1;
        vfrDir.last_son_idx  = prevVfr.idx;
    }else
    {
        VFRelation &nextVfr  = getNextVfrByVfr(vfr);
        VFRelation &prevVfr  = getPrevVfrByVfr(vfr);
        prevVfr.next_bro_idx = nextVfr.idx;
        nextVfr.prev_bro_idx = prevVfr.idx;
    }

    vf.destory();
    vRelations.erase(path);
    vfDir.decSize();

    err = "";
    // if(!updateHashByRmFileOp(path))
    // {
    //     logger.fatal("rm hash update error");
    // }

    // if(!writeRmFileOpLog(path))
    // {
    //     logger.fatal("rm log write error");
    // }

    if(opTime == -1){
        opTime = getSystemTime();
    }

    if(!rmFileEndingWork(path), opTime)
    {
        logger.fatal("remove file ending work error");
        return false;
    }

    // logger.debug("rm file:"+name+"ok");
    logger.log(LDEBUG, "rm op ok, path:%s, new hash:%s", path.c_str(), hash.c_str());
    return true;
}



VFile & Vvfs::getVFByVfr(VFRelation & vfr)
{
    return vFiles[vfr.idx];
}

VFRelation &Vvfs::getVfrByVf(VFile &vf)
{
    string path = vf.getVfrKey();
    return vRelations[path];
}

VFRelation &Vvfs::getVfrByIdx(int idx)
{
    VFile & vf = vFiles[idx];
    return getVfrByVf(vf);
}

int Vvfs::getIdxByVf(VFile &vf)
{
    VFRelation &vfr = getVfrByVf(vf);
    return vfr.idx;
}

VFRelation &Vvfs::getNextVfrByVfr(VFRelation &vfr)
{
    return vRelations[vFiles[vfr.next_bro_idx].getVfrKey()];
}

int Vvfs::getNextIdxByIdx(int idx)
{
    if(idx == -1) return -1;
    VFile &vf = vFiles[idx];
    VFRelation &vfr = vRelations[vf.getVfrKey()];
    return vfr.next_bro_idx;
}

VFRelation &Vvfs::getPrevVfrByVfr(VFRelation &vfr)
{
    return vRelations[vFiles[vfr.prev_bro_idx].getVfrKey()];
}


VFRelation &Vvfs::getLastSonVfrByVfr(VFRelation &vfr)
{
    return vRelations[vFiles[vfr.last_son_idx].getVfrKey()];
}

VFRelation &Vvfs::getFirstSonVfrByVfr(VFRelation &vfr){
    return vRelations[vFiles[vfr.first_son_idx].getVfrKey()];
}

bool Vvfs::lsVF(const string & path, Msg::Message &msg)
{
    string err;
    if (path.size() == 0 || path == " ") 
    {
        err = "ls: file name error!";
        LsFileMsgResInst(msg, Msg::MSG_RES_ERROR, err);
        logger.log(err);
        return false;
    }
    if (vRelations.count(path) == 0) 
    {
        err = "ls file:"+path+" not exists!";
        LsFileMsgResInst(msg, Msg::MSG_RES_ERROR, err);
        logger.log(err);
        return false;
    }

    logger.debug("test ls vf");
    VFRelation &vfr = vRelations[path];
    VFile &vf = vFiles[vfr.idx];

    AddAttributeToLsFileMsg(msg, vf.getName(), vf.getSize(), (Msg::FileType)vf.getType(), getTimeStringFromTvSec(vf.getTvSec()));
    if(vf.getType() == VFT_FILE || vf.getSize() == 0)
        return true;

    // cout<<vfr.first_son_idx<<endl;
    VFRelation &firstSonVfr = getVfrByIdx(vfr.first_son_idx);
    VFile &firstSonVf = vFiles[vfr.first_son_idx];
    AddAttributeToLsFileMsg(msg, firstSonVf.getName(), firstSonVf.getSize(), (Msg::FileType)firstSonVf.getType(), getTimeStringFromTvSec(firstSonVf.getTvSec()));
    int tmpVfrIdx = firstSonVfr.next_bro_idx;
    while (tmpVfrIdx != -1)
    {
        VFRelation & tmpVfr = getVfrByIdx(tmpVfrIdx);
        VFile & tmpVf = vFiles[tmpVfrIdx];
        AddAttributeToLsFileMsg(msg, tmpVf.getName(), tmpVf.getSize(), (Msg::FileType)tmpVf.getType(), getTimeStringFromTvSec(tmpVf.getTvSec()));
        tmpVfrIdx = tmpVfr.next_bro_idx;
    }
    LsFileMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    return true;
}


bool Vvfs::getDirPathAndName(const string &path, string &dirPath, string &name, string &err)
{
    try
    {
        int spIdx = path.find_last_of("/");
        if(spIdx == -1) 
            return false;
        if (spIdx == 0) 
            dirPath = "/";
        else
            dirPath = path.substr(0, spIdx);
        name = path.substr(spIdx + 1);
        if(name.size() == 0) 
            return false;
        return true;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
}


string Vvfs::updateHashByNewFileOp(const string &path, time_t opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::NEW_OP << "path:" << path << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    string newestHash = hashpkg.getMD5Hex();
    updateHash(newestHash);
    return newestHash;
}

string Vvfs::updateHashByRmFileOp(const string &path, time_t opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::RM_OP << "path:" << path << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    string newestHash = hashpkg.getMD5Hex();
    updateHash(newestHash);
    return newestHash;
}

string Vvfs::updateHashByMvFileOp(const string &srcPath, const string &dstPath, time_t opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::RM_OP << "srcPath:" << srcPath << "dstPath:" << dstPath << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    string newestHash = hashpkg.getMD5Hex();
    updateHash(newestHash);
    return newestHash;
}

string Vvfs::updateHashByCpFileOp(const string &srcPath, const string &dstPath, time_t opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::RM_OP << "srcPath:" << srcPath << "dstPath:" << dstPath << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    string newestHash = hashpkg.getMD5Hex();
    updateHash(newestHash);
    return newestHash;
}

bool Vvfs::updateHash(const string &newestHash)
{
    hash = newestHash;
    return true;
}


bool Vvfs::writeMvFileOpLogTest()
{
    (*pVFOpLogFileFstream)<<"4323"<<endl;

    if(pVFOpLogFileFstream->fail())
    {
        cout<<"fuck"<<endl;
        return false;
    }
    return true;
}

bool Vvfs::writeOpLog(Msg::FileOpType op, const string & pathString, const string &hash)
{
    *pVFOpLogFileFstream << op << " " << pathString << " "<< hash << " " << getSystemTime() <<endl;
    if(pVFOpLogFileFstream->fail())
    {
        cout<<"write log error"<<endl;
        return false;
    }
    return true;
}

bool Vvfs::writeNewFileOpLog(const string &path, const string &hash)
{
    string pathString  = path + " " + "null";
    if(!writeOpLog(Msg::NEW_OP, pathString, hash))
    {
        logger.fatal("write new file log fail");
        return false;
    }
    return true;
}
bool Vvfs::writeRmFileOpLog(const string &path, const string &hash)
{
    string pathString  = path + " " + "null";
    if(!writeOpLog(Msg::RM_OP, pathString, hash))
    {
        logger.fatal("write rm log fail");
        return false;
    }
    return true;
}

bool Vvfs::writeMvFileOpLog(const string &srcPath, const string &dstPath, const string &hash)
{
    string pathString = srcPath + " " + dstPath;
    if(!writeOpLog(Msg::RM_OP, pathString, hash))
    {
        logger.fatal("write mv log fail");
        return false;
    }
    return true;
}


bool Vvfs::writeCpFileOpLog(const string &srcPath, const string &dstPath, const string &hash)
{
    string pathString = srcPath + " " + dstPath;
    if(!writeOpLog(Msg::CP_OP, pathString, hash))
    {
        logger.fatal("write mv log fail");
        return false;
    }
    return true;
}

bool Vvfs::appenNewFileOperation(FileOperation && op)
{
    try
    {
        operationList.push_back(move(op));
        return true;
    }
    catch(const std::exception& e)
    {
        logger.fatal(e.what());
        return false;
    }
}

bool Vvfs::newFileEndingWork(const string &path, time_t opTime)
{
    logger.debug("new file ending work");
    // time_t opTime = getSystemTime();
    if(opTime == -1)
        opTime = getSystemTime();
    string newestHash = updateHashByNewFileOp(path, opTime);
    appenNewFileOperation(move(FileOperation(Msg::NEW_OP, path, "", newestHash, opTime)));
    // writeNewFileOpLog(path);
    return true;
}

bool Vvfs::newDirEndingWork(const string &path, time_t opTime)
{
    logger.debug("new file ending work");
    if(opTime == -1)
        opTime = getSystemTime();
    string newestHash = updateHashByNewFileOp(path, opTime);
    appenNewFileOperation(move(FileOperation(Msg::NEW_DIR_OP, path, "", newestHash, opTime)));
    // writeNewFileOpLog(path);
    return true;
}

bool Vvfs::rmFileEndingWork(const string &path, time_t opTime)
{
    logger.debug("rm file ending work");
    if(opTime == -1)
        opTime = getSystemTime();
    string newestHash = updateHashByRmFileOp(path, opTime);
    appenNewFileOperation(move(FileOperation(Msg::RM_OP, path, "", newestHash, opTime)));
    // writeRmFileOpLog(path);
    return true;
}


bool Vvfs::mvFileEndingWork(const string &srcPath, const string &dstPath, time_t opTime)
{
    logger.debug("mv file ending work");
    if(opTime == -1)
        opTime = getSystemTime();
    string newestHash = updateHashByMvFileOp(srcPath, dstPath, opTime);
    appenNewFileOperation(move(FileOperation(Msg::MV_OP, srcPath, dstPath, newestHash, opTime)));
    // writeMvFileOpLog(srcPath, dstPath);
    return true;
}

bool Vvfs::cpFileEndingWork(const string &srcPath, const string &dstPath, time_t opTime)
{
    logger.debug("cp file ending work");
    if(opTime == -1)
        opTime = getSystemTime();
    string newestHash = updateHashByCpFileOp(srcPath, dstPath, opTime);
    appenNewFileOperation(move(FileOperation(Msg::CP_OP, srcPath, dstPath, newestHash, opTime)));
    // writeCpFileOpLog(srcPath, dstPath);
    return true;
}


bool Vvfs::getLeftFileOpsToMsg(Msg::Message &msg, const string &hash)
{
    list<FileOperation>::iterator tmpIter = operationList.begin();
    while (tmpIter->stateHash != hash && tmpIter != operationList.end())
    {
        tmpIter++;
    }

    // not found
    if(tmpIter == operationList.end())
        tmpIter = operationList.begin();
    while (tmpIter != operationList.end())
    {
        AddOperationToOpsMsg(msg, tmpIter->type, tmpIter->srcPath,
            tmpIter->dstPath, tmpIter->stateHash, tmpIter->opTime);
        tmpIter++;
    }
    return true;
}


/*
 * store vf relations
 * format:
 *        count
 *        idx; name; dirPath; fa_idx; prev_bro_idx; next_bro_idx; first_son_idx; last_son_idx; type:0 file 1 dir; tv_sec; tv_nsec; size; hash
*/
bool Vvfs::storeVFRelations()
{
    // string vfrPath = vfsPath + "/" + ".vfr.tmp.txt";
    string vfrPath = vfsPath + "/" + ".vfr.tmp.txt";
    ofstream ofs(vfrPath);

    if(ofs.fail())
    {
        logger.fatal("error store VRelations");
        return false;
    }

    map<string, VFRelation>::iterator it;

    int count = 0;
    ofs << count << endl;
    string sep = " ";
    for( it = vRelations.begin(); it != vRelations.end(); it++)
    {
        if(!vFiles[it->second.idx].isActive())
        {
            cout<<"not active"<<endl;
            continue;
        }
        VFRelation *pVFR = &it->second;
        VFile *pVF = &vFiles[pVFR->idx];
        //ostringstream oss;
        ofs << pVFR->idx << sep << pVF->getName() 
            << sep << pVF->getDirPath() << sep << pVFR->fa_idx << sep << pVFR->prev_bro_idx
            << sep << pVFR->next_bro_idx << sep << pVFR->first_son_idx
            << sep << pVFR->last_son_idx << sep << pVF->getType()
            << sep << pVF->getTvSec() << sep << pVF->getTvNsec() 
            << sep << pVF->getSize()  << sep << pVF->getHash()
            << endl;
        //ofs<<oss.str();
        if(ofs.fail())
            logger.fatal("write VFRelations fail");
        count++;
    }
    ofs.seekp(0, ios::beg);
    ofs<<count;
    ofs.flush();

    if(ofs.fail())
    {
        logger.log("sotre VFRations fail!");
        return false;
    }

    ofs.close();
    string dstPath = vfsPath + "/vfr.txt";
    cout<<"rename:"<<vfrPath<<" dstPath:"<<dstPath<<endl;
    if(rename(vfrPath.c_str(), dstPath.c_str()) < 0)
    {
        logger.log("move vfr file fail");
        return false;
    }
    return true;
}


void Vvfs::storeVFRelationsBackend()
{
    while(true)
    {
        if(!isVFRelationsStored && storeVFRelations())
        {
            logger.debug("success store VFRelations");
            isVFRelationsStored = true;
        }
        this_thread::sleep_for(chrono::seconds(VFRELATIONS_BACKEND_INTERVAL));
        logger.debug("store relations");
    }
}

bool Vvfs::deamon()
{
    logger.log("starting deamon...");
    thread storeVFRelationsBackendThd(&Vvfs::storeVFRelationsBackend, this);
    storeVFRelationsBackendThd.detach();
    logger.log("deamon started");
    return true;
}

//reverse todo
bool Vvfs::watchVFS()
{
    int fd;
	int wd;
	int len;
	int nread;
	char buf[BUFSIZ];
	struct inotify_event *event;
	int i;
    ostringstream info;
    fd = inotify_init();
    if (fd < 0)
	{
        logger.log("inotify_init failed");
		return -1;
	}

    wd = inotify_add_watch(fd, vfsPath.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY );
    if (wd < 0)
	{
        info<<"inotify_add_watch "<<vfsPath<<"failed";
        logger.log(info.str());
        info.flush();
		return -1;
	}
 
	buf[sizeof(buf) - 1] = 0;

    //todo watch changes of files
    return true;
}
