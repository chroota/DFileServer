#pragma once
#include "msg.pb.h"
#include <string>
#include "defines.hpp"

using namespace std;
/*
 * message package functions  =============================================================================== start
*/
// msg package
Msg::Message JoinMsgReqInst(const string &name, const string & ip, const string &port, const string &auth);
bool JoinMsgReqInst(Msg::Message &msg, const string &name, const string & ip, const string &port, const string &auth);
bool JoinMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info, const string &encryptedEcryptKey);

Msg::Message UpdateStatusMsgReqInst(const char * name, NODE_STATUS status);
Msg::Message UpdateStatusMsgReqInst(const string &name, NODE_STATUS status);
Msg::Message UpdateStateMsgReqInst(const string & name, const string & hash, const string &auth);
// bool UpdateStateHashMsgReqInst(Msg::Message &msg, const string & name, const string & hash);
Msg::Message GetStateMsgReqInst(const string & name, const string &auth);
bool GetStateMsgResInst(Msg::Message &resMsg, Msg::MsgResStatus status, const string &info, const string &hash);

// bool GetStateMsgResInst(Msg::Message & msg, Msg::MsgResStatus status, const string & hash);
Msg::Message CommonMsgResInst(Msg::MsgResStatus status, const char * info);
// common response
bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const char * info);
bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string & info);


// create new file, allocate a session id for VvfsTP client
bool NewFileMsgReqInst(Msg::Message &msg, const string &name, Msg::FileType type, int totalPackSize, int totalFileSize);
bool NewFileMsgResInst(Msg::Message & msg, Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message NewFileMsgResInst(Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message GetStateNodeMsgInst();

//file post msg
bool FileChunkPostMsgInst(Msg::Message & msg, const string &name, char buf[], int fileIndx, int packIndx, int dataSize, int postSesionid);

// rm file msg
bool RMFileMsgReqInst(Msg::Message &msg, const string &path);

// ls file
bool LsFileMsgReqInst(Msg::Message &msg, const string &remotePath);
bool LsFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info);
bool AddAttributeToFileMsg(Msg::Message &msg, const string name, int size, Msg::FileType type, const string &time);

//move file
bool MvFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath);

//cp file
bool CpFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath);