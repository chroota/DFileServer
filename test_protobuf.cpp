// #include "people.pb.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "msg.pb.h"
using namespace std;

int main(){
    string s;
    //demo::People p;
    // demo::People p;  
    // p.set_name("Hideto");  
    // p.set_id(123);  
    // //p.set_email("hideto.bj@gmail.com");  
    // //p.set_name();
    // cout<<p.name()<<endl;
    // cout<<p.id()<<endl;
    // cout<<p.email()<<endl;
    // cout<<p.x()<<endl;
    //p.SerializeToString(&s);
    //cout<<s<<endl;
    // demo::OneMessage o;
    // o.set_type(demo::OneMessage_Type_BAR);
    Msg::CommonResponse csp;
    csp.set_status(0x01);
    csp.set_info("vvv");
    char buf[100];
    csp.SerializeToArray(buf, 100);
    cout<<buf<<endl;
    printf("123");
}