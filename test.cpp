// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <string>
// #include <iostream>

// class father
// {
// private:
// 	/* data */
// public:
// 	father(/* args */);
// 	~father();
// 	virtual void handle()=0;
// };

// father::father(/* args */)
// {
// }

// father::~father()
// {
// }

// class son:father
// {
// private:
// 	/* data */
// public:
// 	son(/* args */);
// 	~son();

// 	void handle(){
// 		printf("son son ....");
// 	}
// };

// son::son(/* args */)
// {
// }

// son::~son()
// {
// }



// int main(){
// //	perror("fuck!!\n");
// //	exit(-1);
// 	//printf("test");
// 	// son s;
// 	// s.handle();

// 	// char buf[500];
// 	// char *x = buf;

// 	char str[100] = "123--141";
// 	//char *str2 = "123--141";
// 	//printf("%d\n", str==str2);

// 	//return 1;
// 	//char *pstr = strsep((char **) &str, "-");

// 	// char *q = str;
// 	// char *p = strsep(&q, "--");

// 	std::string s(str, str+5);
// 	std::cout<<s<<std::endl;


// 	//puts(p);
// 	//puts(p+1);

// 	//printf("%d\n", pstr);
// }

// #include <fstream>
// #include <openssl/md5.h>
// #include <string>
// #include <stdlib.h>
// #include <memory.h>
// using std::string;

// int get_file_md5(const std::string &file_name, std::string &md5_value)
// {
//     md5_value.clear();

//     std::ifstream file(file_name.c_str(), std::ifstream::binary);
//     if (!file)
//     {
//         return -1;
//     }

//     MD5_CTX md5Context;
//     MD5_Init(&md5Context);

//     char buf[1024 * 16];
//     while (file.good()) {
//         file.read(buf, sizeof(buf));
//         MD5_Update(&md5Context, buf, file.gcount());
//     }

//     unsigned char result[MD5_DIGEST_LENGTH];
//     MD5_Final(result, &md5Context);

//     char hex[35];
//     memset(hex, 0, sizeof(hex));
//     for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
//     {
//         sprintf(hex + i * 2, "%02x", result[i]);
//     }
//     hex[32] = '\0';
//     md5_value = string(hex);

//     return 0;
// }

// int main(int argc, char* argv[])
// {
//     string file_name = "./t.txt";
//     string md5value;
//     int ret = get_file_md5(file_name, md5value);
//     if (ret < 0){
// 	  printf("get file md5 failed. file=%s\n", file_name.c_str());
// 	  return -1;
// 	}
// 	printf("%d\n", md5value.size());
//     printf("the md5value=%s\n", md5value.c_str());
// }


// #include<stdio.h>
// #include<stdlib.h>
// #include<string.h>
// #include <time.h>
 
 
 
// int main(void)
// {
 
// 	time_t t;
// 	t = time(NULL);
 
// 	int ii = time(&t);
// 	printf("ii = %d\n", ii);
// 	system("pause");
// 	return 0;
// 	/*
// 	ii = 1516020076
// 	请按任意键继续. . .
// 	*/
// }


// #include <stdio.h>
// #include <sys/timeb.h>
// #include <sys/types.h>

// long long getSystemTime()
// {
// 	struct timeb t;
	
// 	ftime(&t);
	
// 	return 1000*t.time + t.millitm;
// }

// int main()
// {
// 	long long start = getSystemTime();
	
// 	printf("start = %lld\n",start);
	
// 	return 0;
// }

// #include <string>
// #include <iostream>
// using namespace std;

// int main(){
// 	string s;
// 	s.push_back(67);
// 	cout<<s.length()<<endl;
// }

// #include <stdio.h>

// int main(){
//     char name[100];
//     int idx;
//     int first_bro_idx;
//     int fa_idx;
//     int son_idx;
//     FILE *fp = fopen("./vfr.txt", "r");
//     while(!feof(fp)){
//         fscanf(fp, "%d %s %d %d %d", &idx, name, &fa_idx, &first_bro_idx, &son_idx);
//         printf("%d %s %d %d %d\n", idx, name, fa_idx, first_bro_idx, son_idx);
//     }
// }


