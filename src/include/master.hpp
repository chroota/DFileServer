#pragma once
#include "udp_server.hpp"
// #include "node.hpp"
#include <map>
#include <string>
#include "logger.hpp"
#include <time.h>
#include "common.hpp"
#include <vector>
#include <sstream>
#include "defines.hpp"
#include "msg.hpp"

// state node
#define DEFAULT_STATE_NODE "node1"
using namespace std;
class Node
{
private:
    NODE_STATUS status;
    string ip;
    string port;
    string name;
    string datakey;
    string pubkey;
public:
    Node(){};
    Node(const string &name, const string &ip, const string &port, const string &datakey)
    {
        this->ip = ip;
        this->name = name;
        this->port = port;
        this->datakey = datakey;
        status = NODE_STATUS_CREATE;
    }
    ~Node(){};

    string getName()
    {
        return name;
    };

    string getIp()
    {
        return ip;
    }

    string getConnstring()
    {
        return ip + ":" + port;
    }

    bool setPubkey(const string &pubkey)
    {
        this->pubkey = pubkey;
        return true;
    }
    
    string getPubkey()
    {
        return pubkey;
    }

    int getStatus();

    bool changeStatus(NODE_STATUS status)
    {
        this->status = status;
    }
};

class Master:UdpServer
{
private:
    Logger logger;
    map<string, Node> nodes;
    vector<string> nodeNames;
    string stateName;
    string stateHash;
    string listenIp;
    string clusterIp;
    int listenPort;
    long long stateTimeStamp;
    string keypairDir;

public:
    Master(){};
    ~Master(){};

    bool initConfig();
    bool initConfig(int argc, char *argv[]);
    // recvbuf: buffer from client, sendbuf: buffer to be sent
    bool handle(char recvbuf[], int recvLen, char sendbuf[], int &sendLen, bool &isResponse);
    // join 
    bool join(Msg::Message & resMsg, const string & name, const string & ip, const string &port, const string &auth);
    // set status of node
    bool updateStatus(Msg::Message & resMsg, const string & name, NODE_STATUS status);
    // update state
    bool updateState(Msg::Message &resMsg, const string & name, const string & hash, const string &auth);
    // get state of specify node
    bool getStateNode(Msg::Message &resMsg, const string & names);
    // get ip&port master node
    string getMasterNodeConnString(const string &name, char sendbuf[], int &sendLen);
    bool setState(const string &hash)
    {
        this->stateHash = hash;
        this->stateTimeStamp = getSystemTime();
        return true;
    }
    bool listen(int port);
    bool forever(int argc, char *argv[]);
   // string getNodes();
};
