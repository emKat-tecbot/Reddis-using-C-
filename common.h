#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cerrno> // for errno
#include <unistd.h>  // read and write

//error func
static void die(const char* msg){
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}

const size_t max_msg = 4096; //maximum length of a message