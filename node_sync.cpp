#include "node_sync.hpp"

bool FileSaver::open(string &err)
{
    if(type == Msg::FT_FILE){
        pOutput = new ofstream(savePath, ios::trunc);
    }
    if(pOutput->fail()){
        err = "can't create file";
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

    if(recvChunkCount == totalPackSize)
    {
        err = "file already complete";
        logger.log(err);
        return false;
    }

    // logger.debug(to_string(pRecvFlag->size()));
    
    if(pRecvFlag->at(packIdx))
    {
        err = "file chunk already writed at packIdx:" + to_string(packIdx);
        return false;
    }

    // cout<<"data size:"<<data.size()<<endl
    //     <<"data:"<<data<<endl;
    //todo exception
    pOutput->seekp(fileIdx);
    pOutput->write(data.c_str(), data.size());
    // pOutput->close();
    if(pOutput->fail())
    {
        err = "can't write file chunk";
        logger.log(err);
        return false;
    }
    recvChunkCount++;
    pRecvFlag->at(packIdx) = 1;
    return true;
}

bool FileSaver::isComplete()
{
    if(recvChunkCount == totalPackSize)
    {
        pOutput->close();
        return true;
    }
    return false;
}


bool UdpFileServer::handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse)
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
    if(recvMsg.type() == Msg::NewFile_Request){
        logger.debugAction("create file");
        return createNewFile(
            recvMsg.request().file().name(),
            recvMsg.request().file().type(),
            recvMsg.request().file().total_file_size(),
            recvMsg.request().file().total_pack_size(),
            sendbuf, 
            sendLen
        );
    }else if(recvMsg.type() == Msg::File_Post){
        logger.debugAction("file post");
        // todo async recieve
        // isResponse = false;
        recvChunk(
            recvMsg.file_post().name(),
            recvMsg.file_post().post_session_id(),
            recvMsg.file_post().file_idx(),
            recvMsg.file_post().pack_idx(),
            recvMsg.file_post().data(),
            sendbuf,
            sendLen
        );
    }else{
        logger.log("msg type not found for udp file server!");
    }

    return false;
}


bool UdpFileServer::createNewFile(const string &name, Msg::FileType type, int totalFileSize, 
    int totalPackSize, char sendbuf[], int &sendLen)
{
    string err;
    Msg::Message msg;
    if(savers.size() >= MAX_OPEN_FILES){
        err = "exceeding max open file nums";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    // cout<<name<<endl;
    if(name[0] != '/'){
        err = "error name path";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    if(!pVvfs->newVF(name, err)){
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    string savePath = pVvfs->vfsPath + "/" + name.substr(1);
    logger.log("save path:" + savePath);

    // create file saver
    FileSaver *pFileSarver = new FileSaver(savePath, type, totalFileSize, totalPackSize);

    if(!pFileSarver->open(err))
    {
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    savers[name] = pFileSarver;
    logger.debug("saver len:" + to_string(savers.size()));

    // allocate session post id
    int sessionId = getSessionId();
    logger.log(LDEBUG, "allocate session id:%d", sessionId);
    sessions[name] = sessionId;
    NewFileMsgResInst(msg, Msg::MSG_RES_OK, sessionId, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    return true;
}

bool UdpFileServer::recvChunk(const string &name, int sessionId, int fileIdx, int packIdx, const string & data, char sendbuf[], int &sendLen)
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
        sendLen = strlen(sendbuf);
        return false;
    }

    if(!savers[name]->writeChunk(data, fileIdx, packIdx, err))
    {
        logger.debug(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
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
    sendLen = strlen(sendbuf);
    
    return true;
}

bool UdpFileServer::listen(int port)
{
    //todo deamon
    logger.log(L0, "listening at port: %d", port);
    if (!start(port)) {
        return false;
    }
    return true;
}

bool UdpFileServer::test(){
    // pVvfs->vfrsTest();
    return false;
}



bool NodeSync::createVFS()
{
    pVvfs = new Vvfs(syncDir);
    return pVvfs->buildVFS();
}

bool NodeSync::createFileServer()
{
    pFileServer = new UdpFileServer(pVvfs);
    return true;
}


void NodeSync::checkFileSync(){
    
}

void NodeSync::syncFileFn()
{
    logger.log("check file sync");
    while(true)
    {
        logger.log("check file sync");
        this_thread::sleep_for(chrono::seconds(checkFileSyncTime));
    }
}

void NodeSync::serverFn()
{
    int i = 0;
    while(i++ < 100)
    {
        std::cout<<"server fn"<<std::endl;
        sleep(1);
    }
}

void NodeSync::keepAliveFn(){
    logger.log("test");
    while(true)
    {
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

    if(!createVFS()){
        logger.log("build VFS failed");
        exit(-1);
    }

    if(!createFileServer()){
        logger.log("build VFS server failed");
        exit(-1);
    }
    pFileServer->listen(8080);
    // thread UdpFileServer
}


bool NodeSync::getStateNode()
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];

    Msg::Message msg = GetStateNodeMsgReqInst(myName.c_str());
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, recv_len);
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
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, recv_len);
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
    int recvLen;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = UpdateStatusMsgReqInst(myName.c_str(), status);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, recvLen);
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

bool NodeSync::join()
{
    char recvbuf[MAXBUFSIZE];
    int recv_len;
    char sendbuf[MAXBUFSIZE];
    Msg::Message msg = JoinMsgReqInst(myName.c_str(), "172.23.21.1");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    //todo catch error recvMsg
    urequest(master.host, master.port, sendbuf, strlen(sendbuf), recvbuf, recv_len);
    // printf("%d\n", strlen(recvbuf));
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
    syncDir = "./test_dir/remote";
    return true;
}

int main(){
    NodeSync node;
    node.start();
}