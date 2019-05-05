#include <pullfile_server.hpp>

bool PullFileServer::listen(int port)
{
    //todo deamon
    logger.log(L0, "push file server listening at port: %d", port);
    if (!start(port)) 
    {
        return false;
    }
    return true;
}

bool PullFileServer::sessionPut(const string &key, const string &name, const string &path, const string &pubkey)
{
    if(isSessionExist(key))
    {
        logger.debug("session existed!");
        return false;
    }
    sessions.emplace(piecewise_construct, forward_as_tuple(key), forward_as_tuple(name, path, pubkey));
    return true;
}

bool PullFileServer::isSessionExist(const string &key)
{
    if(sessions.count(key))
        return true;
    return false;
}


bool PullFileServer::handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse)
{
    if (recvLen <= 0)
    {
        logger.log("error: len = 0");
        //todo response
        return false;
    }

    Msg::Message recvMsg;
    Msg::Message resMsg;
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);

    switch (recvMsg.type())
    {
    case Msg::GetFile_Request :
        getFileResponse(recvMsg, resMsg);
        break;
    default:
        break;
    }

    resMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    sendLen = resMsg.ByteSize();
    return true;
}

bool PullFileServer::getFileResponse(Msg::Message &recvMsg, Msg::Message &resMsg)
{
    string path = recvMsg.request().get_file_req().path();
    string name = recvMsg.request().get_file_req().name();
    string auth = recvMsg.request().get_file_req().auth();

    //todo auth
    string pubkey = "";
    string sessionKey = getRandomKey();
    string err;
    if(!sessionPut(sessionKey, name, path, pubkey))
    {
        GetFileMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, 0, "");
        return false;
    }

    struct stat statBuf;

    if(stat(path.c_str(), &statBuf) == -1)
    {
        err = "path: " + path + " is not exist";
        logger.log(err);
        GetFileMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, 0, "");
        return false;
    }

    if(!S_ISREG(statBuf.st_mode))
    {
        err = "file: " + path + " is not regular file";
        logger.log(err);
        GetFileMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, 0, "");
        return false;
    }

    GetFileMsgResInst(resMsg, Msg::MSG_RES_OK, "ok", statBuf.st_size, sessionKey);
    return true;
}

bool PullFileServer::getFileChunkResponse(Msg::Message &recvMsg, Msg::Message &resMsg)
{
    string path = recvMsg.request().get_file_chunk_req().path();
    int fileIdx = recvMsg.request().get_file_chunk_req().fileidx();
    string sessionKey = recvMsg.request().get_file_chunk_req().session_key();
    char databuf[TRANS_CHUNK_SIZE];

    if(!isSessionExist(sessionKey))
    {
        GetFileChunkMsgResInst(resMsg, Msg::MSG_RES_ERROR, "session is not exists", databuf, 0);
        return false;
    }

    string disakPath = pVvfs->getVFPhasicalPath(path);
    ifstream ifs(disakPath);
    if(ifs.fail())
    {
        logger.log(strerror(errno));
        return false;
    }

    ifs.read(databuf, TRANS_CHUNK_SIZE);

    

    ifs.close();   
}