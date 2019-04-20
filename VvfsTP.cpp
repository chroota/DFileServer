#include "VvfsTp.hpp"

void VvfsTp::run(int argc, char *argv[]){
    if (argc < 2) help();
    if (!strcmp(argv[1], NEW_FILE)) 
    {
        // cout<<NEW_FILE<<endl;
        if (argc < 4) help();
        newVF(argv[2], argv[3]);
    }else if(!strcmp(argv[1], RM_FILE))
    {
        // cout<<RM_FILE<<endl;
        if(argc < 3) help();
        rmVF(argv[2]);
    }else if(!strcmp(argv[1], MV_FILE))
    {
        // cout<<MV_FILE<<endl;
        if(argc < 4) help();
    }else if(!strcmp(argv[1], GET_FILE))
    {
        // cout<<GET_FILE<<endl;
        if(argc < 4) help();
    }else if(!strcmp(argv[1], LS_FILE))
    {
        if(argc < 3) help();
        // cout<<LS_FILE<<endl;
    }else{
        help();
    }
}


bool VvfsTp::newVF(const string & localPath, const string & remotePath){
    logger.debug("new file: local path:"+ localPath +", remote path:" + remotePath);
    string err;
    struct stat statBuf;
    if(stat(localPath.c_str(), &statBuf) == -1){
        logger.log("local path error");
        return false;
    }

    // create new file message
    int totalPackSize = statBuf.st_size / TRANS_CHUNK_SIZE;
    int lastBufSize = statBuf.st_size % TRANS_CHUNK_SIZE;
    if(lastBufSize !=0) totalPackSize++;

    Msg::Message sendMsg, recvMsg;
    NewFileMsgReqInst(sendMsg, remotePath, Msg::FT_FILE, totalPackSize, statBuf.st_size);
    char sendbuf[MAXBUFSIZE], recvBuf[MAXBUFSIZE];
    int recvLen;
    sendMsg.SerializePartialToArray(sendbuf, MAXBUFSIZE);
    if(!urequest(host.c_str(), port, sendbuf, strlen(sendbuf), recvBuf, recvLen)){
        logger.log("new file send fail");
        return false;
    }
    recvMsg.ParseFromArray(recvBuf, recvLen);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR){
        err = recvMsg.response().info();
        logger.log(err);
        return false;
    }

    //logger.log("create remote file success: "+ remotePath + " postSessionId:");
    int postSessionId = recvMsg.response().new_file_response().post_session_id();
    // cout<<postSessionId<<endl;

    ifstream ifs(localPath);
    if(ifs.fail()){
        logger.log(strerror(errno));
        return false;
    }

    sendMsg.Clear();
    char databuf[TRANS_CHUNK_SIZE];
    int packIdx = 0, fileIdx = 0;

    memset(sendbuf, 0, sizeof(sendbuf));
    //vector<int> packFileIdxMap(totalPackSize);

    // send file data
    // todo design of high performance
    int readedSize;
    while(!ifs.eof()){
        memset(databuf, 0, TRANS_CHUNK_SIZE);
        fileIdx = ifs.tellg();
        try
        {
            ifs.read(databuf, TRANS_CHUNK_SIZE);
        }
        catch(const std::exception& e)
        {
            // std::cerr << e.what() << '\n';
            logger.fatal(e.what());
        }

        // packFileIdxMap
        //packFileIdxMap[packIdx] = fileIdx;

        if(ifs.eof()) readedSize = lastBufSize;
        else readedSize = TRANS_CHUNK_SIZE;
        

        FileChunkPostMsgInst(sendMsg, remotePath, databuf, fileIdx, packIdx, readedSize, postSessionId);
        if(strlen(sendbuf) >= MAXBUFSIZE)
        {
            logger.debug("size of sendbuf larger than MAXBUFSIZE!");
            return false;
        }
        sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);

        int sendCount = 0;
        if(!urequest(host.c_str(), port, sendbuf, MAXBUFSIZE, recvBuf, recvLen))
        {
            logger.log(L0, "send data error, errorno:%d info:%s", errno, strerror(errno));
        }

        recvMsg.ParseFromArray(recvBuf, MAXBUFSIZE);

        if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
        {
            logger.log(L4, "error send at packIdx:%d fileIdx:%d", packIdx, fileIdx);
        }
        logger.log(L0,"success at packIdx:%d fileIdx:%d", packIdx, fileIdx);
        packIdx++;
        recvMsg.Clear();
    }

    return true;
}

bool VvfsTp::rmVF(const string & remotePath)
{
    logger.debugAction("rm: "+remotePath);

    char sendbuf[MAXBUFSIZE], recvbuf[MAXBUFSIZE];
    int recvLen;
    Msg::Message sendMsg, recvMsg;
    RMFileMsgReqInst(sendMsg, remotePath);
    sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);

    if(!urequest(host.c_str(), port, sendbuf, MAXBUFSIZE, recvbuf, recvLen)){
        logger.log(L4, "send data error, errorno:%d info:%s", errno, strerror(errno));
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        logger.log(L1, "error, info: %s", remotePath.c_str(), recvMsg.response().info().c_str());
        return false;
    }

    logger.log("success remove file:"+remotePath);
    return true;
}

bool VvfsTp::lsVF(const string & remotePath)
{
    logger.debugAction("ls: "+remotePath);
    char sendbuf[MAXBUFSIZE], recvbuf[MAXBUFSIZE];
    int recvLen;
    Msg::Message sendMsg, recvMsg;
    LsFileMsgReqInst(sendMsg, remotePath);
    sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    if(!urequest(host.c_str(), port, sendbuf, MAXBUFSIZE, recvbuf, recvLen))
    {
        logger.log(L4, "send data error, errorno:%d info:%s", errno, strerror(errno));
        return false;
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        logger.log(L1, "error, info: %s", remotePath.c_str(), recvMsg.response().info().c_str());
        return false;
    }

    const Msg::LsFileResponse & lsFileResponse = recvMsg.response().ls_file_res();

    for (int i = 0; i < lsFileResponse.files_size(); i++)
    {
        const Msg::FileAttribute & file = lsFileResponse.files(i);
        cout<<file.time()<<" ";
        cout.setf(ios::right);
        cout.width(4);
        if(file.type() == Msg::FT_DIR){
            cout<<"dir"<<" ";
        }else{
            cout<<"file"<<" ";
        }
        cout.setf(ios::right);
        cout.width(10);
        cout<<file.size()<<" ";
        cout<<file.name()<<endl;
    }
    
    return true;
}

bool VvfsTp::getVF(const string & remotePath, const string & localPath){
    return true;
}

bool VvfsTp::init(){
    host = "0.0.0.0";
    port = 8080;
    return true;
}


#define VVFS_DEBUG
int main(int argc, char *argv[]){
    VvfsTp tp;
    tp.init();

    #ifdef VVFS_DEBUG
        assert(tp.newVF("./test_dir/local/md5.cpp", "/md5.cpp") == true);
        // assert(tp.newVF("./node_sync", "/node_sync") == true);
        assert(tp.newVF("./msg.pb.cc", "/msg.pb.cc") == true);
        //assert(tp.rmVF("/md5.cpp") == true);
        assert(tp.lsVF("/") == true);
    #else
        tp.run(argc, argv);
    #endif
}


