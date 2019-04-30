#include "node.hpp"

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
        // sendLen = msg.ByteSize();
        return false;
    }
    if(!pVvfs->cpVF(srcPath, dstPath, err))
    {
        CommonMsgResInst(msg, Msg::MSG_RES_ERROR, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        // sendLen = msg.ByteSize();
        sendLen = msg.ByteSize();
        return false;
    }
    CommonMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    // sendLen = msg.ByteSize();
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

bool Node::createVFS()
{
    pVvfs = new Vvfs();

    if(!pVvfs->initConfig(syncDir))
    {
        logger.fatal("Vvfs cofnig init fail");
    }

    if(!pVvfs->buildVFS())
    {
        logger.fatal("Vvfs build error");
    }

    // if(!pVvfs->deamon()){
    //     logger.fatal("Vvfs demaon started failed");
    // }

    return true;
}

bool Node::createFileServer()
{
    pPushFileServer = new PushFileServer(pVvfs);
    return true;
}


void Node::checkFileSync()
{
    
}

void Node::syncFileFn()
{
    logger.log("check file sync");
    while(true)
    {
        logger.log("check file sync");
        this_thread::sleep_for(chrono::seconds(checkFileSyncTime));
    }
}

void Node::serverFn()
{
    int i = 0;
    while(i++ < 100)
    {
        std::cout<<"server fn"<<std::endl;
        sleep(1);
    }
}

void Node::keepAliveFn(){
    logger.log("test");
    while(true)
    {
        // std::cout<<"request fn"<<std::endl;
        updateStatus(NODE_STATUS_ALIVE);
        this_thread::sleep_for(chrono::seconds(aliveSyncTime));
    }
}

void Node::forever(int argc, char *argv[])
{
    if(!initConfig(argc, argv)) 
    {
        logger.log("config error");
        exit(-1);
    }

    if(!createVFS())
    {
        logger.log("build VFS failed");
        exit(-1);
    }

    if(!createFileServer())
    {
        logger.log("build VFS server failed");
        exit(-1);
    }
    if (!join()) 
    {
        logger.log("join error");
        exit(-1);
    }


    /*
     * for test
    */
    if(!updateState(pVvfs->getHash()))
    {
        logger.log("update hash error");
        exit(-1);
    }

    pPushFileServer->listen(pushFileServerPort);
}


bool Node::getStateNode()
{
    char recvbuf[MAXBUFSIZE];
    int recvLen;
    char sendbuf[MAXBUFSIZE];

    Msg::Message msg = GetStateNodeMsgReqInst(name.c_str());
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, strlen(sendbuf), recvbuf, recvLen);
    Msg::Message recvMsg;
    // printf("%d\n", strlen(recvbuf));
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
    {
        logger.log(recvMsg.response().info());
        return false;
    }
    return true;
}

// update hash
bool Node::updateState(const string &hash)
{
    logger.debug("update state");
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    string auth = rsaEncryptByPrivkey(name, privkey);
    Msg::Message sendMsg = UpdateStateMsgReqInst(name, hash, auth);

    sendMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, sendMsg.ByteSize(), recvbuf, recvLen);
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        logger.log(recvMsg.response().info());
        return false;
    }
    logger.log(recvMsg.response().info());
    return true;
}

// update status
bool Node::updateStatus(NODE_STATUS status)
{
    char recvbuf[MAXBUFSIZE];
    int recvLen;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = UpdateStatusMsgReqInst(name.c_str(), status);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, strlen(sendbuf), recvbuf, recvLen);
    Msg::Message recvMsg;
    // printf("%d\n", strlen(recvbuf));
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
    {
        logger.log(recvMsg.response().info());
        return false;
    }
    logger.log(recvMsg.response().info());
    return true;
}

bool Node::join()
{
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    string auth = rsaEncryptByPrivkey(name, privkey);
    Msg::Message msg = JoinMsgReqInst(name, nodeIp, to_string(nodePort), auth);
    msg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error recvMsg
    urequest(masterIp, masterPort, sendbuf, msg.ByteSize(), recvbuf, recvLen);
    
    Msg::Message recvMsg;
    if(!recvMsg.ParseFromArray(recvbuf, recvLen))
    {
        logger.fatal("fail to join, parse msg error");
        return false;
    }
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
    {
        logger.log(recvMsg.response().info());
        return false;
    }
    string encryptedEncryptKey = recvMsg.response().join_res().encrypedencryptkey();
    encryptKey = rsaDecryptByPrivkey(encryptedEncryptKey, privkey);
    
    logger.debug("encrypt key:"+encryptKey);
    // logger.log(recvMsg.response().info());
    logger.log("success join");
    return true;
}

bool Node::initConfig(int argc, char *argv[])
{
    //todo parse config
    nodeIp = "0.0.0.0";
    nodePort = 8888;
    pushFileServerPort = 8080;
    pullFileServerPort = 8081;
    masterIp = "127.0.0.1";
    masterPort = 8888;
    aliveSyncTime = ALIVE_SYNC_TIME;
    name = "node1";
    syncDir = "./test_dir/remote";
    // catchSegmentFaultError();
    keypairDir = "./test_dir/nodes_dir/" + name;

    int ch;
    opterr = 0;
    while ((ch = getopt(argc, argv,"i:p:n:d:h:q:l:s:k:"))!= -1)
    {
        switch(ch)
        {
            case 'i': nodeIp = optarg;  break;
            case 'p': nodePort = atoi(optarg); break;
            case 'n': name = optarg; keypairDir = "./test_dir/nodes_dir/" + name; break;
            case 'd': syncDir = optarg;  break;
            case 'h': masterIp = optarg;  break;
            case 'q': masterPort = atoi(optarg);  break;
            case 'l': pullFileServerPort = atoi(optarg);  break;
            case 's': pushFileServerPort = atoi(optarg);  break;
            case 'k': keypairDir = optarg;  break;
            default: printf("unkown option :%c\n",ch); exit(EXIT_FAILURE);
        }
    }
    cout << "args:"                << endl
         << "node ip:"             << nodeIp << endl
         << "master ip:"           << masterIp << endl
         << "node port:"           << nodePort << endl
         << "master port:"         << masterPort << endl
         << "pullFileServer port:" << pullFileServerPort << endl
         << "pushFileServer port:" << pushFileServerPort << endl
         << "name:"                << name << endl
         << "syncDir:"             << syncDir << endl
         << "keypairDir:"          << keypairDir << endl
         << "------------------------------------------------------------------"
         << endl;

    string privkeyPath = keypairDir + "/" + "privkey.pem";
    string pubkeyPath = keypairDir + "/" + "pubkey.pem";
    
    ifstream privkeyIfs(privkeyPath);
    ifstream pubkeyIfs(pubkeyPath);
    if(privkeyIfs.fail() || pubkeyIfs.fail())
    {
        logger.log("no rsa key pair for master, now create new pairs");
        string strKey[2];
        string err;
        mkdirIfNotExist(keypairDir, err);
        generateRSAKey(strKey, pubkeyPath, privkeyPath);
    }else
    {
        string privkey((std::istreambuf_iterator<char>(privkeyIfs)),
                    std::istreambuf_iterator<char>());
        string pubkey((std::istreambuf_iterator<char>(pubkeyIfs)),
                    std::istreambuf_iterator<char>());
        this->privkey = privkey;
        this->pubkey = pubkey;
        if(!inspectRsaKeyPair(pubkey, privkey))
        {
            logger.fatal("rsa key pair not correct!");
            return false;
        }
    }

    return true;
}

int main(int argc, char *argv[]){
    Node node;
    node.forever(argc, argv);
}