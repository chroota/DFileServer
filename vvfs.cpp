#include "vvfs.hpp"

bool Vvfs::initConfig(){
    vFRConfigFile = "./vfr.txt";
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
    string vfHash;
    for(int i=0; i < vFiles.size(); i++)
    {
        vfHash = vFiles[i].getHash();
        mpkg.update(vfHash.c_str(), vfHash.size());
    }
    hash = vfHash;
    return true;
}

bool Vvfs::buildVFS()
{
    FILE *fp = fopen(vFRConfigFile.c_str(), "r");
    if(fp == NULL){
        logger.log("vfr config file not exists!!", L4);
        return false;
    }

    char name[MAXBUFSIZE], path[MAXBUFSIZE];
    int idx, prev_bro_idx, next_bro_idx, fa_idx, first_son_idx, last_son_idx, type, count;
    long tv_sec, tv_nsec, size;

    char hashBuf[17];
    
    fscanf(fp, "%d", &count);
    vFiles.resize(count);
    int i = 0;
    logger.log("read file realations from " + vFRConfigFile + ", file num:" + to_string(count));

    // todo exception deal
    while(!feof(fp) && i++ < count)
    {

        fscanf(fp, 
            "%d %s %s %d %d %d %d %d %d %ld %ld %ld %s", 
            &idx, name, path, &fa_idx, &prev_bro_idx, &next_bro_idx, 
            &first_son_idx, &last_son_idx,&type, &tv_sec, &tv_nsec, 
            &size, hashBuf
        );

        // printf("%d %s %s %d %d %d %d %d %d %ld %ld %ld %s\n", idx, name, path, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx, type, tv_sec, tv_nsec, size, hashBuf);

        if(idx >= count){
            logger.log("invalid file idx for file" + string(name) + " " + to_string(idx));
            continue;
        }

        VFRelation vfr(idx, fa_idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
        string fullPath = string(path) + "/" + name;
        if (vfrs.count(fullPath) != 0) {
            logger.log("file path repeat!");
        }
        
        vfrs[fullPath] = vfr;
        timespec tspc = {tv_sec, tv_nsec};
        VFile vf(name, (FileType)type, path, size, tspc, fa_idx, idx, prev_bro_idx, next_bro_idx, first_son_idx, last_son_idx);
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
    if (name.size() == 0 || name == " ") {
        err = "file name error!";
        return false;
    }
    if (vfrs.count(name) != 0) {
        err = "file:"+name+" exists!";
        return false;
    }

    int spIdx = name.find_last_of("/");
    string path = "/";

    if (spIdx != -1) path = name.substr(0, spIdx + 1);

    int newIdx = allocIdx();
    VFile vf(name, type, path, 0, getTimeSpec(), -1,  -1, -1, -1, -1, -1);
    VFRelation newVfr(newIdx, -1, -1, -1, -1, -1);  
    vfrs[name] = newVfr;
    if (vfrs.count(path) == 0) 
    {
        err = "path not exist!";
        return false;
    }

    VFile dirVf = vFiles[vfrs[path].idx];
    VFRelation dirVfr = vfrs[name];
    if (dirVfr.last_son_idx != -1) {
        vfrs[vFiles[dirVfr.last_son_idx].getName()].next_bro_idx = newIdx;
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
    if (vfrs.count(name) == 0) {
        err = "file:"+name+"not exists!";
        return false;
    }

    int spIdx = name.find_last_of("/");
    string path = "/";

    if (spIdx != -1) path = name.substr(0, spIdx + 1);

    VFRelation vfr = vfrs[name];
    VFile vf = vFiles[vfr.idx];
    VFile vfDir = vFiles[vfr.fa_idx];
    VFRelation vfrDir = vfrs[vfDir.getName()];

    if(remove((vfsPath+name).c_str()) == -1){
        // logger.log(string("fail to remove :")+name);
        err = string("fail to remove :")+name;
        return false;
    }

    //first son
    if(vfr.idx == vfrDir.first_son_idx){
        VFRelation *pNextVfr = &(vfrs[vFiles[vfr.next_bro_idx].getName()]);
        pNextVfr->prev_bro_idx = -1;
        vfrDir.first_son_idx = pNextVfr->idx;
    }
    // last son
    else if(vfr.idx == vfrDir.last_son_idx){
        VFRelation *pPrevVfr = &(vfrs[vFiles[vfr.prev_bro_idx].getName()]);
        pPrevVfr->next_bro_idx = -1;
        vfrDir.last_son_idx = pPrevVfr->idx;
    }else{
        VFRelation *pNextVfr = &(vfrs[vFiles[vfr.next_bro_idx].getName()]);
        VFRelation *pPrevVfr = &(vfrs[vFiles[vfr.prev_bro_idx].getName()]);
        pPrevVfr->next_bro_idx = pNextVfr->idx;
        pPrevVfr->prev_bro_idx = pNextVfr->next_bro_idx;
    }

    vf.destory();
    vfrs.erase(name);
    vf.decSize();
    err = "";
    return true;
}


bool Vvfs::lsVF(const string &name, string & fileList, string &err){
    //todo
    return true;
}

//reverse todo
bool Vvfs::watchVFs(){
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



// reverse
bool Vvfs::buildVFSByScanDir(){
    DIR *dir;
    dirent *dptr;
    
    // queue<string> qDirPaths;
    struct stat statbuf;
    int num = 0;

    // if(!buildVFR()){
    //     logger.log("fail to build vfs relation!");
    //     return false;
    // }

    if (vfsPath.size() == 0) return false;

    // qDirPaths.push(vfsPath);

    if((dir = opendir(vfsPath.c_str())) == NULL)
    {
        logger.log("can't open dir:"+vfsPath);
        return false;
    }

    while((dptr = readdir(dir)) != NULL)
    {
        string fullPath = vfsPath+'/'+dptr->d_name;
        stat(fullPath.c_str(), &statbuf);           
        if(strcmp(dptr->d_name,".")==0 || strcmp(dptr->d_name,"..")==0)
            continue;

        if(!vfrs.count(dptr->d_name))
        {
            logger.log("there are not relation for:" + string(dptr->d_name));
            continue;
        }

        if(dptr->d_type == DT_REG || dptr->d_type == DT_DIR)
        {
            VFRelation vfr = vfrs[dptr->d_name];
            // VFile vf = VFile(
            //     dptr->d_name,
            //     VFT_FILE, dptr->d_name, 
            //     statbuf.st_size, 
            //     statbuf.st_mtim,
            //     vfr.fa_idx,vfr.idx,vfr.next_bro_idx,vfr.first_son_idx
            // );
            // vFiles.push_back(vf);
        }
        num++;
    }
    return true;
}


bool Vvfs::buildVFsReserve()
{
    DIR *dir;
    dirent *dptr;
    queue<string> qDirPaths;
    struct stat statbuf;
    int num = 0;

    if (vfsPath.size() == 0) return false;

    // fileNameIdxMap[vfsPath] = -1;
    qDirPaths.push(vfsPath);

    // while(qDirPaths.size() > 0)
    // {
    //     string dirPath = qDirPaths.front();
    //     if((dir = opendir(dirPath.c_str())) == NULL)
    //     {
    //         logger.log("can't open dir:"+dirPath);
    //     }

    //     int childrenNum = 0;
    //     int dirIdx = fileNameIdxMap[dirPath];

    //     while((dptr = readdir(dir)) != NULL)
    //     {
    //         // cout<<dptr->d_name<<endl;
    //         string fullPath = dirPath+'/'+dptr->d_name;
    //         stat(fullPath.c_str(), &statbuf);           

    //         if(strcmp(dptr->d_name,".")==0 || strcmp(dptr->d_name,"..")==0) 
    //             continue;

    //         // fle or dir
    //         if(dptr->d_type == DT_REG || dptr->d_type == DT_DIR)
    //         {
    //             cout<<fullPath<<endl;
    //             //todo find exception deal
    //             string mapPath = fullPath.substr(fullPath.find_first_of('/')+1);
    //             VFile vf = VFile(
    //                 dptr->d_name, 
    //                 FT_FILE, mapPath, statbuf.st_size, 
    //                 statbuf.st_mtim,
    //                 fileNameIdxMap[dirPath],num
    //             );

    //             if (num > 1) {
    //                 vFiles[num-1].next_bro_idx = num;
    //             }
    //             fileNameIdxMap[fullPath] = num;

    //             if(dptr->d_type == DT_DIR){
    //                 vf.setType(FT_DIR);
    //                 qDirPaths.push(dirPath+'/'+dptr->d_name);
    //             }
    //             vFiles.push_back(vf);
    //             num++;
    //             childrenNum++;

    //             if(dirIdx != -1 && childrenNum == 1)
    //             {
    //                 vFiles[dirIdx].first_son_idx = vf.idx;
    //             }
    //         }
    //     }
    //     vFiles[dirIdx].setChildrenNum(childrenNum);
    //     qDirPaths.pop();
    // }

    initHash();
    return true;
}


