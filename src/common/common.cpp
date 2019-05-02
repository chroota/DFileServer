#include "common.hpp"

string getTimeStringFromTvSec(int tv_sec)
{
    struct tm *t;
    time_t tt = tv_sec;
    t = localtime(&tt);
    char buf[MAXBUFSIZE];
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
    return string(buf);
}

struct timespec getTimeSpec()
{
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

long long getSystemTime()
{
	struct timeb t;
	ftime(&t);
	return 1000*t.time + t.millitm;
}

void ssplit(const string& s, vector<string>& sv, const char flag) 
{
    sv.clear();
    istringstream iss(s);
    string temp;

    while (getline(iss, temp, flag)) {
        if(temp.size() == 0) continue;
        sv.push_back(temp);
    }
    return;
}

// 
string getBufMD5(const void * buf, int len)
{
    MD5_CTX md5Context;
    MD5Init(&md5Context);
    MD5Update(&md5Context, (unsigned char *)buf, len);
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5Final(&md5Context, result);
    return getMD5string(result, MD5_DIGEST_LENGTH);
}

string getMD5string(unsigned char buf[], int len = MD5_DIGEST_LENGTH)
{
    char hex[35];
    memset(hex, 0, sizeof(hex));
    for (int i = 0; i < len; ++i)
    {
        sprintf(hex + i * 2, "%02x", buf[i]);
    }
    hex[32] = '\0';
    return string(hex);
}


// udp client
bool urequest(const char * host, int port, const char sendbuf[], int send_len, char recvbuf[], int &recv_len)
{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) return false;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    socklen_t len = sizeof(addr);
    int sentLen = sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&addr, len);
    // cout<<sentLen<<endl;
    // todo resend & time limit to send
    if(sentLen < 0){
        return false;
    }
    recv_len = recvfrom(sockfd, recvbuf, MAXMSGSIZE, 0, (struct sockaddr*)&addr,&len);
    close(sockfd);
	return true;
}


bool urequest(const string  & host, int port,const char sendbuf[], int send_len, char recvbuf[], int &recv_len)
{
    return urequest(host.c_str(), port, sendbuf, send_len, recvbuf, recv_len);
}

bool urequestNoResponse(const char * host, int port, const char sendbuf[], int send_len)
{
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) return false;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    socklen_t len = sizeof(addr);
    if(sendto(sockfd, sendbuf, send_len, 0, (struct sockaddr*)&addr,len) < 0){
        return false;
    }
    close(sockfd);
    return true;
}

/*
 * file
*/
bool mkdirIfNotExist(const string &path, string &err)
{
    struct stat statbuf;
    stat(path.c_str(), &statbuf);
    if(stat(path.c_str(), &statbuf) == -1)
    {
        if(mkdir(path.c_str(),  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            err = "mkdir err";
            return false;
        }
        return true;
    }
    if(!S_ISDIR(statbuf.st_mode))
    {
        err = "target path is not a dir";
        return false;
    }
    return true;
}

/*
 * encrypt
*/
// generate rsa key pair
void generateRSAKey(string strKey[2], const string & pubKeyFile, const string &privkeyFile)
{    
    size_t pri_len;  
    size_t pub_len;  
    char *pri_key = NULL;  
    char *pub_key = NULL;  
    
    RSA *keypair = RSA_generate_key(KEY_LENGTH, RSA_3, NULL, NULL);  
  
    BIO *pri = BIO_new(BIO_s_mem());  
    BIO *pub = BIO_new(BIO_s_mem());  
  
    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);  
    PEM_write_bio_RSAPublicKey(pub, keypair);  
      
    pri_len = BIO_pending(pri);  
    pub_len = BIO_pending(pub);  
   
    pri_key = (char *)malloc(pri_len + 1);  
    pub_key = (char *)malloc(pub_len + 1);
  
    BIO_read(pri, pri_key, pri_len);  
    BIO_read(pub, pub_key, pub_len);  
  
    pri_key[pri_len] = '\0';  
    pub_key[pub_len] = '\0';  
    
    strKey[0] = pub_key;  
    strKey[1] = pri_key;  
  
    FILE *pubFile = fopen(pubKeyFile.c_str(), "w");  
    if (pubFile == NULL)  
    {  
        assert(false);  
        return;  
    }  
    fputs(pub_key, pubFile);  
    fclose(pubFile);  
  
    FILE *priFile = fopen(privkeyFile.c_str(), "w");  
    if (priFile == NULL)  
    {  
        assert(false);  
        return;  
    }  
    fputs(pri_key, priFile);  
    fclose(priFile);
  
    RSA_free(keypair);  
    BIO_free_all(pub); 
    BIO_free_all(pri);
  
    free(pri_key);  
    free(pub_key);  
}

