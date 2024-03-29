#pragma once
#include "logger.hpp"
#include <thread>
#include <mutex>
#include <iostream>
#include <chrono>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <common.hpp>
#include <defines.hpp>
#include <msg.hpp>
// #include "node_server.hpp"
#include <udp_server.hpp>
#include <vvfs.hpp>
#include <pushfile_server.hpp>
#include <pullfile_server.hpp>
using namespace std;

struct NodeData
{
public:
    string name;
    string datakey;
    string pubkey;
    string stateHash;
    NodeData(){};
    NodeData(const string &name, const string &pubkey, const string &datakey)
    {
        this->name    = name;
        this->pubkey  = pubkey;
        this->datakey = datakey;
    };
    ~NodeData(){};
};

class Node:UdpServer
{
private:
    void syncStateFn();
    void keepAliveFn();
    void syncFileFn();
    void serverFn();
    bool join();
    bool updateStatus(NODE_STATUS status);
    bool initConfig(int argc, char *argv[]);
    bool updateState(const string &hash);
    void checkFileSync();
    bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen);
    bool createVfs();

    bool isMaster();
    string encryptKey;
    int aliveSyncTime;
    bool isStateNode = false;
    string masterIp, nodeIp;
    string masterNode;
    int masterPort, nodePort, pushFileServerPort, pullFileServerPort;
    string keypairDir;
    string privkey;
    string pubkey;
    string stateHash;
    void help();
    Logger logger;
    std::mutex mutex_;
    //master host
    string vfsPath;
    string myName;
    // NodeServer server;
    Vvfs *pVvfs = nullptr;

    /*
     * push file server
    */
    PushFileServer *pPushFileServer = nullptr;
    bool createPushFileServer();
    // backend file server
    void backendPushFileServerFunc();

    /*
     * pull file server
    */
    PullFileServer *pPullFileServer = nullptr;
    bool createPullFileServer();
    void backendPullFileServerFunc();

    string getPubkeyByNodeName(const string &name, string &err);
    /*
     * cache node for
    */
    map<string, NodeData> nodesDataCache;
    bool cacheNodeData(const string &name, const string &pubkey);
    NodeData *getNodeDataFromcache(const string &name, string &err);
    
    /*
     * authenticate
    */    
    string myAuth;
    string encryptByMyPrivKey(const string &data);
    string decryptByMyPrivKey(const string &data);
    string encryptByOtherNodePubKey(const string & name, const string &data);
    string decryptByOtherNodePubKey(const string & name, const string &data);
    bool authenticateNode(const string &name, const string &auth, string &err);
    string getEncryptedAuthkey();   

    /*
     * state server & response
    */
    bool listen(int port);
    bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool &isResponse);
    bool responseState(Msg::Message &recvMsg, Msg::Message &resMsg);
    void backendStateServerFunc();
    bool responseFileOps(Msg::Message &recvMsg, Msg::Message &resMsg);


   /*
    * backend request state for sync Vvfs
   */
  int syncStateInterval;
  string requestState();
  void backendSyncVvfsFunc();
  bool requestUpdateFileOps();
  // sync operation from state node
  bool syncVfOperation(Msg::FileOpType type, const string &srcPath, const string &dstPath, const string &hash, time_t opTime);
  bool syncFileFromOtherNode(const string &path, const string &ip, int port, string &err);

public:
    enum sync_status
    {
        sync_state,
        sync_file,
        sync_ok
    };

    Node(){};
    ~Node()
    {
        if(pPushFileServer) 
        {
            delete pPushFileServer;
            pPushFileServer = nullptr;
        }

        if(pVvfs)
        {
            delete pVvfs;
            pVvfs = nullptr;
        }
    };

    // bool join();
    void forever(int argc, char *argv[]);
};