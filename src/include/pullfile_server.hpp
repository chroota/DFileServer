#pragma once
#include <logger.hpp>
#include <thread>
#include <mutex>
#include <iostream>
#include <chrono>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <sys/stat.h>
#include <common.hpp>
#include <defines.hpp>
#include <msg.hpp>
#include <udp_server.hpp>
#include <vvfs.hpp>

using namespace std;
struct GetFileSession
{
    string name;
    string path;
    string pubkey;

    GetFileSession(const string name, const string &path, const string &pubkey)
    {
        this->name = name;
        this->path = path;
        this->pubkey = pubkey;
    }
    ~GetFileSession();
    //todo other info
};

class PullFileServer:UdpServer
{
private:
    Logger logger;
    bool getFileResponse(Msg::Message &recvMsg, Msg::Message &resMsg);
    bool getFileChunkResponse(Msg::Message &recvMsg, Msg::Message &resMsg);

    string getSessionKey()
    {
        //todo check allocated
        return getRandomKey();
    }

    /*
     * sessions for client info
    */
    map<string, GetFileSession> sessions;
    bool sessionPut(const string &key, const string &name, const string &path, const string &pubkey);
    bool isSessionExist(const string &key);
    Vvfs *pVvfs = nullptr;

public:
    PullFileServer(){};
    PullFileServer(Vvfs *pVvfs)
    {
        this->pVvfs = pVvfs;
    };
    ~PullFileServer(){};

    bool listen(int port);
    bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool &isResponse);
};
