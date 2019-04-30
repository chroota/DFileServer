#pragma once
// #include <openssl/md5.h>
#include <memory.h>
#include <string>
#include "md5.hpp"
#include "common.hpp"

using namespace std;

class MD5pkg
{
private:
    MD5_CTX md5Context;
public:
    MD5pkg(){
        MD5Init(&md5Context);
    };
    ~MD5pkg(){};
    bool update(const void *data, size_t len)
    {
        MD5Update(&md5Context, (unsigned char *)data, len);
    }
    string getMD5Hex()
    {
        unsigned char result[MD5_DIGEST_LENGTH];
        MD5Final(&md5Context, result);
        return getMD5string(result, MD5_DIGEST_LENGTH);
    }
};
