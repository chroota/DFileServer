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
#include "msg.pb.h"
#include "node_server.hpp"
#include "vvfs.hpp"

using namespace std;

struct FileSaver{
private:
    //FILE *fp;
    string savePath;
    Msg::FileType type;
    int totalFileSize;
    int totalPackSize;
    ofstream *pOutput = nullptr;
    int recvChunkCount = 0;
    vector<int> *pRecvFlag = nullptr;
public:
    FileSaver(){};
    FileSaver(const string &savePath, Msg::FileType type, int totalFileSize, int totalPackSize){
        this->type = type;
        this->totalFileSize = totalFileSize;
        this->totalPackSize = totalPackSize;
        this->savePath = savePath;
        // cout<<"saver init success"<<endl;
    };
    ~FileSaver(){
        if(pOutput){
            delete pOutput;
        }
        if(pRecvFlag){
            delete pRecvFlag;
        }
    };

    bool open(string &err);

    bool writeChunk(const string &data, int fileIdx, int packIdx, string &err);

    bool isComplete(){
        return recvChunkCount == totalPackSize;
    }
};

// udp data server
class UdpFileServer:UdpServer{
    public:
        UdpFileServer(Vvfs *pVvfs){
            this->pVvfs = pVvfs;
            //pVvfs->vfrsTest();
            //cout<<"vfrs test"<<endl;
        };
        ~UdpFileServer(){};
        // null function
        bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse);
        bool createNewFile(const string &name, Msg::FileType type, int totalFileSize, int totalPackSize, char sendbuf[], int &sendLen);
        bool recvChunk(const string &name, int sessionId, int fileIdx, int packIdx, const string & data);
        bool listen(int port);
        bool test();
    private:
        // savers
        map<string, FileSaver> savers;
        map<string, int> sessions;
        Logger logger;
        Vvfs *pVvfs;

        int getSessionId(){
            srand((unsigned int)(time(NULL)));
            return rand()/10000;
        }
};


struct Master{
    string host;
    int port;
};

class NodeSync
{
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
    bool createVFS();
    bool createFileServer();

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
    Vvfs *pVvfs = nullptr;
    UdpFileServer *pFileServer = nullptr;

public:
    enum sync_status{
        sync_state,
        sync_file,
        sync_ok
    };

    NodeSync(){};
    ~NodeSync(){
        if(pFileServer) delete pFileServer;
        if(pVvfs) delete pVvfs;
    };

    // bool join();
    void start();
};