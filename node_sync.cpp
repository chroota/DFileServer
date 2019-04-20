#include "node_sync.hpp"

bool FileSaver::open(string &err)
{
    if(type == Msg::FT_FILE){
        pOutput = new ofstream(savePath, ios::trunc);
    }
    if(pOutput->fail()){
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

    // logger.debug(to_string(pRecvFlag->size()));
    if(pRecvFlag->at(packIdx))
    {
        err = "file chunk already writed at packIdx:" + to_string(packIdx);
        return false;
    }
    // cout<<"complete: fileIdx:"<<fileIdx<<" "<<"packIdx:"<<packIdx<<endl;

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
    recvFileSizeCount += data.size();
    pRecvFlag->at(packIdx) = 1;
    return true;
}

bool FileSaver::isComplete()
{

    //todo verify file hash
    // cout<<recvChunkCount<<"/"<<totalPackSize<<endl;
    // if(recvChunkCount == totalPackSize)
    if(recvChunkCount == totalPackSize && recvFileSizeCount == totalFileSize)
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
    if(recvMsg.type() == Msg::NewFile_Request)
    {
        logger.debugAction("create file");
        return createNewFile
        (
            recvMsg.request().file().name(),recvMsg.request().file().type(),
            recvMsg.request().file().total_file_size(),recvMsg.request().file().total_pack_size(),
            sendbuf, sendLen
        );
    }else if(recvMsg.type() == Msg::File_Post)
    {
        logger.debugAction("file post");
        recvChunk
        (
            recvMsg.file_post().name(),recvMsg.file_post().post_session_id(),
            recvMsg.file_post().file_idx(),recvMsg.file_post().pack_idx(),
            recvMsg.file_post().data(),sendbuf,sendLen
        );

    }else if(recvMsg.type() == Msg::Rm_File_Request){
        string rmPath = recvMsg.request().rm_op().path();
        logger.debugAction("rmFile:" + rmPath);
        rmFile(rmPath, sendbuf, sendLen);
    }else if(recvMsg.type() == Msg::LsFile_Request)
    {
        string lsPath = recvMsg.request().ls_file_req().path();
        logger.debugAction("ls file:" + lsPath);
        lsFiles(lsPath, sendbuf, sendLen);
    }
    else{
        logger.log("msg type not found for udp file server!");
    }

    return false;
}


bool UdpFileServer::createNewFile(const string &path, Msg::FileType type, int totalFileSize, 
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

    if(path[0] != '/'){
        err = "error name path";
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    if(!pVvfs->newVF(path, totalFileSize, err)){
        logger.log(err);
        NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }

    string savePath = pVvfs->vfsPath + "/" + path.substr(1);
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

    savers[path] = pFileSarver;
    logger.debug("saver len:" + to_string(savers.size()));

    // allocate session post id
    int sessionId = getSessionId();
    logger.log(LDEBUG, "allocate session id:%d", sessionId);
    sessions[path] = sessionId;
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

    cout<<"recv "<<"fileIdx:"<<fileIdx<<" "<<"packIdx:"<<packIdx<<endl;

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


bool UdpFileServer::rmFile(const string &path, char sendbuf[], int &sendLen){
    string err;
    Msg::Message msg;
    if(!pVvfs->rmVF(path, err)){
        CommonMsgResInst(msg, Msg::MSG_RES_ERROR, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }
    CommonMsgResInst(msg, Msg::MSG_RES_OK, "ok");
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    return true;
}

bool UdpFileServer::lsFiles(const string &path, char sendbuf[], int &sendLen){
    string err;
    Msg::Message msg;
    if(path[0] != '/')
    {
        err = "error name path";
        logger.log(err);
        //NewFileMsgResInst(msg, Msg::MSG_RES_ERROR, -1, err.c_str());
        msg.SerializeToArray(sendbuf, MAXBUFSIZE);
        sendLen = strlen(sendbuf);
        return false;
    }
    bool ret = pVvfs->lsVF(path, msg);
    msg.SerializeToArray(sendbuf, MAXBUFSIZE);
    sendLen = strlen(sendbuf);
    return ret;
}



bool NodeSync::createVFS()
{
    pVvfs = new Vvfs();

    if(!pVvfs->initConfig(syncDir)){
        logger.fatal("Vvfs cofnig init fail");
    }

    if(!pVvfs->buildVFS()){
        logger.fatal("Vvfs build error");
    }

    // if(!pVvfs->deamon()){
    //     logger.fatal("Vvfs demaon started failed");
    // }

    return true;
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

void NodeSync::forever(){
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
    node.forever();
}