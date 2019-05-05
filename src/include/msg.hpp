/*
 * message package functions
*/
#pragma once
#include <msg.pb.h>
#include <string>
#include <defines.hpp>
#include <list>
#include <common.hpp>
using namespace std;



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
bool NewFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message NewFileMsgResInst(Msg::MsgResStatus status, int postessionId, const char * info);
// Msg::Message GetStateNodeMsgInst();

//file post msg
bool FileChunkPostMsgInst(Msg::Message &msg, const string &name, char buf[], int fileIndx, int packIndx, int dataSize, int postSesionId);

// rm file msg
bool RMFileMsgReqInst(Msg::Message &msg, const string &path);

// ls file
bool LsFileMsgReqInst(Msg::Message &msg, const string &remotePath);
bool LsFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info);
bool AddAttributeToLsFileMsg(Msg::Message &msg, const string name, int size, Msg::FileType type, const string &time);

//move file
bool MvFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath);

//cp file
bool CpFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath);

// get file ops
Msg::Message GetFileOpsMsgReqInst(const string &name, const string &newestHash, const string &auth);
bool GetFileOpsMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info);
bool AddOperationToOpsMsg(Msg::Message &msg, Msg::FileOpType type, const string &srcPath, const string &dstPath, const string &hash, time_t opTime);

// get file
Msg::Message GetFileMsgReqInst(const string &name, const string &path, const string &auth);
bool GetFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info, int totalFileSize, const string &sessionKey);
bool GetFileChunkMsgReqInst(Msg::Message &msg, const string &path, int fileIdx, const string &sessionKey);
bool GetFileChunkMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info, char data[], int dataSize);