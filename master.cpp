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

bool Master::join(const string &name, const string &ip, const string &port, char sendbuf[], int &sendLen)
{
    puts("node join");
    // cout<<name<<" "<<ip<<endl;

    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "ok");

    if(nodes.count(name))
    {
        logger.log("existed!");
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, "name existed");
    }
    
    Node node(name, ip, port);
    nodes[name] = node;
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    // logger.log("join:"+)
    return true;
}

bool Master::updateStatus(const string &name, NODE_STATUS status, char sendbuf[], int &sendLen)
{
    logger.log("change status");
    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "update status: ok");
    if (!nodes.count(name)) 
    {
        logger.log("node not exist!");
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, "name existed");
    }

    nodes[name].changeStatus(status);
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    cout<<sendLen<<endl;
    return true;
}

bool Master::updateState(const string &name, const string &hash, char sendbuf[], int &sendLen)
{
    logger.log("update state");
    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "update hash: ok");

    //todo exception to deal
    string err;
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
    else
    {
        cout<<"recieve hash:"<<hash<<endl;
        setState(name, hash);
    }
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    return true;
}

bool Master::getStateNode(const string &name, char sendbuf[], int &sendLen)
{
    logger.log("get state");
    Msg::Message resMsg;

    //todo exception to deal
    if (!nodes.count(name)) 
    {
        logger.log("node not exist!");
        GetStateNodeMsgResErrorInst(resMsg, "node not exist!");
    }else
    {
        GetStateNodeMsgResInst(resMsg, stateName, nodes[stateName].getConnstring(), stateHash);
    }

    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    return false;
}


bool Master::handle(char recvbuf[], int recv_len, char sendbuf[], int &sendLen, bool &isResponse)
{

    if (strlen(recvbuf) == 0 || recv_len <= 0) 
    {
        puts("error: len = 0");
        return false;
    }

    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    // node join
    if (recvMsg.type() == Msg::Join_Request)
    {
        string name = recvMsg.request().join().name();
        string ip = recvMsg.request().join().ip();
        string port = recvMsg.request().join().port();
        return join(name, ip, port, sendbuf, sendLen);   
    }
    
    // node change status
    else if(recvMsg.type() == Msg::UpdateStatus_Request)
    {
        return updateStatus
        (
            recvMsg.request().update_status().name(), 
            (NODE_STATUS)recvMsg.request().update_status().status(), 
            sendbuf, sendLen
        );
    }
    // update state hash
    else if(recvMsg.type() == Msg::UpdateStateHash_Request)
    {
        return updateState
        (
            recvMsg.request().state_hash().name(), 
            recvMsg.request().state_hash().hash(),
            sendbuf, sendLen
        );
    }
    // get stateNode for update
    else if(recvMsg.type() == Msg::GetStateNode_Request)
    {
        return getStateNode
        (
            recvMsg.request().get_state_node().name(), 
            sendbuf, sendLen
        );       
    }
    return true;
}

bool Master::initConfig()
{
    stateName = DEFAULT_STATE_NODE;
    return true;
}

bool Master::initConfig(int argc, char *argv[])
{
    listenIp = "0.0.0.0";
    listenPort = 8888;

    int ch;
    opterr = 0;
    while ((ch = getopt(argc, argv,"i:p:n:d:h:q"))!= -1)
    {
        switch(ch)
        {
            case 'i': listenIp = optarg;  break;
            case 'p': listenPort = atoi(optarg); break;
            default: printf("unkown option :%c\n",ch); exit(EXIT_FAILURE);
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