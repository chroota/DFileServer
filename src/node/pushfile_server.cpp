#include <pushfile_server.hpp>

bool FileSaver::open(string &err)
{
    if(type == Msg::FT_FILE)
    {
        pOutput = new ofstream(savePath, ios::trunc);
    }
    if(pOutput->fail())
    {
        err = "can't create file:" + savePath;
        logger.log(err);
        return false;
    }
    //todo other type exception
    pRecvFlag = new vector<int>(totalPackSize, 0);
    logger.log(LDEBUG, "success create vf name:%s precvFlagsize:%d", savePath.c_str(), pRecvFlag->size());
    return true;
}

bool FileSaver::writeChunk(const string &data, int fileIdx, int packIdx, string &err)
{
    logger.debugAction("writeChunk");
    if(isComplete())
    {
        err = "file already complete";
        logger.log(err);
        return false;
    }

    if(pRecvFlag->at(packIdx))
    {
        err = "file chunk already writed at packIdx:" + to_string(packIdx);
        return false;
    }

    //todo exception
    pOutput->seekp(fileIdx);
    pOutput->write(data.c_str(), data.size());
    if(pOutput->fail())
    {
        err = "can't write file chunk";
        logger.log(err);
        return false;
    }

    recvChunkCount++;
    recvFileSizeCount += data.size();
    pRecvFlag->at(packIdx) = 1;
    return true;
}

bool FileSaver::isComplete()
{
    // todo verify file hash
    // cout<<recvChunkCount<<"/"<<totalPackSize<<endl;
    // if(recvChunkCount == totalPackSize)
    if(recvChunkCount == totalPackSize && recvFileSizeCount == totalFileSize)
    {
        pOutput->close();
        return true;
    }
    return false;
}

bool PushFileServer::handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse)
{
    if (strlen(recvbuf) == 0 || recvLen <= 0) 
    {
        logger.log("error: len = 0");
        //todo response
        return false;
    }

    Msg::Message recvMsg;
    Msg::Message sendMsg;
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if(recvMsg.type() == Msg::NewFile_Request)
    {
        logger.debugAction("create file");
        return createNewFile
        (
            recvMsg.request().file().name(),recvMsg.request().file().type(),
            recvMsg.request().file().total_file_size(),recvMsg.request().file().total_pack_size(),
            sendbuf, sendLen
        );
    }
    else if(recvMsg.type() == Msg::File_Post)
    {
        logger.debugAction("file post");
        recvChunk(
            recvMsg.file_post().name(),recvMsg.file_post().post_session_id(),
            recvMsg.file_post().file_idx(),recvMsg.file_post().pack_idx(),
            recvMsg.file_post().data(),sendbuf,sendLen
        );

    }else if(recvMsg.type() == Msg::Rm_File_Request)
    {
        string rmPath = recvMsg.request().rm_op().path();
        logger.debugAction("rmFile:" + rmPath);
        rmFile(rmPath, sendbuf, sendLen);
    }else if(recvMsg.type() == Msg::LsFile_Request)
    {
        string lsPath = recvMsg.request().ls_file_req().path();
        logger.debugAction("ls file:" + lsPath);
        lsFiles(lsPath, sendbuf, sendLen);
    }
    else if(recvMsg.type() == Msg::MvFile_Request)
    {
        mvFile(recvMsg.request().mv_file_req().srcpath(), recvMsg.request().mv_file_req().dstpath(), sendbuf, sendLen);
    }
    else if(recvMsg.type() == Msg::CpFile_Request)
    {
        cpFile(recvMsg.request().cp_file_req().srcpath(), recvMsg.request().cp_file_req().dstpath(), sendbuf, sendLen);
    }
    else
    {
        logger.log("msg type not found for udp file server!");
    }

    return false;
}

