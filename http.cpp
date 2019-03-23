#include "http.hpp"


Http::Http(){

}

Http::~Http(){

}

bool Http::listen(int port){
    if (start(port)) {
        return true;
    }
    return false;
}

void Http::handle(char buf[]){
    printf("recieve: %s\n", buf);
}