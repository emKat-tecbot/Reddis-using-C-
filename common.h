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

const size_t max_msg = 32 << 20; //maximum length of a message
const size_t max_arg = 200; // maximum number of arguments a command can have