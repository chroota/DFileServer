#pragma once
#include <stdio.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "defines.hpp"
#include "msg.pb.h"
#include "md5.hpp"
#include <memory.h>
#include <time.h>
#include <sstream>

using namespace std;


/*
 * time
*/
struct timespec getTimeSpec();
long long getSystemTime();

/*
 * string
*/
void ssplit(const string& s, vector<string>& sv, const char flag = ' ');

/*
 *   md5 
*/
string getBufMD5(const void * buf, int len);
string getMD5string(unsigned char buf[], int len);

bool urequest(const char * host, int port, const char sendbuf[], int send_len, char recvbuf[], int & recv_len);
bool urequest(const string & host, int port, const char sendbuf[], int send_len, char recvbuf[], int & recv_len);
bool urequestNoResponse(const char * host, int port, const char sendbuf[], int send_len);

// msg package
Msg::Message JoinMsgReqInst(const char * name, const char * ip);
Msg::Message JoinMsgReqInst(const string &name, const string & ip);
Msg::Message UpdateStatusMsgReqInst(const char * name, NODE_STATUS status);
Msg::Message UpdateStatusMsgReqInst(const string &name, NODE_STATUS status);
Msg::Message UpdateStateHashMsgReqInst(const char * name, const char * hash);
Msg::Message UpdateStateHashMsgReqInst(const string & name, const string & hash);
bool UpdateStateHashMsgReqInst(Msg::Message &msg, const string & name, const string & hash);
Msg::Message GetStateNodeMsgReqInst(const char * name);

// Msg::Message GetStateNodeMsgResInst(const char * name, const char * ip, const char * hash);
bool GetStateNodeMsgResInst(Msg::Message & msg, const char * name, const char * ip, const char * hash);
bool GetStateNodeMsgResInst(Msg::Message & msg, const string & name, const string & ip, const string & hash);
bool GetStateNodeMsgResErrorInst(Msg::Message & msg, const char * info);
Msg::Message CommonMsgResInst(Msg::MsgResStatus status, const char * info);
// common response
bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const char * info);


// create new file, allocate a session id for VvfsTP client
bool NewFileMsgReqInst(Msg::Message &msg, const string &name, Msg::FileType type, int totalPackSize, int totalFileSize);
bool NewFileMsgResInst(Msg::Message & msg, Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message NewFileMsgResInst(Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message GetStateNodeMsgInst();

//file post msg
bool FileChunkPostMsgInst(Msg::Message & msg, const string &name, char buf[], int fileIndx, int packIndx, int dataSize, int postSesionid);

// rm file msg
bool RMFileMsgReqInst(Msg::Message &msg, const string &path);