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
#include "common.hpp"
#include "defines.hpp"
#include "msg.hpp"
// #include "node_server.hpp"
#include "udp_server.hpp"
#include "vvfs.hpp"
#include <pushfile_server.hpp>

using namespace std;

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
    bool handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len);
    bool createVFS();
    bool createFileServer();
    bool isMaster();
    string encryptKey;
    int aliveSyncTime;
    int checkFileSyncTime;
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
    string syncDir;
    string name;
    // NodeServer server;
    Vvfs *pVvfs = nullptr;
    PushFileServer *pPushFileServer = nullptr;
    
    /*
     * state request
    */
    bool getStateRequest();

    /*
     * state server & response
    */
    bool listen(int port);
    // handle function
    bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse);
    bool getStateResponse(Msg::Message & recvMsg, Msg::Message & resMsg);

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