/*
#include <time.h> 
#include <memory.h> 
#include <stdio.h>

int main()
{
    time_t t;
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("%d----->%d\n", ts.tv_sec, ts.tv_nsec);
    return 0;
}
*/

//例子1：以二进制模式打开并写入文件
// #include <stdio.h>
// #include <iostream>
// #include <fstream>
// #include <stdlib.h>
// #include <time.h>
// using namespace std;


// void copyFile(const string & srcPath, const string &dstPath){
//     // ifstream input(srcPath.c_str(), ios::binary);
//     ofstream output(dstPath.c_str(), ios::binary);
//     int num = 10;
//     srand(time(NULL));
//     while(num-- >= 0){
//         //cout<<rand()%255<<" ";
//         char v = rand()%255;
//         cout<<(short)v<<" ";
//         output.write(&v, 1);
//     }
//     cout<<" "<<endl;
//     output.close();
//     //return ;
//     ifstream input(dstPath.c_str(), ios::binary);
//     char buf[1];
//     while(!input.eof()){
//         input.read(buf, 2);
//         cout<<input.tellg()<<endl;
//         // cout<<(short)*buf<<" "<<;
//         // buf[9] = 0;
//         // cout<<buf<<" ";
//     }
//     cout<<" "<<endl;

//     // while(input.){

//     // }
// }

// int main()
// {
//     // int m=97;
//     // char s[] = "中国\n";
//     // FILE *fp = fopen("testBin.txt","w");   //二进制模式
//     // if(NULL == fp )
//     // {
//     //     return -1;
//     // }
//     // int successCont=fwrite(&m,sizeof(int),1,fp);
//     // if(successCont!=1)
//     //     cout<<"error"<<std::endl;
//     // fwrite(s,sizeof(char),sizeof(s),fp);
//     // fprintf(fp,"%d",m);   //格式化输出
//     // fclose(fp);
//     copyFile("./testBin.txt", "dstBin.txt");
//     return 1;

// }





// #include <iostream>
// #include <fstream>
// #include <map>

// using namespace std;

// namespace n_class {
// 	class Vvfs
// 	{
// 	public:
// 		Vvfs() {};
// 		~Vvfs() {};

// 		void mTest() {
// 			cout << m.size() << endl;
// 		}
// 		map<string, int> m;
// 	private:

// 	};

// 	class UdpServer
// 	{
// 	public:
// 		UdpServer() {};
// 		~UdpServer() {};
// 		virtual void h() = 0;
// 	private:

// 	};

// 	class FileServer:UdpServer
// 	{
// 	public:
// 		FileServer(Vvfs *pVvfs) {
// 			this->pVvfs = pVvfs;
// 		};
// 		~FileServer() {};
// 		void h() {};
// 		void listen() {
// 			pVvfs->mTest();
// 		};
// 	private:
// 		Vvfs *pVvfs;
// 	};

// 	class NodeSync
// 	{
// 	public:
// 		NodeSync() {};
// 		~NodeSync() {
// 			if(pFileServer) delete pFileServer;
//         	if(pVvfs) delete pVvfs;
// 		};
// 		void createVFS() {
// 			pVvfs = new Vvfs();
// 		};
// 		void createFileServer() {
// 			pFileServer = new FileServer(pVvfs);
// 		}

// 		void start() {
// 			createVFS();
// 			createFileServer();
// 			pFileServer->listen();
// 		}
// 	private:
// 		Vvfs *pVvfs;
// 		FileServer *pFileServer;
// 	};

// 	void test_main() {
// 		NodeSync node;
// 		node.start();
// 		//node.createVFS();
// 		//node.createFileServer();
// 	}
// }


// int i = 0;
// int func(){
// 	cout<<"func"<<endl;
// 	return i++;
// }

// int main(){
//    // n_class::test_main();
// //    cout<<"123"<<endl;
// 	cout<<i<<endl;
// }



#include <iostream>
#include <fstream>
#include <map>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <atomic>

