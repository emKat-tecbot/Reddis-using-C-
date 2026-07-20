#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <cstring> // to zero out adress for structs
#include "common.h"


// read funt
static int32_t total_read(int fd, char*buf, size_t n){
    while(n > 0){ // makes sure that it will read all the bites of a message and not return less bytes
        ssize_t r = read(fd, buf,n);
        if(r <= 0){
            return -1; //error (didnt read)
        }
        assert((size_t)r <= n); // crashes if somehow reads more than 4 bytes
        n-= r; // how many bites read() didnt read
        buf +=r; // indece where to continue filling buf
    } 
    return 0;
}

// write full
static int32_t total_write(int fd, char*buf, size_t n){
    while(n > 0){
        ssize_t w = write(fd, buf,n);
        if(w < 0){
            return -1; // error (didnt write)
        }
        assert((size_t)w <= n); // crashes if it somehow write more than 4 bytes
        n -= (size_t)w;
        buf += (size_t)w;
    }
    return 0;
}


// query (reads and writes)
static int32_t query(int fd, const char* text){
    // Stage 1. test length of query
    uint32_t len = (uint32_t)strlen(text);
    if(len > max_msg){
        return -1;
    }
    // Stage 2. send request
    char wbuf[4 + max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    if(int32_t r = total_write(fd,wbuf,4+len)){
        return r;
    }
    // Stage 3. read reply
    char rbuf[4 + max_msg];
    errno = 0;
    int32_t r = total_read(fd, rbuf, 4);
    if(r){
        if(errno != 0){
            std::cout << "read() error" << "\n";
            return r;
        }
    }
    memcpy(&len,rbuf,4);
    if(len > max_msg){
        std::cout << "Message is too long" << "\n";
        return -1;
    }
    r = total_read(fd, &rbuf[4], len);
    if(r){
        std::cout << "read() error" << "\n";
        return r;
    }
    std::cout << "server says: ";
    std::cout.write(&rbuf[4],len);
    std::cout << "\n";
    return 0;
}


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
