#include "vvfs.hpp"

bool Vvfs::initConfig(const string & vfsPath, const string & vFRLogFile, const string &vFOpLogFile){
    this->vfsPath = vfsPath;
    this->vFRLogFile = vFRLogFile;
    this->vFOpLogFile = vFOpLogFile;
    return true;
}

string VFile::Hash(FileType type, const string & path, off_t size, 
    struct timespec mtime, int fa_idx, int idx, int next_bro_idx, int first_son_idx, int last_son_idx){
    ostringstream oss;
    oss<<type<<path<<size<<mtime.tv_sec<<mtime.tv_nsec<<fa_idx<<idx<<next_bro_idx<<first_son_idx<<last_son_idx;
    return getBufMD5(oss.str().c_str(), oss.str().length());
}

bool Vvfs::updateHashFromVFOpLogFile()
{
    ifstream ifs = ifstream(vFOpLogFile);
    int op;
    long long opTime;
    string path;
    string dstPath;
    while(!ifs.eof()){
        ifs>>op;
        if(ifs.fail()) 
        {
            // logger.fatal("op log file error");
            break;
        }

        if(ifs.eof()) break;

        if(op == Msg::MV_OP)
        {
            ifs >> path >> dstPath >> opTime;
            if(ifs.fail()){
                logger.fatal("read op log fail");
                break;
            }
            updateHashByMvFileOp(path, dstPath, opTime);
        }
        else
        {
            ifs >> path >> opTime;
            if(ifs.fail()){
                logger.fatal("read op log fail");
                break;
            }
            if(op == Msg::NEW_OP) 
                updateHashByNewFileOp(path, opTime);
            else if(op == Msg::RM_OP) 
                updateHashByRMFileOp(path, opTime);
        }
    }
    ifs.close();
    return true;
}

bool Vvfs::createRootVF()
{
    string rootPath = "/";
    int rootIdx = 0;
    VFRelation rootVfr(rootIdx, -1, -1, -1, -1, -1);
    vRelations[rootPath] = rootVfr;
    if(vFiles.size() == 0)
    {
        vFiles.emplace_back("root", VFT_DIR, rootPath, 0, getTimeSpec(), -1,  -1, -1, -1, -1, -1);
    }else{
        logger.fatal("vFiles is not empty, can't create root vf");
    }
    string err;
    if(!activeVF(rootPath, err)){
        logger.fatal("can't active vf");
    }
    logger.debug("success craete root vf");
    return true;
}

bool Vvfs::openVFOpLogFile(){
    pVFOpLogFileFstream = new ofstream(vFOpLogFile,ios::app);
    if(pVFOpLogFileFstream->fail()){
        return false;
    }

    return true;
}

int Vvfs::allocIdx(){
    for(int idx = 0; idx < vFiles.size(); idx++){
        if(!vFiles[idx].isActive()){
            return idx;
        }
    }
    return -1;
}

