#include "master_sync.hpp"

bool MasterSync::listen(int port)
{
    initConfig();
    printf("listening at port: %d\n", port);
    if (!start(port)) {
        return false;
    }
    return true;
}

bool MasterSync::join(const string &name, const string &ip, char sendbuf[], int &send_len)
{
    puts("node join");
    cout<<name<<" "<<ip<<endl;
    // string ip(), name(join.name());

    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "ok");

    // name exists
    if(nodes.count(name))
    {
        logger.log("existed!");
        resMsg = CommonMsgResInst(Msg::MSG_RES_ERROR, "name existed");
    }
    
    Node node(name, ip);
    nodes[name] = node;
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    send_len = strlen(sendbuf);
    return true;
}

bool MasterSync::updateStatus(const string &name, NODE_STATUS status, char sendbuf[], int &send_len)
{
    logger.log("change status");
    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "update status: ok");
    if (!nodes.count(name)) 
    {
        logger.log("node not exist!");
        resMsg = CommonMsgResInst(Msg::MSG_RES_ERROR, "name existed");
    }

    nodes[name].change_status(status);
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    send_len = strlen(sendbuf);
    cout<<send_len<<endl;
    return true;
}

bool MasterSync::updateState(const string &name, const string &hash, char sendbuf[], int &send_len)
{
    logger.log("update state");
    Msg::Message resMsg = CommonMsgResInst(Msg::MSG_RES_OK, "update hash: ok");

    //todo exception to deal
    string err;
    if (!nodes.count(name)) 
    {
        err = "node not exist!";
        logger.log(err);
        // resMsg = CommonMsgResInst(Msg::MSG_RES_ERROR, "name existed");
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, err.c_str());
    }
    else if(name != state_name)
    {
        cout << "node:" << name << " state_node:" << state_name << endl;
        err = "you are not state node!";
        logger.log(err);
        // resMsg = CommonMsgResInst(Msg::MSG_RES_ERROR, "name existed");
        CommonMsgResInst(resMsg, Msg::MSG_RES_ERROR, err.c_str());
    }
    else
    {
        cout<<"recieve hash:"<<hash<<endl;
        setState(name, hash);
    }
    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    send_len = strlen(sendbuf);
    return true;
}

bool MasterSync::getStateNode(const string &name, char sendbuf[], int &send_len)
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
        GetStateNodeMsgResInst(resMsg, state_name, nodes[state_name].get_ip(), state_hash);
    }

    resMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
    send_len = strlen(sendbuf);
    return false;
}


bool MasterSync::handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len)
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
        return join(name, ip, sendbuf, send_len);   
    }
    
    // node change status
    else if(recvMsg.type() == Msg::UpdateStatus_Request)
    {
        return updateStatus(
            recvMsg.request().update_status().name(), 
            (NODE_STATUS)recvMsg.request().update_status().status(), 
            sendbuf, send_len);        
    }
    // update state hash
    else if(recvMsg.type() == Msg::UpdateStateHash_Request)
    {
        return updateState(
            recvMsg.request().state_hash().name(), 
            recvMsg.request().state_hash().hash(),
            sendbuf, send_len);
    }
    else if(recvMsg.type() == Msg::GetStateNode_Request)
    {
        return getStateNode(
            recvMsg.request().get_state_node().name(), 
            sendbuf, send_len);       
    }
    return true;
}


bool MasterSync::initConfig(){
    state_name = DEFAULT_STATE_NODE;
}

// string & MasterSync::getNodes(){
//     string nodes_str;
//     nodes_str.push_back('\x06');
//     for(int i = 0; i < node_names.size(); i++)
//     {
//         nodes_str += " ";   
//     }
// }

int main(int argc, char *argv[]){
    MasterSync sync;
    sync.listen(8888);
}