// encrypt by public key
string rsaEncryptByPubkey(const string &clearText, const string &pubKey)  
{  
    string strRet;
    RSA *rsa = NULL;  
    BIO *keybio = BIO_new_mem_buf((unsigned char *)pubKey.c_str(), -1);  

    RSA* pRSAPublicKey = RSA_new();  
    rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);  
  
    int len = RSA_size(rsa);  
    char *encryptedText = (char *)malloc(len + 1);  
    memset(encryptedText, 0, len + 1);  
  
    int ret = RSA_public_encrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)  
        strRet = string(encryptedText, ret);  
    
    free(encryptedText);  
    BIO_free_all(keybio);  
    RSA_free(rsa);  
  
    return strRet;  
}

// decrypt 
string rsaDecryptByPrivkey(const string &cipherText, const string &privKey)  
{  
    string strRet;
    RSA *rsa = RSA_new();  
    BIO *keybio;  
    keybio = BIO_new_mem_buf((unsigned char *)privKey.c_str(), -1);  
  
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);  
  
    int len = RSA_size(rsa);  
    char *decryptedText = (char *)malloc(len + 1);  
    memset(decryptedText, 0, len + 1);  
  
  
    int ret = RSA_private_decrypt(cipherText.length(), (const unsigned char*)cipherText.c_str(), (unsigned char*)decryptedText, rsa, RSA_PKCS1_PADDING);  
    if (ret >= 0)  
        strRet = std::string(decryptedText, ret);  
    
    free(decryptedText);
    BIO_free_all(keybio);  
    RSA_free(rsa);
    return strRet;  
}


// encrypt by private key
string rsaEncryptByPrivkey(const string &clearText, const string &privKey)  
{  
    string strRet;
    RSA *rsa = NULL;  
    BIO *keybio = BIO_new_mem_buf((unsigned char *)privKey.c_str(), -1);
    RSA* pRSAPrivateKey = RSA_new();
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    if(!rsa)
    {
        BIO_free_all(keybio);
        return "";
    }
  
    int len = RSA_size(rsa);
    char *encryptedText = (char *)malloc(len + 1);
    memset(encryptedText, 0, len + 1);
  
    int ret = RSA_private_encrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)
        strRet = string(encryptedText, ret);
   
    free(encryptedText);
    BIO_free_all(keybio);
    RSA_free(rsa);
    return strRet;
}

// decrypt by public key
string rsaDecryptByPubkey(const string &cipherText, const string &pubkey)  
{  
    string strRet;
    RSA *rsa = RSA_new();  
    BIO *keybio;  
    keybio = BIO_new_mem_buf((unsigned char *)pubkey.c_str(), -1);   
    rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);  
  
    int len = RSA_size(rsa);  
    char *decryptedText = (char *)malloc(len + 1);  
    memset(decryptedText, 0, len + 1);  
   
    int ret = RSA_public_decrypt(cipherText.length(), (const unsigned char*)cipherText.c_str(), (unsigned char*)decryptedText, rsa, RSA_PKCS1_PADDING);  
    if (ret >= 0)  
        strRet = std::string(decryptedText, ret);  
  
    free(decryptedText);  
    BIO_free_all(keybio);  
    RSA_free(rsa);
    return strRet;  
}

// inspect rsa key pair
bool inspectRsaKeyPair(const string &pubkey, const string &privkey)
{
    string testText = "plain text for test";
    string encryptText = rsaEncryptByPrivkey(testText, privkey);
    string decryptText = rsaDecryptByPubkey(encryptText, pubkey);
    return testText == decryptText;
}

/*
 * get random key for encryption
*/
string getRandomKey(int size)
{
    const char set[] = "_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    srand((unsigned)time(NULL));
    char buf[size + 1] = {0};
    for (int i = 0; i < size; ++i)
    {
        int x = rand() / (RAND_MAX / (sizeof(set) - 1));
        buf[i] = set[x];
    }
    return string(buf);
}

/*
 * linux sig action
*/

void onSIGSEGV(int signum, siginfo_t *info, void *ptr)
{
    int j, nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;
    nptrs = backtrace(buffer, BT_BUF_SIZE);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
        would produce similar output to the following: */
    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
            perror("backtrace_symbols");
            exit(EXIT_FAILURE);
    }       
    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);
    // todo

    free(strings);
    abort();
}

bool catchSegmentFaultError()
{
    struct sigaction action;
    int sig = SIGSEGV;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = onSIGSEGV;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(sig, &action, NULL) < 0)
        perror("SIGEGV sigaction register error:");
    return true;
}