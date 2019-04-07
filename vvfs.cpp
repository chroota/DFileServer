#include "vvfs.hpp"

bool Vvfs::initConfig(const string & vfsPath, const string & vFrLogFile, const string &vFOpLogFile){
    this->vfsPath = vfsPath;
    this->vFrLogFile = vFrLogFile;
    this->vFOpLogFile = vFOpLogFile;
    return true;
}

string VFile::Hash(FileType type, const string & path, off_t size, 
    struct timespec mtime, int fa_idx, int idx, int next_bro_idx, int first_son_idx, int last_son_idx){
    ostringstream oss;
    oss<<type<<path<<size<<mtime.tv_sec<<mtime.tv_nsec<<fa_idx<<idx<<next_bro_idx<<first_son_idx<<last_son_idx;
    return getBufMD5(oss.str().c_str(), oss.str().length());
}

bool Vvfs::initHash()
{
    int op;
    long long opTime;
    string path;
    string dstPath;
    while(!pVFOpLogFileFstream->eof()){
        *pVFOpLogFileFstream>>op;
        if(op != Msg::NEW_OP || op != Msg::RM_OP || op != Msg::MV_OP) break;
        if(op == Msg::MV_OP)
        {
            *pVFOpLogFileFstream >> path >> dstPath >> opTime;
            updateHashByMvFileOp(path, dstPath, opTime);
        }
        else
        {
            *pVFOpLogFileFstream >> path >> opTime;
            if(op == Msg::NEW_OP) updateHashByNewFileOp(path, opTime);
            else if(op == Msg::RM_OP) updateHashByRMFileOp(path, opTime);
        }
    }
    return true;
}

bool Vvfs::createRootVF(){
    string rootPath = "/";
    int rootIdx = 0;
    VFRelation rootVfr(rootIdx, -1, -1, -1, -1, -1);
    vRelations[rootPath] = rootVfr;
    VFile rootVf("root", VFT_DIR, rootPath, 0, getTimeSpec(), -1,  -1, -1, -1, -1, -1);
    if(vFiles.size() == 0){
        vFiles.push_back(rootVf);
    }else{
        vFiles[rootIdx] = rootVf;
    }
    logger.debug("success craete root vf");
    return true;
}

