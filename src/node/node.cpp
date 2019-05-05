#include <node.hpp>
bool Node::createVfs()
{
    pVvfs = new Vvfs();

    if(!pVvfs->initConfig(vfsPath))
    {
        logger.fatal("Vvfs cofnig init fail");
    }
    logger.debug("building vfs....");
    if(isStateNode && !pVvfs->buildVFS())
    {
        logger.fatal("Vvfs build error");
    }
    logger.debug("success build vfs");

    // if(!pVvfs->deamon()){
    //     logger.fatal("Vvfs demaon started failed");
    // }

    return true;
}

bool Node::createPushFileServer()
{
    pPushFileServer = new PushFileServer(pVvfs);
    return true;
}

bool Node::createPullFileServer()
{
    pPullFileServer = new PullFileServer();
    return true;
}

void Node::checkFileSync()
{
    //todo   
}

void Node::syncFileFn()
{
    //todo
}

bool Node::cacheNodeData(const string &name, const string &pubkey)
{
    if(name.size() == 0 || pubkey.size() == 0 || nodesDataCache.count(name) != 0) 
        return false;
    
    nodesDataCache.emplace
    (
        piecewise_construct,
        forward_as_tuple(name),
        forward_as_tuple(name, pubkey, getRandomKey())
    );
    return true;
}

NodeData *Node::getNodeDataFromcache(const string &name, string  &err)
{
    if(nodesDataCache.count(name) != 0)
        return &nodesDataCache[name];

    string pubkeyPath = keypairDir + "/" + name + "/pubkey.pem";
    ifstream pubkeyIfs(pubkeyPath);
    if(pubkeyIfs.fail())
    {
        err = name + " pubkey not existed";
        return nullptr;
    }

    string pubkey((std::istreambuf_iterator<char>(pubkeyIfs)),  
        std::istreambuf_iterator<char>());
    if(!cacheNodeData(name, pubkey))
    {
        err = "cache node data err";
        return nullptr;
    }
    return &nodesDataCache[name];
}

string Node::encryptByMyPrivKey(const string &data)
{
    return rsaEncryptByPrivkey(data, privkey);
}

string Node::encryptByOtherNodePubKey(const string & name, 
    const string &data)
{
    string err;
    NodeData *pNodeData = getNodeDataFromcache(name, err);
    if(pNodeData == nullptr)
        return "";
    return rsaEncryptByPubkey(data, pNodeData->pubkey);
}

string Node::decryptByOtherNodePubKey(const string & name, 
    const string &data)
{
    string err;
    NodeData *pNodeData = getNodeDataFromcache(name, err);
    if(pNodeData == nullptr)
        return "";
    return rsaDecryptByPubkey(data, pNodeData->pubkey);
}

string Node::decryptByMyPrivKey(const string &data)
{
    return rsaDecryptByPrivkey(data, privkey);
}

string Node::getEncryptedAuthkey()
{
    return encryptByMyPrivKey(myName);
}

bool Node::authenticateNode(const string &name, const string &auth, string &err)
{
    NodeData *pNodeData = getNodeDataFromcache(name, err);
    if(pNodeData == nullptr)
        return false;
    
    string decryptedAuth = decryptByOtherNodePubKey(name, auth);
    return name == decryptedAuth;
}

void Node::backendPushFileServerFunc()
{
    if(!createVfs())
    {
        logger.log("build VFS failed");
        exit(-1);
    }

    if(!createPushFileServer())
    {
        logger.fatal("build VFS push server failed");
    }

    pPushFileServer->listen(pushFileServerPort);
}

void Node::backendPullFileServerFunc()
{
    if(!createPullFileServer())
    {
        logger.fatal("build VFS pull server failed");
    }

    pPullFileServer->listen(pullFileServerPort);
}

void Node::backendStateServerFunc()
{
    this->listen(nodePort);
}


void Node::backendSyncVvfsFunc()
{
    cout << "backend sync func" << endl;;
    if(isStateNode)
        return;
    while(true)
    {
        this_thread::sleep_for(chrono::seconds(syncStateInterval));
        logger.log("get state");
        string newsetHash = requestState();

        if(newsetHash == stateHash)
        {
            logger.debug("is newest state, need not update vvfs");
            continue;
        }
        logger.debug("not newest state, need update vvfs");
        if(!requestUpdateFileOps())
        {
            logger.debug("error to get and update ops");
        }
    }
}


void Node::forever(int argc, char *argv[])
{
    if(!initConfig(argc, argv)) 
    {
        logger.log("config error");
        exit(-1);
    }
    thread stateServerThd(&Node::backendStateServerFunc, this);
    thread pushFileServerThd(&Node::backendPushFileServerFunc, this);
    thread syncVvfsThd(&Node::backendSyncVvfsFunc, this);
    syncVvfsThd.join();
    stateServerThd.join();
    pushFileServerThd.join();
}

