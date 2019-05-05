#pragma once
#include <stdio.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "defines.hpp"
#include "msg.pb.h"
#include "md5.hpp"
#include <memory.h>
#include <time.h>
#include <sstream>
#include <cassert>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <execinfo.h>
#include<sys/ucontext.h> 
using namespace std;

/*
 * time
*/
struct timespec getTimeSpec();
time_t getSystemTime();
long long getSystemLongTime();
string getTimeStringFromTvSec(int tv_sec);

/*
 * string
*/
void ssplit(const string& s, vector<string>& sv, const char flag = ' ');


/*
 * udp connection 
*/
bool urequest(const char * host, int port, const char sendbuf[], int send_len, char recvbuf[], int & recv_len);
bool urequest(const string & host, int port, const char sendbuf[], int send_len, char recvbuf[], int & recv_len);
bool urequestNoResponse(const char * host, int port, const char sendbuf[], int send_len);

/*
 * fs
*/
bool mkdirIfNotExist(const string &path, string &err);

/*
 * encrypt
*/
//rsa
#define KEY_LENGTH  2048                  // 密钥长度
void generateRSAKey(string strKey[2], const string & pubKeyFile, const string &privkeyFile);
string rsaEncryptByPubkey(const string &clearText, const string &pubKey);
string rsaDecryptByPrivkey(const string &cipherText, const string &privKey);
string rsaEncryptByPrivkey(const string &clearText, const string &privKey);
string rsaDecryptByPubkey(const string &cipherText, const string &pubkey);
bool inspectRsaKeyPair(const string &pubkey, const string &privkey);
// md5
string getBufMD5(const void * buf, int len);
string getMD5string(unsigned char buf[], int len);

// random key
string getRandomKey(int size = 7);

/*
 * linux sig action
*/
#define BT_BUF_SIZE 10
void onSIGSEGV(int signum, siginfo_t *info, void *ptr);
bool catchSegmentFaultError();