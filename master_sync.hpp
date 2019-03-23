#pragma once
#include "udp_server.hpp"
#include "node.hpp"
#include <map>
#include <string>
#include "logger.hpp"
#include <time.h>
#include "common.hpp"
#include <vector>
#include "msg.pb.h"
#include <sstream>
#include "defines.hpp"
// sync master
// manage information for multiple nodes

using namespace std;


class MasterSync:UdpServer
{
private:
    Logger logger;
    map<string, Node> nodes;
    vector<string> node_names;
    string state_name;
    string state_hash;
    long long state_timestamp;

public:
    MasterSync();
    ~MasterSync();

    bool initConfig();

    // recvbuf: buffer from client, sendbuf: buffer to be sent
    //
    bool handle(char recvbuf[], int recv_len, char sendbuf[], int &send_len);
    //join
    bool join(const string & name, const string & ip, char sendbuf[], int &send_len);
    // set status
    bool updateStatus(const string & name, NODE_STATUS status, char sendbuf[], int &send_len);
    // 
    bool updateState(const string & name, const string & hash, char sendbuf[], int &send_len);

    bool getStateNode(const string & name, char sendbuf[], int &send_len);

    bool setState(const string &name, const string &hash){
        this->state_name = name;
        this->state_hash = hash;
        this->state_timestamp = getSystemTime();
    }
    bool listen(int port);
   // string getNodes();
};