using namespace std;
// int main(){
// 	// ofstream *pOfs = new ofstream("./test_dir/test.txt");
// 	// // ofs.write("123", 3);
// 	// // ofs.flush();
// 	// // ofs<<"124312"<<endl;
// 	// // ofs.flush();

// 	// *pOfs<<"132"<<endl;

// 	// while(1){
// 	// 	this_thread::sleep_for(chrono::seconds(1));
// 	// 	cout<<"sleep"<<endl;
// 	// }

// 	// ifstream ifs("./test_dir/remote/oplog.txt");
// 	// fstream ifs("./test_dir/remote/oplog.txt", ios::in | ios::out);
// 	// int op;
// 	// string path;
// 	// ifs>>i;
// 	// cout<<x;
// 	// long long t;

// 	// while(!ifs.eof()){
// 	// 	// ifs>>op>>path>>t;
// 	// 	ifs>>op;

// 	// 	cout<<ifs.eof()<<endl;
// 	// 	// if(ifs.)
		
// 	// 	if(ifs.fail()){
// 	// 		cout<<"fuck error"<<endl;
// 	// 	}

// 	// 	cout<<op<<" "<<path<<" "<<t<<endl;
// 	// 	// if(!op) {
// 	// 	// 	break;
// 	// 	// }
// 	// }

// 	// return 1;
// 	// ifs.seekg(ifs.end-1);

// 	// //ifs.close();
// 	// //ifs = fstream("./test_dir/remote/oplog.txt", ios::in | ios::out | ios::app);
// 	// cout<<ifs.cur<<endl;
// 	// ifs.seekp(0, ios::beg);
// 	// // ifs.seekg(0, ios::beg);
// 	// // cout<<ifs.tellp()<<endl;
// 	// ifs<<"789test"<<endl;
// 	// ifs<<"12321test"<<endl;
// 	// ifs<<"12321test"<<endl;

// 	// cout<<rename("./test_dir/test1.txt", "./test_dir/test.txt")<<endl;

// 	// atomic<bool> b(true);

// 	// int idx, prev_bro_idx, next_bro_idx, fa_idx, first_son_idx, last_son_idx, type, count;
//     // long tv_sec, tv_nsec, size;
//     // // char hashBuf[17];
//     // string readHash, name, dirPath;
// 	// ifstream ifs("./test_dir/remote/vfr.txt");
// 	// ifs>>count;
// 	// while(!ifs.eof()){
// 	// 	// ifs>>idx>>name>>dirPath>>fa_idx>>prev_bro_idx>>next_bro_idx>>first_son_idx>>last_son_idx>>type>>tv_sec>>tv_nsec>>size>>readHash;
// 	// 	ifs>>idx>>name>>dirPath>>fa_idx>>prev_bro_idx>>next_bro_idx>>first_son_idx>>last_son_idx>>type>>tv_sec>>tv_nsec>>readHash;
//     //     cout<<readHash<<endl;
//     //     cout<<idx << " "<<name<< " "<<dirPath<< " "<<fa_idx<< " "<<prev_bro_idx<< " "<<next_bro_idx<< " "<<first_son_idx<< " "<<last_son_idx<< " "<<type<< " "<<tv_sec<< " "<<tv_nsec<< " "<<size<< " "<<readHash<<endl;
// 	// 	return 1;
// 	// }

// 	class Test
// 	{
// 	private:
// 	public:
// 		Test(){};
// 		Test(Test & t){};
// 		~Test(){};
// 	};
// }


class Person
{
public:
    Person(){}
    Person(const Person& p)
    {
        cout << "Copy Constructor" << endl;
    }

    Person& operator=(const Person& p)
    {
        cout << "Assign" << endl;
        return *this;
    }

private:
    int age;
    string name;
};

void f(Person p)
{
    return;
}

Person f1()
{
    Person p;
    return p;
}

int main()
{
    Person p;
	
    Person p1 = p;    // 1
    // Person p2;
    // p2 = p;           // 2
    // f(p2);            // 3

    // p2 = f1();        // 4

    // Person p3 = f1(); // 5

    getchar();
    return 0;
}