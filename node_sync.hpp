#pragma once
#include "logger.hpp"
#include <thread>
#include <mutex>
#include <iostream>
#include <unistd.h>
#include "common.hpp"
#include "defines.hpp"
#include "msg.pb.h"
#include <chrono>
#include "node_server.hpp"
#include "vvfs.hpp"

using namespace std;

struct Master{
    string host;
    int port;
};


// udp data server
class UdpDataServer:UdpServer{
    public:
        bool start();
        bool handle(char recvbuf[], int recv_len);
    private:
        map<string, vector<int>> fileRecvStatus;
};

class NodeSync
{
public:
    enum sync_status{
        sync_state,
        sync_file,
        sync_ok
    };

    NodeSync(){};
    ~NodeSync(){};

    // bool join();
    void start();

private:
    void syncStateFn();
    void keepAliveFn();
    void syncFileFn();
    void serverFn();
    bool join();
    bool updateStatus(NODE_STATUS status);
    bool initConfig();
    bool updateState(const string &hash);
    bool getStateNode();
    void checkFileSync();
    bool handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len);
    bool buildMyFs();

    string state_hash;
    Logger logger;
    std::mutex mutex_;
    NODE_STATUS curStatus;
    int aliveSyncTime;
    int checkFileSyncTime;
    //master host
    Master master;
    string syncDir;
    string myName;
    string myStateHash;
    // NodeServer server;
    Vvfs vvfs;
};