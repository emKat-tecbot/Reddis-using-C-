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