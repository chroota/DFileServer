#include "msg.hpp"

/*
 * message package functions  =============================================================================== start
*/
Msg::Message JoinMsgReqInst(const string & name, const string & ip, const string &port, const string &auth)
{
    Msg::Message msg;
    msg.set_type(Msg::Join_Request);
    msg.mutable_request()->mutable_join()->set_ip(ip);
    msg.mutable_request()->mutable_join()->set_name(name);
    msg.mutable_request()->mutable_join()->set_port(port);
    msg.mutable_request()->mutable_join()->set_auth(auth);
    return msg;
}

bool JoinMsgReqInst(Msg::Message &msg, const string &name, const string & ip, const string &port, const string &auth)
{
    msg.set_type(Msg::Join_Request);
    msg.mutable_request()->mutable_join()->set_ip(ip);
    msg.mutable_request()->mutable_join()->set_name(name);
    msg.mutable_request()->mutable_join()->set_port(port);
    msg.mutable_request()->mutable_join()->set_auth(auth);
    return true;
}

bool JoinMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info, const string &encryptedEcryptKey)
{
    msg.set_type(Msg::Join_Response);
    msg.mutable_response()->set_status(status);
    msg.mutable_response()->set_info(info);
    msg.mutable_response()->mutable_join_res()->set_encrypedencryptkey(encryptedEcryptKey);
    return true;
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

Msg::Message UpdateStateMsgReqInst(const string & name, const string & hash, const string &auth)
{
    Msg::Message msg;
    msg.set_type(Msg::UpdateState_Request);
    msg.mutable_request()->mutable_state()->set_name(name);
    msg.mutable_request()->mutable_state()->set_hash(hash);
    msg.mutable_request()->mutable_state()->set_auth(auth);
    return msg;
}

Msg::Message GetStateMsgReqInst(const string & name, const string &auth)
{
    Msg::Message msg;
    msg.set_type(Msg::GetState_Request);
    // msg.mutable_request()->mutable_get_state_node()->set_name(name);
    msg.mutable_request()->mutable_get_state_req()->set_name(name);
    msg.mutable_request()->mutable_get_state_req()->set_auth(auth);
    return msg;
}


// bool GetStateMsgResInst(Msg::Message & msg, const string & hash)
// {
//     msg.mutable_response()->set_status(Msg::MSG_RES_OK);
//     msg.mutable_response()->set_info("ok");
//     // msg.mutable_response()->mutable_state_node()->set_name(name);
//     // msg.mutable_response()->mutable_state_node()->set_hash(hash);
//     // msg.mutable_response()->mutable_state_node()->set_conn_string(connString);
//     msg.mutable_response()->mutable_get_state_res()->set_hash(hash);
//     return true;
// }


// bool GetStateNodeMsgResErrorInst(Msg::Message &msg, const char * info)
// {
//     msg.mutable_response()->set_status(Msg::MSG_RES_OK);
//     msg.mutable_response()->set_info(info);
//     return true;
// }


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

bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const char * info)
{
    msg.set_type(Msg::Common_Response);
    msg.mutable_response()->set_info(info);
    msg.mutable_response()->set_status(status);
    return true;
}

bool CommonMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string & info)
{
    return CommonMsgResInst(msg, status, info.c_str());
}


// bool NewFileMsgResInst(Msg::Message & msg, Msg::MsgResStatus status, int postessionId, const char * info){
    
// }


bool NewFileMsgReqInst(Msg::Message &msg, const string &name, Msg::FileType type, int totalPackSize, int totalFileSize)
{
    msg.set_type(Msg::NewFile_Request);
    msg.mutable_request()->mutable_file()->set_name(name);
    msg.mutable_request()->mutable_file()->set_type(type);
    msg.mutable_request()->mutable_file()->set_total_file_size(totalFileSize);
    msg.mutable_request()->mutable_file()->set_total_pack_size(totalPackSize);
    return true;
}


//
bool NewFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, int postSessionId, const char * info)
{
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



// ls files
bool LsFileMsgReqInst(Msg::Message &msg, const string &remotePath){
    msg.set_type(Msg::LsFile_Request);
    msg.mutable_request()->mutable_ls_file_req()->set_path(remotePath);
    return true;
}

bool LsFileMsgResInst(Msg::Message &msg, Msg::MsgResStatus status, const string &info){
    msg.set_type(Msg::LsFile_Response);
    msg.mutable_response()->set_info(info);
    msg.mutable_response()->set_status(status);
    return true;
}

bool AddAttributeToFileMsg(Msg::Message &msg, const string name, int size, Msg::FileType type, const string &time)
{
    Msg::FileAttribute *fattr;
    fattr = msg.mutable_response()->mutable_ls_file_res()->add_files();
    fattr->set_name(name);
    fattr->set_size(size);
    fattr->set_type(type);
    fattr->set_time(time);
    return true;
}

bool MvFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath)
{
    msg.set_type(Msg::MvFile_Request);
    msg.mutable_request()->mutable_mv_file_req()->set_srcpath(srcPath);
    msg.mutable_request()->mutable_mv_file_req()->set_dstpath(dstPath);
    return true;
}

bool CpFileMsgReqInst(Msg::Message &msg, const string &srcPath, const string &dstPath)
{
    msg.set_type(Msg::CpFile_Request);
    msg.mutable_request()->mutable_cp_file_req()->set_srcpath(srcPath);
    msg.mutable_request()->mutable_cp_file_req()->set_dstpath(dstPath);
    return true;
}

/*
 * message package functions  =============================================================================== end =====================================================
*/