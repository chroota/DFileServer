#include "node_sync.hpp"


bool UdpDataServer::handle(char recvbuf[], int recv_len){
    return true;
}




void NodeSync::checkFileSync(){
    
}

void NodeSync::syncFileFn(){
    logger.log("check file sync");
    while(true){
        logger.log("check file sync");
        this_thread::sleep_for(chrono::seconds(checkFileSyncTime));
    }
}

void NodeSync::serverFn(){
    int i = 0;
    while(i++ < 100){
        std::cout<<"server fn"<<std::endl;
        sleep(1);
    }
}

void NodeSync::keepAliveFn(){
    logger.log("test");
    while(true){
        // std::cout<<"request fn"<<std::endl;
        updateStatus(NODE_STATUS_ALIVE);
        this_thread::sleep_for(chrono::seconds(aliveSyncTime));
    }
}

void NodeSync::start(){
    if (!initConfig()) {
        logger.log("config error");
        exit(-1);
    }
    // if (!join()) {
    //     logger.log("join error");
    //     exit(-1);
    // }

    // if(!updateState("test hash")){
    //     logger.log("update state hash error");
    //     exit(-1);
    // }

    // if(!getStateNode()){
    //     logger.log("get state node error");
    //     exit(-1);
    // }
    
    // std::thread keepAliveThd(&NodeSync::keepAliveFn, this);
    // std::thread fileSyncThd(&NodeSync::syncFileFn, this);
    // keepAliveThd.join();
    // fileSyncThd.join();

    if(!buildMyFs()){
        logger.log("build Fs failed");
        exit(-1);
    }
}


bool NodeSync::getStateNode()
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];

    Msg::Message msg = GetStateNodeMsgReqInst(myName.c_str());
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, &recv_len);
    Msg::Message recvMsg;
    // printf("%d\n", strlen(recvbuf));
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) {
        logger.log(recvMsg.response().info());
        return false;
    }
    // logger.log(recvMsg.response().info());
    // cout<<"name:" << recvMsg.response().state_node().name()
    //     <<"ip:" << recvMsg.response().state_node().ip()
    //     <<"hash:" << recvMsg.response().state_node().hash()
    //     <<endl;
    return true;
}

// update hash
bool NodeSync::updateState(const string &hash)
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = UpdateStateHashMsgReqInst(myName.c_str(), hash);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, &recv_len);
    Msg::Message recvMsg;
    // printf("%d\n", strlen(recvbuf));
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) {
        logger.log(recvMsg.response().info());
        return false;
    }
    logger.log(recvMsg.response().info());
    return true;
}

// update status
bool NodeSync::updateStatus(NODE_STATUS status)
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = UpdateStatusMsgReqInst(myName.c_str(), status);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, &recv_len);
    Msg::Message recvMsg;
    // printf("%d\n", strlen(recvbuf));
    recvMsg.ParseFromArray(recvbuf, MAXBUFSIZE);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) {
        logger.log(recvMsg.response().info());
        return false;
    }
    logger.log(recvMsg.response().info());
    return true;
}

bool NodeSync::join()
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = JoinMsgReqInst(myName.c_str(), "172.23.21.1");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error recvMsg
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, &recv_len);
    printf("%d\n", strlen(recvbuf));
    Msg::Message recvMsg;
    recvMsg.ParseFromArray(recvbuf, recv_len);
    if (recvMsg.response().status() == Msg::MSG_RES_ERROR) {
        logger.log(recvMsg.response().info());
        return false;
    }
    logger.log(recvMsg.response().info());
    return true;
}

bool NodeSync::initConfig()
{
    //todo parse config
    master.host = "0.0.0.0";
    master.port = 8888;
    aliveSyncTime = ALIVE_SYNC_TIME;
    myName = "node1";
    syncDir = "test_dir";
    return true;
}



bool NodeSync::buildMyFs(){
    vvfs = Vvfs(syncDir);
    vvfs.buildVFS();
}

int main(){
    NodeSync ns;
    ns.start();
}