bool PushFileServer::cpFile(const string &srcPath, const string & dstPath, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;
    cout<<"cp file src:"<<srcPath<<" dst:"<<dstPath<<endl;
    if(srcPath[0] != '/' || dstPath[0] != '/')
    {
        err = "error name path";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }
    if(!pVvfs->cpVF(srcPath, dstPath, err))
    {
        CommonMsgResInst(msg, Msg::MSG_RES_ERROR, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }
    CommonMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = msg.ByteSize();
    return true;
}

bool PushFileServer::mvFile(const string &srcPath, const string & dstPath, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;
    cout<<"move file src:"<<srcPath<<" dst:"<<dstPath<<endl;

    if(srcPath[0] != '/' || dstPath[0] != '/')
    {
        err = "error name path";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        // sendLen = msg.ByteSize();
        return false;
    }
    if(!pVvfs->mvVF(srcPath, dstPath, err))
    {
        CommonMsgResInst(msg, Msg::MSG_RES_ERROR, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        // sendLen = msg.ByteSize();
        return false;
    }
    CommonMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = msg.ByteSize();
    // sendLen = msg.ByteSize();
    return true;    
}

bool PushFileServer::createNewFile(const string &path, Msg::FileType type, int totalFileSize, 
    int totalPackSize, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;

    if(path[0] != '/')
    {
        err = "error name path";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    if(type == Msg::FT_DIR)
    {
        if(!pVvfs->mkVDir(path, err))
        {
            logger.log(err);
            NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
            msg.SerializeToArray(sendbuf, MAXBUFSIZE);
            sendLen = msg.ByteSize();
            return false;
        }
        NewFileMsgResInst(msg, Msg::MSG_RES_OK, -1, "ok");
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return true;
    }
    

    string diskPath;
    if(!pVvfs->newVF(path, totalFileSize, err, diskPath))
    {
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    string savePath = pVvfs->getVFPhasicalPath(diskPath);
    logger.log("save path:" + savePath);

    if(savers.size() >= MAX_OPEN_FILES)
    {
        err = "exceeding max open file nums";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    // create file saver
    FileSaver *pFileSaver = new FileSaver(savePath, type, totalFileSize, totalPackSize);

    if(!pFileSaver->open(err))
    {
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    savers[path] = pFileSaver;
    logger.debug("saver len:" + to_string(savers.size()));

    // allocate session post id
    int sessionId = getSessionId();
    logger.log(LDEBUG, "allocate session id:%d", sessionId);
    sessions[path] = sessionId;
    NewFileMsgResInst(msg, Msg::MSG_RES_OK, sessionId, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = msg.ByteSize();
    return true;
}

bool PushFileServer::recvChunk(const string &name, int sessionId, int fileIdx, int packIdx, const string & data, char sendbuf[], int &sendLen)
{
    logger.debugAction("recvChunk");
    logger.debug("post file name: "+name);
    string err;
    Msg::Message msg;
    if(savers.count(name) == 0)
    {
        err = "file saver not exists";
        logger.debug(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    cout<<"recv "<<"fileIdx:"<<fileIdx<<" "<<"packIdx:"<<packIdx<<endl;

    if(!savers[name]->writeChunk(data, fileIdx, packIdx, err))
    {
        logger.debug(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }

    if(savers[name]->isComplete())
    {
        logger.debug("complete");
        pVvfs->activeVF(name, err);
        savers.erase(name);
    }

    NewFileMsgResInst(msg, Msg::MSG_RES_OK, -1, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = msg.ByteSize();
    
    return true;
}

bool PushFileServer::listen(int port)
{
    //todo deamon
    logger.log(L0, "push file server listening at port: %d", port);
    if (!start(port)) 
    {
        return false;
    }
    return true;
}

bool PushFileServer::test()
{
    // pVvfs->vfrsTest();
    return false;
}


bool PushFileServer::rmFile(const string &path, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;
    if(!pVvfs->rmVF(path, err))
    {
        CommonMsgResInst(msg, Msg::MSG_RES_ERROR, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }
    CommonMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = msg.ByteSize();
    return true;
}

bool PushFileServer::lsFiles(const string &path, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;
    if(path[0] != '/')
    {
        err = "error name path";
        logger.log(err);
        //NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = msg.ByteSize();
        return false;
    }
    bool ret = pVvfs->lsVF(path, msg);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    // sendLen = msg.ByteSize();
    sendLen = MAXBUFSIZE;
    return ret;
}