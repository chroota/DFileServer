#include "master.hpp"

bool Master::listen(int port)
{
    initConfig();
    printf("listening at port: %d\n", port);
    if (!start(port)) 
    {
        return false;
    }
    return true;
}

bool Master::forever(int argc, char *argv[])
{
    if(!initConfig(argc, argv))
    {
        logger.fatal("fail to config master");
    }
    printf("listening at port: %d\n", listenPort);
    if (!start(listenPort)) 
    {
        return false;
    }
    return true;
}

bool Master::join(Msg::Message &resMsg, const string &name, const string &ip, const string &port, const string &auth)
{
    string pubkeyPath = keypairDir + "/" + name + "_pubkey.pem";
    ifstream pubkeyIfs(pubkeyPath);
    string err;

    if(auth.size() == 0)
    {
        err = name + " auth error";
        logger.log(err);
        JoinMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    if(pubkeyIfs.fail())
    {
        err = name + " pubkey not existed";
        logger.log(err);
        JoinMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    string pubkey((std::istreambuf_iterator<char>(pubkeyIfs)),  std::istreambuf_iterator<char>());
    string decryptAuth = rsaDecryptByPubkey(auth, pubkey);
    if(decryptAuth != name)
    {
        err = name + " auth fail";
        logger.log(err);
        JoinMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    if(nodes.count(name))
    {
        nodes.erase(name);
        // todo
        // logger.log("existed!");
        // CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, "name existed");
    }
    string encryptKey = getRandomKey(ENC_KEY_LENGTH);
    logger.debug("grant encrypt key:" + encryptKey);
    string encryptedEncryptKey = rsaEncryptByPubkey(encryptKey, pubkey);
    Node newNode(name, ip, port, encryptKey);
    newNode.setPubkey(pubkey);
    nodes[name] = newNode;
    JoinMsgResInst(resMsg, Msg::MSG_RES_OK, "ok", encryptedEncryptKey);
    logger.log(name+" success join");
    return true;
}

bool Master::updateStatus(Msg::Message &resMsg, const string &name, NODE_STATUS status)
{
    logger.log("change status");
    if (!nodes.count(name)) 
    {
        logger.log("node not exist!");
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, "name existed");
    }

    nodes[name].changeStatus(status);
    return true;
}

bool Master::updateState(Msg::Message &resMsg, const string &name, const string &hash, const string &auth)
{
    // string pubkeyPath = keypairDir + "/" + name + "_pubkey.pem";
    // ifstream pubkeyIfs(pubkeyPath);
    logger.log("update state");
    string err;

    //todo exception to deal
    if (!nodes.count(name)) 
    {
        err = "node not exist!";
        logger.log(err);
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, err.c_str());
    }
    else if(name != stateName)
    {
        cout << "node:" << name << " state_node:" << stateName << endl;
        err = "you are not state node!";
        logger.log(err);
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, err.c_str());
    }

    if(auth.size() == 0)
    {
        err = name + " auth error";
        logger.log(err);
        JoinMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    string pubkey = nodes[name].getPubkey();
    string decryptAuth = rsaDecryptByPubkey(auth, pubkey);

    
    if(decryptAuth != name)
    {
        err = name + " auth fail";
        logger.log(err);
        JoinMsgResInst(resMsg, Msg::MSG_RES_ERROR, err, "None");
        return false;
    }

    cout << "recieve hash:" << hash << endl;
    setState(hash);
    return true;
}

bool Master::getStateNode(Msg::Message &resMsg, const string &name)
{
    // logger.log("get state");

    // //todo exception to deal
    // if (!nodes.count(name)) 
    // {
    //     logger.log("node not exist!");
    //     GetStateNodeMsgResErrorInst(resMsg, "node not exist!");
    // }else
    // {
    //     GetStateNodeMsgResInst(resMsg, stateName, nodes[stateName].getConnstring(), stateHash);
    // }

    return false;
}


bool Master::handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool &isResponse)
{
    Msg::Message recvMsg;
    Msg::Message resMsg;
    recvMsg.ParseFromArray(recvbuf, MAXMSGSIZE);
    // node join
    if (recvMsg.type() == Msg::Join_Request)
    {
        string name = recvMsg.request().join().name();
        string ip = recvMsg.request().join().ip();
        string port = recvMsg.request().join().port();
        string auth = recvMsg.request().join().auth();
        join(resMsg, name, ip, port, auth);   
    }
    // node change status
    else if(recvMsg.type() == Msg::UpdateStatus_Request)
    {
        updateStatus(
            resMsg,
            recvMsg.request().update_status().name(), 
            (NODE_STATUS)recvMsg.request().update_status().status()
        );
    }
    // update state hash
    else if(recvMsg.type() == Msg::UpdateState_Request)
    {
        updateState(
            resMsg,
            recvMsg.request().state().name(), 
            recvMsg.request().state().hash(),
            recvMsg.request().state().auth()
        );
    }
    // get stateNode for update
    else if(recvMsg.type() == Msg::GetState_Request)
    {
        // getStateNode(
        //     resMsg,
        //     recvMsg.request().get_state_node().name()
        // );
    }

    resMsg.SerializeToArray(sendbuf, MAXMSGSIZE);
    sendLen = resMsg.ByteSize();
    return true;
}


bool Master::initConfig()
{
    stateName = DEFAULT_STATE_NODE;
    return true;
}

bool Master::initConfig(int argc, char *argv[])
{
    int ch;
    listenIp = "0.0.0.0";
    listenPort = 8888;
    opterr = 0;
    keypairDir = "./test_dir/master_dir";
    stateName = DEFAULT_STATE_NODE;
    // catchSegmentFaultError();

    while ((ch = getopt(argc, argv,"i:p:n:d:h:q:k:e:"))!= -1)
    {
        switch(ch){
            case 'i': 
                listenIp = optarg;  
                break;
            case 'p': 
                listenPort = atoi(optarg); 
                break;
            case 'k': 
                keypairDir = optarg; 
                break;
             case 'e': 
                stateName = optarg; 
                break;
            default:
                cout << "unkown option :" << ch;
                exit(EXIT_FAILURE);
        }
    }

    string privkeyPath = keypairDir + "/" + "privkey.pem";
    string pubkeyPath = keypairDir + "/" + "pubkey.pem";
    
    ifstream privkeyIfs(privkeyPath);
    ifstream pubkeyIfs(pubkeyPath);
    if(privkeyIfs.fail() || pubkeyIfs.fail())
    {
        logger.log("no rsa key pair for master, now create new pairs");
        string strKey[2];
        generateRSAKey(strKey, pubkeyPath, privkeyPath);
    }else
    {
        string privkey((std::istreambuf_iterator<char>(privkeyIfs)),  
                    std::istreambuf_iterator<char>());
        string pubkey((std::istreambuf_iterator<char>(pubkeyIfs)),  
                    std::istreambuf_iterator<char>());
        if(!inspectRsaKeyPair(pubkey, privkey))
        {
            logger.fatal("rsa key pair not correct!");
            return false;
        }
    }

    cout << "args:"<<endl
         << "listen ip:"   << listenIp << endl
         << "listen port:" << listenPort << endl
         << "------------------------------------------------------------------"
         << endl;
    return true;
}

int main(int argc, char *argv[])
{
    Master master;
    master.forever(argc, argv);
}