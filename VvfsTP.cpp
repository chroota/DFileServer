#include "VvfsTp.hpp"

void VvfsTp::run(int argc, char *argv[]){
    init();
    if (argc < 2) {
        help();
    }

    if (!strcmp(argv[1], NEW_FILE)) 
    {
        // cout<<NEW_FILE<<endl;
        if (argc < 4) {
            help();
        }
        string err;
        newFile(argv[2], argv[3], err);
    }else if(!strcmp(argv[1], RM_FILE))
    {
        // cout<<RM_FILE<<endl;
        if(argc < 3){
            help();
        }
    }else if(!strcmp(argv[1], MV_FILE))
    {
        // cout<<MV_FILE<<endl;
        if(argc < 4){
            help();
        }
    
    }else if(!strcmp(argv[1], GET_FILE))
    {
        // cout<<GET_FILE<<endl;
        if(argc < 4){
            help();
        }
        
    }else if(!strcmp(argv[1], LS_FILE))
    {
        if(argc < 3){
            help();
        }
        // cout<<LS_FILE<<endl;
    }else{
        help();
    }
}


bool VvfsTp::newFile(const string & localPath, const string & remotePath, string & err){
    struct stat statBuf;
    if(stat(localPath.c_str(), &statBuf) == -1){
        logger.log("local path error");
        return false;
    }

    // create new file message
    int totalPackSize = statBuf.st_size / TRANS_CHUNK_SIZE;
    int lastBufSize = statBuf.st_size % TRANS_CHUNK_SIZE;
    if(lastBufSize !=0) totalPackSize++;

    Msg::Message sendMsg, recvMsg;
    NewFileMsgReqInst(sendMsg, remotePath, Msg::FT_FILE, totalPackSize, statBuf.st_size);
    char sendbuf[MAXBUFSIZE], recvBuf[MAXBUFSIZE];
    int recvLen;
    sendMsg.SerializePartialToArray(sendbuf, MAXBUFSIZE);
    if(!urequest(host.c_str(), port, sendbuf, strlen(sendbuf), recvBuf, recvLen)){
        logger.log("new file send fail");
        return false;
    }
    recvMsg.ParseFromArray(recvBuf, recvLen);
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR){
        err = recvMsg.response().info();
        logger.log(err);
        return false;
    }

    //logger.log("create remote file success: "+ remotePath + " postSessionId:");
    int postSessionId = recvMsg.response().new_file_response().post_session_id();
    // cout<<postSessionId<<endl;

    ifstream ifs(localPath);
    if(ifs.fail()){
        logger.log(strerror(errno));
        return false;
    }

    sendMsg.Clear();
    char databuf[TRANS_CHUNK_SIZE];
    int packIdx = 0, fileIdx = 0;

    memset(sendbuf, 0, sizeof(sendbuf));
    //vector<int> packFileIdxMap(totalPackSize);

    // send file data
    // todo design of high performance
    int readedSize;
    while(!ifs.eof()){
        memset(databuf, 0, TRANS_CHUNK_SIZE);
        fileIdx = ifs.tellg();
        try
        {
            ifs.read(databuf, TRANS_CHUNK_SIZE);
        }
        catch(const std::exception& e)
        {
            // std::cerr << e.what() << '\n';
            logger.fatal(e.what());
        }

        // packFileIdxMap
        //packFileIdxMap[packIdx] = fileIdx;

        if(ifs.eof()) readedSize = lastBufSize;
        else readedSize = TRANS_CHUNK_SIZE;
        

        FileChunkPostMsgInst(sendMsg, remotePath, databuf, fileIdx, packIdx, readedSize, postSessionId);
        if(strlen(sendbuf) >= MAXBUFSIZE){
            logger.debug("size of sendbuf larger than MAXBUFSIZE!");
            return false;
        }
        sendMsg.SerializeToArray(sendbuf, MAXBUFSIZE);
        // urequestNoResponse(host.c_str(), port, sendbuf, MAXBUFSIZE);

        int sendCount = 0;
        urequest(host.c_str(),port, sendbuf, MAXBUFSIZE, recvBuf, recvLen);
        recvMsg.ParseFromArray(recvBuf, MAXBUFSIZE);

        if (recvMsg.response().status() == Msg::MSG_RES_ERROR) {
            logger.log(L4, "error send at packIdx:%d fileIdx:%d", packIdx, fileIdx);
        }
        logger.log(L0,"success at packIdx:%d fileIdx:%d", packIdx, fileIdx);
        // cout<<sendMsg.file_post().data()<<endl;
        packIdx++;
    }

    return true;
}

bool VvfsTp::init(){
    host = "0.0.0.0";
    port = 8080;
    return true;
}


int main(int argc, char *argv[]){
    VvfsTp tp;
    tp.run(argc, argv);
}