bool Vvfs::buildVFS()
{
    if(!updateHashFromVFOpLogFile()) {
        logger.fatal("hash init fail");
        return false;
    }

    if(!openVFOpLogFile()){
        logger.fatal("op log file open fail!");
        return false;
    }

    ifstream ifs(vFRLogFile);
    if(ifs.fail()){
        logger.log("vfr config file not exists, now ceate a new vfr config file!!");
        ofstream ofs(vFRLogFile);
        ofs << 0 << endl;
        if(ofs.fail()){
            logger.log(" cann't ceate a new vfr config file!!");
            return false;
        }
        ofs.close();
        if(!createRootVF()){
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
        ifs>>idx>>name>>dirPath>>fa_idx>>prev_bro_idx>>next_bro_idx>>first_son_idx>>last_son_idx>>type>>tv_sec>>tv_nsec>>size>>readHash;
        if(ifs.fail()){
            logger.fatal("read VFRelations error");
        }
        if(idx >= count)
        {
            logger.log("invalid file idx for file" + string(name) + " " + to_string(idx));
            continue;
        }

        VFRelation vfr(idx, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
        string fullPath = string(dirPath) + "/" + name;
        if(dirPath == "/" && name == "root"){
            fullPath = "/";
            isRootExists = true;
        }
        if (vRelations.count(fullPath) != 0) 
        {
            logger.fatal("file path repeat!");
            continue;
        }
        // cout<<fullPath<<endl;
        
        vRelations[fullPath] = vfr;
        timespec tspc = {tv_sec, tv_nsec};
        vFiles[idx] = VFile(name, (FileType)type, dirPath, size, tspc, fa_idx, idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
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

bool Vvfs::mkVDir(const string & name, string & err)
{
    return newVF(name, err, VFT_DIR);
}

bool Vvfs::newVF(const string &path, string &err, FileType type){
    if (path.size() == 0 || path == " ") {
        err = "file name error!";
        logger.log(err);
        return false;
    }

    // if file exists, use origin relations
    if (vRelations.count(path) != 0) {
        err = "file:"+path+" exists!";
        logger.log(err);
        return true;
    }

    int spIdx = path.find_last_of("/");
    string dirPath = path.substr(0, spIdx + 1);
    string name = path.substr(spIdx + 1);
    VFile vf(name, type, dirPath, 0, getTimeSpec(), -1,  -1, -1, -1, -1, -1);
    int newIdx = allocIdx();
    if(newIdx == -1){
        newIdx = vFiles.size();
        vFiles.push_back(vf);
    }else
    {
        vFiles[newIdx] = vf;
    }
    
    // VFRelation newVfr(newIdx, -1, -1, -1, -1, -1);  
    vRelations[path] = move(VFRelation(newIdx, -1, -1, -1, -1, -1));
    logger.debug(dirPath);
    if (vRelations.count(dirPath) == 0) 
    {
        err = "dir path not exist!";
        return false;
    }
    VFile dirVf = vFiles[vRelations[dirPath].idx];
    VFRelation &dirVfr = vRelations[dirPath];
    if (dirVfr.last_son_idx != -1) {
        vRelations[vFiles[dirVfr.last_son_idx].getName()].next_bro_idx = newIdx;
        vRelations[path].prev_bro_idx = dirVfr.last_son_idx;
        dirVfr.last_son_idx = newIdx;
    }else{
        dirVfr.last_son_idx = newIdx;
    }

    if(dirVfr.first_son_idx == -1)
    {
        dirVfr.first_son_idx = newIdx;
    }
    

    vRelations[path].fa_idx = dirVfr.idx;
    dirVf.incSize();



    //todo recursive create
    err ="";
    return true;
}

bool Vvfs::rmVF(const string &name, string &err){
    if (name.size() == 0 || name == " ") {
        err = "file name error!";
        return false;
    }
    if (vRelations.count(name) == 0) {
        err = "file:"+name+"not exists!";
        return false;
    }
    logger.debug("test rm vf");

    int spIdx = name.find_last_of("/");

    VFRelation &vfr = vRelations[name];
    VFile &vf = vFiles[vfr.idx];
    VFile &vfDir = vFiles[vfr.fa_idx];
    // VFRelation vfrDir = vRelations[vfDir.getName()];
    VFRelation &vfrDir = vRelations[vfDir.getVfrKey()];

    if(remove((vfsPath+name).c_str()) == -1){
        // logger.log(string("fail to remove :")+name);
        err = string("fail to remove :")+name;
        return false;
    }

    // perent has one son
    if(vfr.idx == vfrDir.first_son_idx && vfrDir.first_son_idx == vfrDir.last_son_idx){
        vfrDir.first_son_idx = vfrDir.last_son_idx = -1;
    }
    //first son
    else if(vfr.idx == vfrDir.first_son_idx){
        VFRelation &nextVfr = vRelations[vFiles[vfr.next_bro_idx].getName()];
        nextVfr.prev_bro_idx = -1;
        vfrDir.first_son_idx = nextVfr.idx;
    }
    // last son
    else if(vfr.idx == vfrDir.last_son_idx){
        VFRelation &prevVfr = vRelations[vFiles[vfr.prev_bro_idx].getName()];
        prevVfr.next_bro_idx = -1;
        vfrDir.last_son_idx = prevVfr.idx;
    }else{
        VFRelation &nextVfr = vRelations[vFiles[vfr.next_bro_idx].getName()];
        VFRelation &prevVfr = vRelations[vFiles[vfr.prev_bro_idx].getName()];
        prevVfr.next_bro_idx = nextVfr.idx;
        prevVfr.prev_bro_idx = nextVfr.next_bro_idx;
    }

    vf.destory();
    vRelations.erase(name);
    vf.decSize();
    err = "";

    if(!updateHashByRMFileOp(name)){
        logger.fatal("rm hash update error");
    }

    if(!writeRMFileOpLog(name)){
        logger.fatal("rm log write error");
    }
    // logger.debug("rm file:"+name+"ok");
    logger.log(LDEBUG, "rm op ok, path:%s, new hash:%s", name.c_str(), hash.c_str());
    return true;
}

bool Vvfs::lsVF(const string &name, string & fileList, string &err){
    //todo
    return true;
}

bool Vvfs::activeVF(const string &name, string & err){
    if(!vRelations.count(name)){
        err = "no relation for file:"+name;
        return false;
    }
    logger.log(LDEBUG, "vFile size:%d, now active:%d", vFiles.size(), vRelations[name].idx);
    vFiles[vRelations[name].idx].active();
    // cout<<vFiles.size()<<endl;
    isVFRelationsStored = false;

    if(!writeNewFileOpLog(name)){
        return false;
    }

    if(!updateHashByNewFileOp(name)){
        return false;
    }

    logger.debug("new hash:" + hash);
    return true;
}


bool Vvfs::updateHashByNewFileOp(const string &path)
{
    return updateHashByNewFileOp(path, getSystemTime());
}

bool Vvfs::updateHashByNewFileOp(const string &path, long long opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::NEW_OP << "path:" << path << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    hash = hashpkg.getMD5Hex();
    return true;
}

bool Vvfs::updateHashByRMFileOp(const string &path)
{
    return updateHashByRMFileOp(path, getSystemTime());
}

bool Vvfs::updateHashByRMFileOp(const string &path, long long opTime)
{
    ostringstream oss;
    oss << Msg::RM_OP << path << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    hash = hashpkg.getMD5Hex();
    // cout<<hash<<endl;
    return true;
}

bool Vvfs::updateHashByMvFileOp(const string &srcPath, const string &dstPath)
{
    return updateHashByMvFileOp(srcPath, dstPath, getSystemTime());
}

bool Vvfs::updateHashByMvFileOp(const string &srcPath, const string &dstPath, long long opTime)
{
    ostringstream oss;
    oss << "op:" << Msg::RM_OP << "srcPath:" << srcPath << "dstPath:" << dstPath << "time:" << opTime;
    hashpkg.update(oss.str().c_str(), oss.str().size());
    hash = hashpkg.getMD5Hex();
    return true;
}

bool Vvfs::writeOpLog(Msg::FileOpType op, const string & pathString)
{
    *pVFOpLogFileFstream << op << " " << pathString << " " << getSystemTime() <<endl;
    if(pVFOpLogFileFstream->fail()){
        cout<<"write log error"<<endl;
        return false;
    }
    return true;
}

bool Vvfs::writeMvFileOpLogTest(){
    // pVFOpLogFileFstream->write("132");
    (*pVFOpLogFileFstream)<<"4323"<<endl;

    if(pVFOpLogFileFstream->fail()){
        cout<<"fuck"<<endl;
        return false;
    }
    return true;
}

bool Vvfs::writeNewFileOpLog(const string &path)
{
    if(!writeOpLog(Msg::NEW_OP, path)){
        logger.fatal("write new file log fail");
        return false;
    }
    return true;
}
bool Vvfs::writeRMFileOpLog(const string &path)
{
    if(!writeOpLog(Msg::RM_OP, path)){
        logger.fatal("write rm log fail");
        return false;
    }
    return true;
}

bool Vvfs::writeMvFileOpLog(const string &srcPath, const string &dstPath)
{
    string pathString = srcPath + " " + dstPath;
    if(!writeOpLog(Msg::RM_OP, pathString))
    {
        logger.fatal("write mv log fail");
        return false;
    }
    return true;
}



/*
 * store vf relations
 * format:
 *        count
 *        idx; name; dirPath; fa_idx; prev_bro_idx; next_bro_idx; first_son_idx; last_son_idx; type:0 file 1 dir; tv_sec; tv_nsec; size; hash
*/
bool Vvfs::storeVFRelations(){
    // string vfrPath = vfsPath + "/" + ".vfr.tmp.txt";
    string vfrPath = vfsPath + "/" + ".vfr.tmp.txt";
    ofstream ofs(vfrPath);

    if(ofs.fail()){
        logger.fatal("error store VRelations");
        return false;
    }

    ostringstream oss;
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
        oss << pVFR->idx
            << sep << pVF->getName() 
            << sep << pVF->getDirPath()
            << sep << pVFR->fa_idx
            << sep << pVFR->prev_bro_idx
            << sep << pVFR->next_bro_idx 
            << sep << pVFR->first_son_idx
            << sep << pVFR->last_son_idx
            << sep << pVF->getType()
            << sep << pVF->getTvSec()
            << sep << pVF->getTvNsec()
            << sep << pVF->getSize()
            << sep << pVF->getHash()
            << endl;
        ofs<<oss.str();
        if(ofs.fail())
        {
            logger.fatal("write VFRelations fail");
        }
        count++;
    }
    ofs.seekp(0, ios::beg);
    ofs<<count;
    ofs.flush();

    if(ofs.fail()){
        logger.log("sotre VFRations fail!");
        return false;
    }

    ofs.close();
    
    string dstPath = vfsPath + "/vfr.txt";
    cout<<"rename:"<<vfrPath<<" dstPath:"<<dstPath<<endl;
    if(rename(vfrPath.c_str(), dstPath.c_str()) < 0){
        logger.log("move vfr file fail");
        return false;
    }
    return true;
}


void Vvfs::storeVFRelationsBackend()
{
    while(true){
        if(!isVFRelationsStored && storeVFRelations()){
            logger.debug("success store VFRelations");
            isVFRelationsStored = true;
        }
        this_thread::sleep_for(chrono::seconds(VFRELATIONS_BACKEND_INTERVAL));
        logger.debug("store relations");
    }
}

bool Vvfs::deamon(){
    logger.log("starting deamon...");
    thread storeVFRelationsBackendThd(&Vvfs::storeVFRelationsBackend, this);
    storeVFRelationsBackendThd.detach();
    logger.log("deamon started");
    return true;
}

//reverse todo
bool Vvfs::watchVFS(){
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