string Node::requestState()
{
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    Msg::Message sendMsg = GetStateMsgReqInst(myName, myAuth);
    sendMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, sendMsg.ByteSize(), recvbuf, recvLen);
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
    {
        logger.log(recvMsg.response().info());
        return "";
    }
    string encryptedHash = recvMsg.response().get_state_res().hash();
    string decryptedHash = decryptByMyPrivKey(encryptedHash);
    cout << "recv state hash:" << decryptedHash << endl;
    return decryptedHash;
}

bool Node::responseState(Msg::Message & recvMsg, Msg::Message & resMsg)
{
    logger.log("request new state");
    string name = recvMsg.request().get_state_req().name();
    string auth = recvMsg.request().get_state_req().auth();
    string err;

    if(auth.size() == 0)
    {
        err = name + " auth error";
        logger.log(err);
        GetStateMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    if(!authenticateNode(name, auth, err))
    {
        logger.log(err);
        err = name + " auth error";
        logger.log(err);
        GetStateMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }
    stateHash = pVvfs->getHash();
    string encryptedHash = encryptByOtherNodePubKey(name, stateHash);
    GetStateMsgResInst(resMsg, Msg::MSG_RES_OK, "", encryptedHash);
    return true;
}


bool Node::requestUpdateFileOps()
{
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    Msg::Message sendMsg = GetFileOpsMsgReqInst(myName, pVvfs->getHash(), myAuth);
    sendMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, sendMsg.ByteSize(), recvbuf, recvLen);
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);

    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        logger.log(recvMsg.response().info());
        return false;
    }

    const Msg::GetFileOpsResponse &leftOpsResponse = recvMsg.response().get_file_ops_res();

    logger.debug("new size:" + to_string(leftOpsResponse.ops_size()));
    bool isOk = true;
    for(int i = 0; i < leftOpsResponse.ops_size(); i++)
    {
        const Msg::FileOperation &op = leftOpsResponse.ops(i);
        cout << op.type() << " " << op.src_path() << " " << op.dst_path() << " " << op.state_hash() << " " << op.optime() <<endl;
        isOk = syncVfOperation(op.type(), op.src_path(), op.dst_path(), op.state_hash(), op.optime());
        if(!isOk)
            break;
    }

    stateHash = pVvfs->getHash();
    return isOk;
}


bool Node::syncVfOperation(Msg::FileOpType type, const string &srcPath, 
    const string &dstPath, const string &hash, time_t opTime)
{
    bool ret;
    switch (type)
    {
    case Msg::NEW_OP:
        // ret = syncNewVfOp(srcPath, hash, opTime);
        break;
    case Msg::NEW_DIR_OP:
        ret = pVvfs->syncNewVDirOp(srcPath, hash, opTime);
        break;
    case Msg::RM_OP:
        ret = pVvfs->syncRmVfOp(srcPath, hash, opTime);
        break;
    case Msg::CP_OP:
        ret = pVvfs->syncCpVfOp(srcPath, dstPath, hash, opTime);
        break;
    case Msg::MV_OP:
        ret = pVvfs->syncMvVfOp(srcPath, dstPath, hash, opTime);
        break;
    default:
        return false;
    }

    return ret;
}

bool Node::syncFileFromOtherNode(const string &path, const string &ip, int port, string &err)
{
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    string err;

    Msg::Message sendMsg = GetFileMsgReqInst(myName, path, myAuth);
    sendMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, sendMsg.ByteSize(), recvbuf, recvLen);
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);

    if(recvMsg.response().status() == Msg::MSG_RES_ERROR)
    {
        logger.log(recvMsg.response().info());
        return false;
    }

    string sesseionKey = recvMsg.response().get_file_res().session_key();
    int totalFileSize = recvMsg.response().get_file_res().total_file_size();
    int recvFileSize = 0;

    string diskPath;
    if(!pVvfs->newVF(path, totalFileSize, err, diskPath))
    {
        err = "vvfs new vf error";
        return false;
    }

    string savePath = pVvfs->getVFPhasicalPath(diskPath);
    logger.log("save path:" + savePath);

    ofstream ofs(savePath, ios::trunc);
    if(ofs.fail())
    {
        err = "can't create file:" + savePath;
        logger.log(err);
        return false;
    }

    Msg::Message reqDataMsg;
    Msg::Message recvDataMsg;
    while (recvFileSize < totalFileSize)
    {
        memset(sendbuf, 0, MAXMSGSIZE);
        memset(recvbuf, 0, MAXMSGSIZE);
        GetFileChunkMsgReqInst(reqDataMsg, path, recvFileSize, sesseionKey);
        reqDataMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
        urequest(ip, port, sendbuf, reqDataMsg.ByteSize(), recvbuf, recvLen);
        recvDataMsg.ParseFromArray(recvbuf, recvLen);
        string recvData = recvDataMsg.response().get_file_chunk_res().data();
        if(recvData.size() < 0)
        {
            logger.fatal("recv data error");
            return false;
        }
        ofs.write(recvData.c_str(), recvData.size());

        if(ofs.fail())
        {
            logger.fatal("write data error");
            return false;
        }
        recvFileSize += recvData.size();
        recvDataMsg.Clear();
        reqDataMsg.Clear();
    }
    logger.debug("recieve data ok");
    return true;
}

