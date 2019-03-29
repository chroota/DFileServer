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
    if(statBuf.st_size % TRANS_CHUNK_SIZE !=0) totalPackSize++;

    Msg::Message sendMsg, recvMsg;
    NewFileMsgReqInst(sendMsg, remotePath, Msg::FT_FILE, totalPackSize, statBuf.st_size);
    char sendbuf[MAXBUFSIZE], recvBuf[MAXBUFSIZE];
    int recvLen;
    sendMsg.SerializePartialToArray(sendbuf, MAXBUFSIZE);
    if(!urequest(host.c_str(), port, sendbuf, strlen(sendbuf), recvBuf, recvLen)){
        logger.log("new file send fail");
        return false;
    }
    logger.log("create remote file success: "+ remotePath);
    recvMsg.ParseFromArray(recvBuf, recvLen);

    cout<<"debug"<<endl;
    return false;
    if(recvMsg.response().status() == Msg::MSG_RES_ERROR){
        err = recvMsg.response().info();
        return false;
    }

    int postSessionId = recvMsg.response().new_file_response().post_session_id();

    ifstream ifs(localPath, ios::binary);

    if(!ifs.fail()){
        logger.log("input stream error");
    }

    Msg::Message msg;
    char databuf[TRANS_CHUNK_SIZE];
    int packIdx = 0, fileIndex = 0;

    while(!ifs.eof()){
        fileIndex = ifs.tellg();
        ifs.read(databuf, TRANS_CHUNK_SIZE);
        FileChunkPostMsgInst(msg, localPath, databuf, fileIndex, packIdx,TRANS_CHUNK_SIZE, postSessionId);
        msg.SerializePartialToArray(sendbuf, MAXBUFSIZE);
        urequestNoResponse(host.c_str(), port, sendbuf, strlen(sendbuf));
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