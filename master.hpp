#pragma once
#include "udp_server.hpp"
// #include "node.hpp"
#include <map>
#include <string>
#include "logger.hpp"
#include <time.h>
#include "common.hpp"
#include <vector>
#include "msg.pb.h"
#include <sstream>
#include "defines.hpp"

using namespace std;

class Node
{
private:
    NODE_STATUS status;
    string ip;
    string port;
    string name;
public:
    Node(){};
    Node(const string &name, const string &ip, const string &port)
    {
        this->ip = ip;
        this->name = name;
        this->port = port;
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
    long long stateTimestamp;

public:
    Master(){};
    ~Master(){};

    bool initConfig();
    bool initConfig(int argc, char *argv[]);
    // recvbuf: buffer from client, sendbuf: buffer to be sent
    // bool handle(char recvbuf[], int recv_len, char sendbuf[], int &sendLen);
    bool handle(char recvbuf[], int recv_len, char sendbuf[], int &sendLen, bool &isResponse);
    // join 
    bool join(const string & name, const string & ip, const string &port, char sendbuf[], int &sendLen);
    // set status of node
    bool updateStatus(const string & name, NODE_STATUS status, char sendbuf[], int &sendLen);
    // update state
    bool updateState(const string & name, const string & hash, char sendbuf[], int &sendLen);
    // get state of specify node
    bool getStateNode(const string & name, char sendbuf[], int &sendLen);
    // get ip&port master node
    string getMasterNodeConnString(const string &name, char sendbuf[], int &sendLen);
    bool setState(const string &name, const string &hash)
    {
        this->stateName = name;
        this->stateHash = hash;
        this->stateTimestamp = getSystemTime();
    }
    bool listen(int port);
    bool forever(int argc, char *argv[]);
   // string getNodes();
};
