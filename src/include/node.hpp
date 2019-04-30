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

using namespace std;

struct FileSaver{
private:
    string savePath;
    Msg::FileType type;
    int totalFileSize;
    int totalPackSize;
    ofstream *pOutput = nullptr;
    int recvChunkCount = 0;
    int recvFileSizeCount = 0;
    vector<int> *pRecvFlag = nullptr;
    Logger logger;
public:
    FileSaver(){};
    FileSaver(const string &savePath, Msg::FileType type, int totalFileSize, int totalPackSize)
    {
        this->type = type;
        this->totalFileSize = totalFileSize;
        this->totalPackSize = totalPackSize;
        this->savePath = savePath;
    };
    ~FileSaver(){
        if(pOutput)
        {
            delete pOutput;
            pOutput = nullptr;
        }
        if(pRecvFlag)
        {
            delete pRecvFlag;
            pRecvFlag = nullptr;
        }
    };

    bool open(string &err);
    bool writeChunk(const string &data, int fileIdx, int packIdx, string &err);
    bool isComplete();
    // Logger logger;
};

// udp data server
class PushFileServer:UdpServer
{
    private:
        // savers
        map<string, FileSaver *> savers;
        map<string, int> sessions;
        Logger logger;
        Vvfs *pVvfs = nullptr;

        int getSessionId()
        {
            //todo check allocated
            srand((unsigned int)(time(NULL)));
            return rand()/10000;
        }
    public:
        // null function
        bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool & isResponse);
        //create new file
        bool createNewFile(const string &name, Msg::FileType type, int totalFileSize, int totalPackSize, char sendbuf[], int &sendLen);
        //recieve data chunk
        bool recvChunk(const string &name, int sessionId, int fileIdx, int packIdx, const string & data, char sendbuf[], int &sendLen);
        //delete file
        bool rmFile(const string &path, char sendbuf[], int &sendLen);
        // ls files
        bool lsFiles(const string &path, char sendbuf[], int &sendLen);
        // move file
        bool mvFile(const string &srcPath, const string & dstPath, char sendbuf[], int &sendLen);
        //cp file
        bool cpFile(const string &srcPath, const string & dstPath, char sendbuf[], int &sendLen);
        //listen a port
        bool listen(int port);
        bool test();

        PushFileServer(Vvfs *pVvfs)
        {
            this->pVvfs = pVvfs;
        };
        ~PushFileServer(){
            map<string, FileSaver *>::iterator it = savers.begin();

            while(it != savers.end())
            {
                if (it->second) delete it->second;
                it++;         
            }
        };
};

class Node
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
    bool getStateNode();
    void checkFileSync();
    bool handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len);
    bool createVFS();
    bool createFileServer();
    string encryptKey;
    int aliveSyncTime;
    int checkFileSyncTime;
    bool isStateNode = false;
    string masterIp, nodeIp;
    int masterPort, nodePort, pushFileServerPort, pullFileServerPort;
    string keypairDir;
    string privkey;
    string pubkey;

    string state_hash;
    Logger logger;
    std::mutex mutex_;
    NODE_STATUS curStatus;
    //master host
    string syncDir;
    string name;
    string myStateHash;
    // NodeServer server;
    Vvfs *pVvfs = nullptr;
    PushFileServer *pPushFileServer = nullptr;
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