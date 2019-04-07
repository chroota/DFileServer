#include "common.hpp"


struct timespec getTimeSpec(){
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

long long getSystemTime()
{
	struct timeb t;
	ftime(&t);
	return 1000*t.time + t.millitm;
}

void ssplit(const string& s, vector<string>& sv, const char flag) {
    sv.clear();
    istringstream iss(s);
    string temp;

    while (getline(iss, temp, flag)) {
        if(temp.size() == 0) continue;
        sv.push_back(temp);
    }
    return;
}


// 
string getBufMD5(const void * buf, int len){
    MD5_CTX md5Context;
    MD5Init(&md5Context);
    MD5Update(&md5Context, (unsigned char *)buf, len);
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5Final(&md5Context, result);
    return getMD5string(result, MD5_DIGEST_LENGTH);
}

string getMD5string(unsigned char buf[], int len = MD5_DIGEST_LENGTH){
    char hex[35];
    memset(hex, 0, sizeof(hex));
    for (int i = 0; i < len; ++i)
    {
        sprintf(hex + i * 2, "%02x", buf[i]);
    }
    hex[32] = '\0';
    return string(hex);
}


// udp client
bool urequest(const char * host, int port,const char sendbuf[], int send_len, char recvbuf[], int &recv_len)
{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) return false;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    socklen_t len = sizeof(addr);

    //todo resend & time limit to send
    if(sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&addr,len) < 0){
        return false;
    }
    recv_len = recvfrom(sockfd, recvbuf, MAXBUFSIZE,0,(struct sockaddr*)&addr,&len);
	return true;
}


bool urequest(const string  & host, int port,const char sendbuf[], int send_len, char recvbuf[], int &recv_len)
{
    return urequest(host.c_str(), port, sendbuf, send_len, recvbuf, recv_len);
}

bool urequestNoResponse(const char * host, int port, const char sendbuf[], int send_len)
{
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) return false;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    socklen_t len = sizeof(addr);
    // sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&addr,len);
    if(sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&addr,len) < 0){
        return false;
    }
    return true;
}


/*
 * message package functions  =============================================================================== start
*/

//message
Msg::Message JoinMsgReqInst(const char * name, const char * ip)
{
    Msg::Message msg;
    msg.set_type(Msg::Join_Request);
    msg.mutable_request()->mutable_join()->set_ip(ip);
    msg.mutable_request()->mutable_join()->set_name(name);
    return msg;
}

Msg::Message JoinMsgReqInst(const string & name, const string & ip)
{
    return JoinMsgReqInst(name.c_str(), ip.c_str());
}

//update status
Msg::Message UpdateStatusMsgReqInst(const char * name, NODE_STATUS status)
{
    Msg::Message msg;
    msg.set_type(Msg::UpdateStatus_Request);
    msg.mutable_request()->mutable_update_status()->set_name(name);
    msg.mutable_request()->mutable_update_status()->set_status(status);
    return msg;
}

Msg::Message UpdateStatusMsgReqInst(const string & name, NODE_STATUS status)
{
    return UpdateStatusMsgReqInst(name.c_str(), status);
}

bool UpdateStateHashMsgReqInst(Msg::Message &msg, const string & name, const string & hash){
    msg.set_type(Msg::UpdateStateHash_Request);
    msg.mutable_request()->mutable_state_hash()->set_name(name);
    msg.mutable_request()->mutable_state_hash()->set_hash(hash);
}

//update state hash
Msg::Message UpdateStateHashMsgReqInst(const char * name, const char * hash){
    Msg::Message msg;
    // msg.set_type(Msg::UpdateStateHash_Request);
    // msg.mutable_request()->mutable_state_hash()->set_name(name);
    // msg.mutable_request()->mutable_state_hash()->set_hash(hash);
    UpdateStateHashMsgReqInst(msg, name, hash);
    return msg;
}

Msg::Message UpdateStateHashMsgReqInst(const string & name, const string & hash){
    return UpdateStateHashMsgReqInst(name.c_str(), hash.c_str());
}

Msg::Message GetStateNodeMsgReqInst(string & name)
{

}


Msg::Message GetStateNodeMsgReqInst(const char * name){
    Msg::Message msg;
    msg.set_type(Msg::GetStateNode_Request);
    msg.mutable_request()->mutable_get_state_node()->set_name(name);
    return msg;
}


bool GetStateNodeMsgResInst(Msg::Message & msg, const char * name, const char * ip, const char * hash){
    cout<<name<<endl;
    cout<<ip<<endl;
    cout<<hash<<endl;
    msg.mutable_response()->set_status(Msg::MSG_RES_OK);
    msg.mutable_response()->set_info("ok");
    msg.mutable_response()->mutable_state_node()->set_name(name);
    msg.mutable_response()->mutable_state_node()->set_hash(hash);
    msg.mutable_response()->mutable_state_node()->set_ip(ip);
    return true;
}

bool GetStateNodeMsgResInst(Msg::Message & msg, const string & name, const string & ip, const string & hash){
    return GetStateNodeMsgResInst(msg, name.c_str(), ip.c_str(), hash.c_str());
}


bool GetStateNodeMsgResErrorInst(Msg::Message &msg, const char * info){
    msg.mutable_response()->set_status(Msg::MSG_RES_OK);
    msg.mutable_response()->set_info(info);
    return true;
}


Msg::Message GetStateNodeMsgResInst(string & name, string &ip, string &hash)
{

}

// common response
Msg::Message CommonMsgResInst(Msg::MsgResStatus status, const char * info)
{
    Msg::Message msg;
    msg.set_type(Msg::Common_Response);
    CommonMsgResInst(msg, status, info);
    return msg;
}

bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const char * info){
    msg.mutable_response()->set_info(info);
    msg.mutable_response()->set_status(status);
    return true;
}


// bool NewFileMsgResInst(Msg::Message & msg, Msg::MsgResStatus status, int postessionId, const char * info){
    
// }


bool NewFileMsgReqInst(Msg::Message &msg, const string &name, Msg::FileType type, int totalPackSize, int totalFileSize){
    msg.set_type(Msg::NewFile_Request);
    msg.mutable_request()->mutable_file()->set_name(name);
    msg.mutable_request()->mutable_file()->set_type(type);
    msg.mutable_request()->mutable_file()->set_total_file_size(totalFileSize);
    msg.mutable_request()->mutable_file()->set_total_pack_size(totalPackSize);
    return true;
}


//
bool NewFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, int postSessionId, const char * info){
    msg.set_type(Msg::NewFile_Response);
    msg.mutable_response()->set_info(info);
    msg.mutable_response()->set_status(status);
    msg.mutable_response()->mutable_new_file_response()->set_post_session_id(postSessionId);
    return true;
}


// file transfer
bool FileChunkPostMsgInst(Msg::Message & msg, const string &name, char buf[], int fileIndx, int packIndx, int dataSize, int postSesionid)
{
    msg.set_type(Msg::File_Post);
    msg.mutable_file_post()->set_name(name);
    msg.mutable_file_post()->set_data(buf, dataSize);
    msg.mutable_file_post()->set_file_idx(fileIndx);
    msg.mutable_file_post()->set_pack_idx(packIndx);
    msg.mutable_file_post()->set_data_size(dataSize);
    msg.mutable_file_post()->set_post_session_id(postSesionid);
    return true;
}


bool RMFileMsgReqInst(Msg::Message &msg, const string &path){
    msg.set_type(Msg::Rm_File_Request);
    msg.mutable_request()->mutable_rm_op()->set_path(path);
    return true;
}


/*
 * message package functions  =============================================================================== end
*/