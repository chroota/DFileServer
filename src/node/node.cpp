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

    if(!isStateNode && !getStateRequest())
    {
        logger.log("get state error");
        exit(-1);
    }

    pPushFileServer->listen(pushFileServerPort);
}

bool Node::getStateRequest()
{
    char recvbuf[MAXMSGSIZE];
    int recvLen;
    char sendbuf[MAXMSGSIZE];
    string auth = rsaEncryptByPrivkey(name, privkey);
    Msg::Message msg = GetStateMsgReqInst(name, auth);
    msg.SerializeToArray(sendbuf, MAXMSGSIZE);
    //todo catch error
    urequest(masterIp, masterPort, sendbuf, strlen(sendbuf), recvbuf, recvLen);
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) 
    {
        logger.log(recvMsg.response().info());
        return false;
    }
    stateHash = recvMsg.response().info();
    cout<<"recv state hash:"<<endl;
    return true;
}


bool Node::getStateResponse(Msg::Message & recvMsg, Msg::Message & resMsg)
{
    logger.log("request new state");
    string name = recvMsg.request().get_state_req().name();
    string auth = recvMsg.request().get_state_req().auth();
    string err;

    if(auth.size() == 0)
    {
        err = name + " auth error";
        logger.log(err);
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, err);
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
    if (strlen(recvbuf) == 0 || recvLen <= 0) 
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
    case Msg::GetState_Request:
        // getStateRequest()
        getStateResponse(recvMsg, resMsg);
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
    name = "node1";
    masterNode = "node1";
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

    if(masterNode == name)
        isStateNode = true;

    return true;
}

int main(int argc, char *argv[]){
    Node node;
    node.forever(argc, argv);
}