bool Node::responseFileOps(Msg::Message &recvMsg, Msg::Message &resMsg)
{
    // todo: for large amount of ops
    logger.log("request new state");
    string name = recvMsg.request().get_file_ops_req().name();
    string auth = recvMsg.request().get_file_ops_req().auth();
    string newestHash = recvMsg.request().get_file_ops_req().newest_hash();
    string err;
    list<FileOperation> leftOps;

    if(auth.size() == 0)
    {
        err = name + " auth error";
        logger.log(err);
        GetFileOpsMsgResInst(resMsg, Msg::MSG_RES_ERROR, err);
        return false;
    }
    if(!authenticateNode(name, auth, err))
    {
        logger.log(err);
        err = name + " auth error";
        logger.log(err);
        GetFileOpsMsgResInst(resMsg, Msg::MSG_RES_ERROR, err);
        return false;
    }

    GetFileOpsMsgResInst(resMsg, Msg::MSG_RES_OK, "ok");
    NodeData *pNodeData = getNodeDataFromcache(name, err);
    // cout<<"fuck"<<endl;
    pVvfs->getLeftFileOpsToMsg(resMsg, newestHash);
    return true;
}

// update hash
bool Node::updateState(const string &hash)
{
    //todo
    return true;
}

// update status
bool Node::updateStatus(NODE_STATUS status)
{
    //todo
    return true;
}

bool Node::join()
{
    //todo
    return true;
}

bool Node::listen(int port)
{
    logger.log(L0, "node state server listening at port: %d", port);
    if (!start(port))
    {
        return false;
    }
    return true;
}

bool Node::handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse)
{
    if (recvLen <= 0) 
    {
        logger.log("error: len = 0");
        //todo response
        return false;
    }

    Msg::Message recvMsg;
    Msg::Message resMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);

    switch (recvMsg.type())
    {
    case Msg::GetState_Request:
        // getStateRequest()
        responseState(recvMsg, resMsg);
        break;
    case Msg::GetFileOps_Request:
        responseFileOps(recvMsg, resMsg);
        break;
    default:
        break;
    }

    resMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    sendLen = resMsg.ByteSize();
    return false;
}

void Node::help()
{
    cout << "-i: " << "node ip" << endl
         << "-p: " << "node port" << endl
         << "-n: " << "name" << endl
         << "-d: " << "sync directory" << endl
         << "-h: " << "master ip" << endl
         << "-q: " << "master port" << endl
         << "-l: " << "pull server port" << endl
         << "-s: " << "push server port" << endl
         << "-k: " << "key pair directory" << endl;
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
    myName = "node1";
    masterNode = "node1";
    vfsPath = "./test_dir/remote";
    // catchSegmentFaultError();
    keypairDir = "./test_dir/nodes_dir/";
    stateHash = "init state";
    syncStateInterval = 1;

    int ch;
    opterr = 0;

    while ((ch = getopt(argc, argv,"i:p:n:d:h:q:l:s:k:"))!= -1)
    {
        switch(ch)
        {
            case 'i': nodeIp = optarg;  break;
            case 'p': nodePort = atoi(optarg); break;
            case 'n': myName = optarg; break;
            case 'd': vfsPath = optarg;  break;
            case 'h': masterIp = optarg;  break;
            case 'q': masterPort = atoi(optarg);  break;
            case 'l': pullFileServerPort = atoi(optarg);  break;
            case 's': pushFileServerPort = atoi(optarg);  break;
            case 'k': keypairDir = optarg;  break;
            default: help(); exit(EXIT_FAILURE);
        }
    }

    if(masterNode == myName)
        isStateNode = true;

    // sfor test avoid confilct
    if(!isStateNode)
    {
        if(nodePort == 8888)
            nodePort = 9999;
        if(pushFileServerPort == 8080)
            pushFileServerPort = 9090;
    }

    cout << "args:"                << endl
         << "node ip:"             << nodeIp << endl
         << "master ip:"           << masterIp << endl
         << "node port:"           << nodePort << endl
         << "master port:"         << masterPort << endl
         << "pullFileServer port:" << pullFileServerPort << endl
         << "pushFileServer port:" << pushFileServerPort << endl
         << "name:"                << myName << endl
         << "vfsPath:"             << vfsPath << endl
         << "keypairDir:"          << keypairDir << endl
         << "------------------------------------------------------------------"
         << endl;
    

    string ownKeyPairDir = keypairDir + "/" + myName + "/";
    string privkeyPath = ownKeyPairDir + "privkey.pem";
    string pubkeyPath = ownKeyPairDir +"pubkey.pem";
    
    ifstream privkeyIfs(privkeyPath);
    ifstream pubkeyIfs(pubkeyPath);
    if(privkeyIfs.fail() || pubkeyIfs.fail())
    {
        logger.log("no rsa key pair for master, now create new pairs");
        string strKey[2];
        string err;
        mkdirIfNotExist(ownKeyPairDir, err);
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

    myAuth = encryptByMyPrivKey(myName);
    return true;
}

int main(int argc, char *argv[]){
    Node node;
    node.forever(argc, argv);
}