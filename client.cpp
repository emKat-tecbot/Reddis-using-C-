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
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // test on same computer
    
    //connect
    int rv = connect(fd,(const struct sockaddr*)&addr, sizeof(addr));
    if(rv){
        die("connect");
    }
    char msg[] = "hello"; // message client sends to server
    ssize_t w = write(fd,msg,strlen(msg));
    (void) w;
    char rbuf[64] = {}; //stores servers response
    ssize_t n = read(fd,rbuf,sizeof(rbuf)-1);
    if (n < 0){
        die("read");
    }
    std::cout << "server says: " << rbuf << "\n";
    close(fd);
}

int main(){
    clientCon();
    return 0;
}
