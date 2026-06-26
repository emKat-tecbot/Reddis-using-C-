#include <cstdio>
#include <cstdlib>
#include <cassert>

//error func
static void die(const char* msg){
    perror(msg);
    exit(1);
}

// read funt
static int32_t total_read(int fd, char*buf, size_t n){
    while(n < 0){ // makes sure that it will read all the bites of a message and not return less bytes
        ssize_t r = read(fd, buf,n);
        if(r <= 0){
            return -1; //error (didnt read)
        }
        assert((size_t)buf <= n); // crashes if somehow reads more than 4 bytes
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
        assert((size_t)buf <= n); // crashes if it somehow write more than 4 bytes
        n -= (size_t)w;
        buf += (size_t)w;
    }
    return 0;
}

// request_one (reads and writes) FOR SERVER
// do to byte specifications we use total_read and total_write instead of read and write
const size_t max_msg = 4096; //maximum length of a message

static int32_t one_request(int connfd){
    // Stage 1. Reading the length of the message
    char rbuf[4 + max_msg]; // 4 = header[byte size or msg/ tells TCP where the message ends and the next one beggins (if there is a next one)]
    errno = 0; // for checking if there was an error (os changes value automatically if an error occurs)
    int32_t read = total_read(connfd,rbuf,4); // read 4 bytes into rbuf
    if(read){
        if(errno != 0){
            std::cout << "read() error" << "\n";
            return read;
        }
    }
    uint32_t len = 0;// length of message
    memcpy(&len,rbuf,4); //copy rbufs into len to know msg length
    if(len > max_msg){
        std::cout << "Message is too long" << "\n";
        return -1;
    }
    // Stage 2. Reading the message
    read = total_read(connfd,&rbuf[4],len); // &rbuf[4] = pointer to where the message starts
    if(read){ // non zero values are true in c++
        std::cout << "read() error" << "\n";
        return read;
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

// query (reads and writes) CLIENT ONLY
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
}