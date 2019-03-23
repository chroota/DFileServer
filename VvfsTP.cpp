#include "VvfsTp.hpp"

void VvfsTp::run(int argc, char *argv[]){
    if (argc < 2) {
        help();
    }

    if (!strcmp(argv[1], NEW_FILE)) 
    {
        // cout<<NEW_FILE<<endl;
        if (argc < 4) {
            help();
        }
        string srcPath = argv[2];
        string dstPath = argv[3];
        
    }else if(!strcmp(argv[1], RM_FILE))
    {
        // cout<<RM_FILE<<endl;
        if(argc < 3){
            help();
        }
    }else if(!strcmp(argv[1], MV_FILE))
    {
        // cout<<MV_FILE<<endl;
        if(argc < 4){
            help();
        }
    
    }else if(!strcmp(argv[1], GET_FILE))
    {
        // cout<<GET_FILE<<endl;
        if(argc < 4){
            help();
        }
        
    }else if(!strcmp(argv[1], LS_FILE))
    {
        if(argc < 3){
            help();
        }
        // cout<<LS_FILE<<endl;
    }else{
        help();
    }
}

int main(int argc, char *argv[]){
    VvfsTp tp;
    tp.run(argc, argv);
}