#include <node.hpp>
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

bool Node::cacheNodeData(const string &name, const string &pubkey)
{
    // cout << name << endl << pubkey << endl;
    if(name.size() == 0 || pubkey.size() == 0 || nodesDataCache.count(name) != 0) 
        return false;
    
    nodesDataCache.emplace(
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
    // if (!join()) 
    // {
    //     logger.log("join error");
    //     exit(-1);
    // }

    /*
     * for test
    */
    // if(isStateNode && !updateState(pVvfs->getHash()))
    // {
    //     logger.log("update hash error");
    //     exit(-1);
    // }

    if(!isStateNode && !requestState())
    {
        logger.log("get state error");
        exit(-1);
    }

    this->listen(nodePort);

    // pPushFileServer->listen(pushFileServerPort);
}

bool Node::requestState()
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
        return false;
    }
    string encryptedHash = recvMsg.response().get_state_res().hash();
    string decryptedHash = decryptByMyPrivKey(encryptedHash);
    cout << "recv state hash:" << decryptedHash << endl;
    return true;
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

    string encryptedHash = encryptByOtherNodePubKey(name, stateHash);
    GetStateMsgResInst(resMsg, Msg::MSG_RES_OK, "", encryptedHash);
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
    syncDir = "./test_dir/remote";
    // catchSegmentFaultError();
    keypairDir = "./test_dir/nodes_dir/";
    stateHash = "init state";

    int ch;
    opterr = 0;
    while ((ch = getopt(argc, argv,"i:p:n:d:h:q:l:s:k:"))!= -1)
    {
        switch(ch)
        {
            case 'i': nodeIp = optarg;  break;
            case 'p': nodePort = atoi(optarg); break;
            case 'n': myName = optarg; break;
            case 'd': syncDir = optarg;  break;
            case 'h': masterIp = optarg;  break;
            case 'q': masterPort = atoi(optarg);  break;
            case 'l': pullFileServerPort = atoi(optarg);  break;
            case 's': pushFileServerPort = atoi(optarg);  break;
            case 'k': keypairDir = optarg;  break;
            default: help(); exit(EXIT_FAILURE);
        }
    }
    cout << "args:"                << endl
         << "node ip:"             << nodeIp << endl
         << "master ip:"           << masterIp << endl
         << "node port:"           << nodePort << endl
         << "master port:"         << masterPort << endl
         << "pullFileServer port:" << pullFileServerPort << endl
         << "pushFileServer port:" << pushFileServerPort << endl
         << "name:"                << myName << endl
         << "syncDir:"             << syncDir << endl
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
    if(masterNode == myName)
        isStateNode = true;

    return true;
}

int main(int argc, char *argv[]){
    Node node;
    node.forever(argc, argv);
}