#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <cstring> // to zero out adress for structs
#include "common.h"

void clientCon(){
    // Obtain socket handle
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd < 0){
        die("socket()");
    }
    //create network addres
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // connect to the servers port
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // test on same computer
    
    //connect
    int rv = connect(fd,(const struct sockaddr*)&addr, sizeof(addr));
    if(rv){
        die("connect");
    }
    // send multiple queries to test server
    int32_t r = query(fd,"hello1");
    if(r){
        goto L_DONE;
    }
    r = query(fd, "hello2");
    if(r){
        goto L_DONE;
    }
    L_DONE:
        close(fd);

}

int main(){
    clientCon();
    return 0;
}