bool Vvfs::buildVFS()
{
    pVFOpLogFileFstream = new fstream(vFOpLogFile, ios::in | ios::out | ios::app);
    if(pVFOpLogFileFstream->fail()){
        logger.fatal("op log file open fail!");
        return false;
    }

    // *pVFOpLogFileFstream<<123<<endl;

    if(!initHash()) {
        logger.fatal("hash init fail");
        return false;
    }

    FILE *fp;
    fp = fopen(vFrLogFile.c_str(), "r");
    if(fp == NULL){
        logger.log("vfr config file not exists, now ceate a new vfr config file!!", L4);
        fp = fopen(vFrLogFile.c_str(), "w");
        char count = '0';
        fputc(count, fp);
        if(!fp) {
            logger.log(" cann't ceate a new vfr config file!!");
            return false;
        }
        fclose(fp);
        createRootVF();
        return true;
    }

    char name[MAXBUFSIZE], dirPath[MAXBUFSIZE];
    int idx, prev_bro_idx, next_bro_idx, fa_idx, first_son_idx, last_son_idx, type, count;
    long tv_sec, tv_nsec, size;

    char hashBuf[17];

     // todo exception deal
    fscanf(fp, "%d", &count);

    if(count <= 0){
        logger.log("there are no records in relations");
        createRootVF();
        return true;
    }

    vFiles.resize(count);

    int i = 0;
    logger.log("read file realations from " + vFrLogFile + ", file num:" + to_string(count));

    while(!feof(fp) && i++ < count)
    {

        fscanf(fp, 
            "%d %s %s %d %d %d %d %d %d %ld %ld %ld %s", 
            &idx, name, dirPath, &fa_idx, &prev_bro_idx, &next_bro_idx, 
            &first_son_idx, &last_son_idx,&type, &tv_sec, &tv_nsec, 
            &size, hashBuf
        );

        // printf("%d %s %s %d %d %d %d %d %d %ld %ld %ld %s\n", idx, name, path, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx, type, tv_sec, tv_nsec, size, hashBuf);

        if(idx >= count){
            logger.log("invalid file idx for file" + string(name) + " " + to_string(idx));
            continue;
        }

        VFRelation vfr(idx, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
        string fullPath = string(dirPath) + "/" + name;
        if (vRelations.count(fullPath) != 0) {
            logger.log("file path repeat!");
        }
        
        vRelations[fullPath] = vfr;
        timespec tspc = {tv_sec, tv_nsec};
        VFile vf(name, (FileType)type, dirPath, size, tspc, fa_idx, idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
        vFiles[idx] = vf;
    }

    fclose(fp);
    return true;
}


bool Vvfs::mkVDir(const string & name, string & err)
{
    return newVF(name, err, VFT_DIR);
}

bool Vvfs::newVF(const string &name, string &err, FileType type){
    // cout<<vRelations.size()<<endl;
    // cout<< "newVF:" <<name<<endl;
    if (name.size() == 0 || name == " ") {
        err = "file name error!";
        logger.log(err);
        return false;
    }

    // if file exists, use origin relations
    if (vRelations.count(name) != 0) {
        err = "file:"+name+" exists!";
        logger.log(err);
        return true;
    }

    int spIdx = name.find_last_of("/");
    string dirPath = name.substr(0, spIdx + 1);

    int newIdx = allocIdx();
    VFRelation newVfr(newIdx, -1, -1, -1, -1, -1);  
    vRelations[name] = newVfr;
    if (vRelations.count(dirPath) == 0) 
    {
        err = "dir path not exist!";
        return false;
    }
    VFile vf(name, type, dirPath, 0, getTimeSpec(), -1,  -1, -1, -1, -1, -1);
    VFile dirVf = vFiles[vRelations[dirPath].idx];
    VFRelation dirVfr = vRelations[name];
    if (dirVfr.last_son_idx != -1) {
        vRelations[vFiles[dirVfr.last_son_idx].getName()].next_bro_idx = newIdx;
        dirVfr.last_son_idx = newIdx;
    }else{
        dirVfr.last_son_idx = newIdx;
    }

    newVfr.fa_idx = dirVfr.idx;
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

    int spIdx = name.find_last_of("/");

    VFRelation vfr = vRelations[name];
    VFile vf = vFiles[vfr.idx];
    VFile vfDir = vFiles[vfr.fa_idx];
    VFRelation vfrDir = vRelations[vfDir.getName()];

    if(remove((vfsPath+name).c_str()) == -1){
        // logger.log(string("fail to remove :")+name);
        err = string("fail to remove :")+name;
        return false;
    }

    //first son
    if(vfr.idx == vfrDir.first_son_idx){
        VFRelation *pNextVfr = &(vRelations[vFiles[vfr.next_bro_idx].getName()]);
        pNextVfr->prev_bro_idx = -1;
        vfrDir.first_son_idx = pNextVfr->idx;
    }
    // last son
    else if(vfr.idx == vfrDir.last_son_idx){
        VFRelation *pPrevVfr = &(vRelations[vFiles[vfr.prev_bro_idx].getName()]);
        pPrevVfr->next_bro_idx = -1;
        vfrDir.last_son_idx = pPrevVfr->idx;
    }else{
        VFRelation *pNextVfr = &(vRelations[vFiles[vfr.next_bro_idx].getName()]);
        VFRelation *pPrevVfr = &(vRelations[vFiles[vfr.prev_bro_idx].getName()]);
        pPrevVfr->next_bro_idx = pNextVfr->idx;
        pPrevVfr->prev_bro_idx = pNextVfr->next_bro_idx;
    }

    vf.destory();
    vRelations.erase(name);
    vf.decSize();
    err = "";
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
    vFiles[vRelations[name].idx].active();

    if(!writeNewFileOpLog(name)){
        return false;
    }

    if(!updateHashByNewFileOp(name)){
        return false;
    }

    logger.debug("new hash:"+hash);

    return true;
}


bool Vvfs::storeVFR(){
    ofstream ofs(".vfr.tmp.txt");

    if(ofs.fail()){
        logger.fatal("error store VRelations");
        return false;
    }

    ostringstream oss;
    map<string, VFRelation>::iterator it;

    //count
    //idx; name; dirPath; fa_idx; prev_bro_idx; next_bro_idx; first_son_idx; last_son_idx; type:0 file 1 dir; tv_sec; tv_nsec; size; hash

    ofs<<vRelations.size()<<endl;
    for( it = vRelations.begin(); it != vRelations.end(); it++)
    {
        VFRelation *pVFR = &it->second;
        VFile *pVF = &vFiles[pVFR->idx];
        string sep = " ";
        oss <<pVFR->idx
            << sep << pVF->getName() 
            << sep << pVF->getDirPath()
            << sep << pVFR->fa_idx
            << sep << pVFR->next_bro_idx 
            << sep << pVFR->first_son_idx
            << sep << pVFR->last_son_idx
            << sep << pVF->getType()
            << sep << pVF->getTvSec()
            << sep << pVF->getTvNsec()
            << sep << pVF->getSize()
            << sep << pVF->getHash()
            <<endl;
    }
    ofs<<oss.str();
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
    hash = hashpkg.update(oss.str().c_str(), oss.str().size()); 
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
    // cout<<"fuck"<<endl;
    // *pVFOpLogFileFstream << op << " " << pathString << " " << getSystemTime() <<endl;
    // *pVFOpLogFileFstream << "fuck"<<endl;
    (*pVFOpLogFileFstream)<<"789"<<456<<endl;
    if(pVFOpLogFileFstream->fail()){
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
