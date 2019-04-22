#include "VvfsTp.hpp"

void VvfsTp::run(int argc, char *argv[])
{
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
        if(argc < 4) help();
    }else if(!strcmp(argv[1], LS_FILE))
    {
        if(argc < 3) help();
        lsVF(argv[2]);
    }else{
        help();
    }
}

bool VvfsTp::mkDir(const string & remotePath)
{
    return newVF("", remotePath, Msg::FT_DIR);
}


bool VvfsTp::newVF(const string & localPath, const string & remotePath, Msg::FileType type)
{
    if(type == Msg::FT_FILE)
    {
        logger.debug("new file: local path:"+ localPath +", remote path:" + remotePath);
    }else
    {
        logger.debug("mkdir:"+remotePath);
    }
    string err;
    struct stat statBuf;
    int totalPackSize, totalBufSize;
    int lastBufSize;
    if(type == Msg::FT_FILE)
    {
        if(stat(localPath.c_str(), &statBuf) == -1)
        {
            logger.log("local path error");
            return false;
        }
        totalBufSize = statBuf.st_size;
        totalPackSize = statBuf.st_size / TRANS_CHUNK_SIZE;
        lastBufSize = statBuf.st_size % TRANS_CHUNK_SIZE;
        if(lastBufSize != 0) totalPackSize++;
    }else
    {
        totalPackSize = 1;
        totalBufSize = 0;
    }
    

    // create new file message
    

    Msg::Message sendMsg, recvMsg;
    NewFileMsgReqInst(sendMsg, remotePath, type, totalPackSize, totalBufSize);
    char sendbuf[MAXBUFSIZE], recvBuf[MAXBUFSIZE];
    int recvLen;
    sendMsg.SerializePartialToArray(sendbuf, MAXBUFSIZE);
    if(!urequest(host.c_str(), port, sendbuf, strlen(sendbuf), recvBuf, recvLen))
    {
        logger.log("new file send fail");
        return false;
    }
    recvMsg.ParseFromArray(recvBuf, recvLen);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        err = recvMsg.response().info();
        logger.log(err);
        return false;
    }

    if(type == Msg::FT_DIR) return true;

    //logger.log("create remote file success: "+ remotePath + " postSessionId:");
    int postSessionId = recvMsg.response().new_file_response().post_session_id();
    // cout<<postSessionId<<endl;

    ifstream ifs(localPath);
    if(ifs.fail())
    {
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
    while(!ifs.eof())
    {
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
        cout<<"send data error, errorno:"<<errno<<" info:%s"<<strerror(errno);
        return false;
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        cout << "rm fail, "<<recvMsg.response().info()<<endl;
        return true;
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
        cout<<"send data error, errorno:"<<errno<<" info:"<<strerror(errno)<<endl;;
        return false;
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        cout<<"error, info:"<<recvMsg.response().info().c_str()<<endl;
        return true;
    }

    const Msg::LsFileResponse & lsFileResponse = recvMsg.response().ls_file_res();

    cout<<"total:"<<lsFileResponse.files_size()<<endl;
    for (int i = 0; i < lsFileResponse.files_size(); i++)
    {
        const Msg::FileAttribute & file = lsFileResponse.files(i);
        cout<<file.time()<<" ";
        cout.setf(ios::right);
        cout.width(4);
        if(file.type() == Msg::FT_DIR)
        {
            cout<<"dir"<<" ";
        }else
        {
            cout<<"file"<<" ";
        }
        cout.setf(ios::right);
        cout.width(10);
        cout<<file.size()<<" ";
        cout<<file.name();
        if(i == 0){
            cout<< "(.)";
        }
        cout<<endl;
    }
    
    return true;
}

bool VvfsTp::getVF(const string & remotePath, const string & localPath)
{
    return true;
}

bool VvfsTp::mvVF(const string & remoteSrcPath, const string & remoteDstPath)
{
    logger.debugAction("mv: src:"+remoteSrcPath+" dst:"+remoteDstPath);

    char sendbuf[MAXBUFSIZE], recvbuf[MAXBUFSIZE];
    int recvLen;
    Msg::Message sendMsg, recvMsg;
    MvFileMsgReqInst(sendMsg, remoteSrcPath, remoteDstPath);
    sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);

    if(!urequest(host.c_str(), port, sendbuf, MAXBUFSIZE, recvbuf, recvLen)){
        cout<<"send data error, errorno:"<<errno<<" info:%s"<<strerror(errno);
        return false;
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        cout << "move fail, "<<recvMsg.response().info()<<endl;
        return true;
    }

    cout<<"success move "<<remoteSrcPath<<" to "<<remoteDstPath<<endl;
    return true;   
}

bool VvfsTp::cpVF(const string & remoteSrcPath, const string & remoteDstPath)
{
    logger.debugAction("cp: src:"+remoteSrcPath+" dst:"+remoteDstPath);

    char sendbuf[MAXBUFSIZE], recvbuf[MAXBUFSIZE];
    int recvLen;
    Msg::Message sendMsg, recvMsg;
    CpFileMsgReqInst(sendMsg, remoteSrcPath, remoteDstPath);
    sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);

    if(!urequest(host.c_str(), port, sendbuf, MAXBUFSIZE, recvbuf, recvLen)){
        cout<<"send data error, errorno:"<<errno<<" info:%s"<<strerror(errno);
        return false;
    }

    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        cout << "move fail, "<<recvMsg.response().info()<<endl;
        return true;
    }

    cout<<"success move "<<remoteSrcPath<<" to "<<remoteDstPath<<endl;
    return true;   
}

bool VvfsTp::init()
{
    host = "0.0.0.0";
    port = 8080;
    return true;
}


#define VVFS_DEBUG
int main(int argc, char *argv[])
{
    VvfsTp tp;
    tp.init();
    #ifdef VVFS_DEBUG
        assert(tp.newVF("./test_dir/local/md5.cpp", "/md5.cpp") == true);
        // assert(tp.newVF("./node_sync", "/node_sync") == true);
        assert(tp.mkDir("/test") == true);
        assert(tp.lsVF("/") == true);
        assert(tp.mvVF("/md5.cpp", "/test/md5.cpp") == true);
        // assert(tp.mkDir("/test/test123") == true);
        // assert(tp.mkDir("/test/test234") == true);
        // assert(tp.mkDir("/test/test123/test456") == true);
        // assert(tp.newVF("./msg.pb.cc", "/msg.pb.cc") == true);
        assert(tp.lsVF("/") == true);
        // assert(tp.lsVF("/md5.cpp") == true);
        // assert(tp.rmVF("/md5.cpp") == true);
        // assert(tp.rmVF("/md5.cpp") == true);
        // assert(tp.lsVF("/") == true);
        assert(tp.lsVF("/test") == true);
        // assert(tp.lsVF("/test/test123") == true);
        // assert(tp.rmVF("/test") == true);
        // assert(tp.lsVF("/") == true);
        // assert(tp.lsVF("/test/test123/test456") == true);
        // assert(tp.lsVF("/msg.pb.c") == true);
    #else
        tp.run(argc, argv);
    #endif
}


