#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <cstring> // to zero out adress for structs
#include <cerrno>
#include <cstring>
#include "common.h"

// request_one (reads and writes)
// do to byte specifications we use total_read and total_write instead of read and write

static int32_t one_request(int connfd){
    // Stage 1. Reading the length of the message
    char rbuf[4 + max_msg]; // 4 = header[byte size or msg/ tells TCP where the message ends and the next one beggins (if there is a next one)]
    errno = 0; // for checking if there was an error (os changes value automatically if an error occurs)
    int32_t r = total_read(connfd,rbuf,4); // read 4 bytes into rbuf
    if(r){
        if(errno != 0){
            std::cout << "read() error" << "\n";
            return r;
        }
    }
    uint32_t len = 0;// length of message
    memcpy(&len,rbuf,4); //copy rbufs into len to know msg length
    if(len > max_msg){
        std::cout << "Message is too long" << "\n";
        return -1;
    }
    // Stage 2. Reading the message
    r = total_read(connfd,&rbuf[4],len); // &rbuf[4] = pointer to where the message starts
    if(r){ // non zero values are true in c++
        std::cout << "read() error" << "\n";
        return r;
    }
    
    std::cout << "client says: ";
    std::cout.write(&rbuf[4],len);
    std::cout << "\n";
    // Stage 3. Reply
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)]; // so we have byte size of msg
    len = (uint32_t)strlen(reply); // get length of message
    memcpy(wbuf,&len,4); 
    memcpy(&wbuf[4],reply,len); // copys 4 bytes on len into wbuf
    return total_write(connfd,wbuf,len + 4);
}

void serverCon(){
    // Obtain socket handle
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1; // keep SO_REUSEADDR activated so os dosent prevent you from using the port again after crash or restart
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //create network adress struct
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // convert port to big endian so network understands the port
    addr.sin_addr.s_addr = htonl(0); //accept connection from anywhere (wildcard IP 0.0.0.0)

    //give adress info to os (bind)
    int rv = bind(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if(rv){ die("bind()"); } // exits if bind dosent succed(dosent return 0)

    //listen (create socket and place established connections in a quewe)
    rv = listen(fd, SOMAXCONN);
    if(rv){ die("listen()"); }

    // accept conections
    while(true){
        //create struct that stores client info (port and IP)
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        //accept conections
        int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
        if(connfd < 0){
            continue; // dont crash server
        }

        // serve one client connection at a time
        while(true){
            int32_t r = one_request(connfd);
            if(r){ break; } // error or EOF, move to next client
        }

        close(connfd); // close conection
    }
}

int main(){
    serverCon();
    return 0;
}