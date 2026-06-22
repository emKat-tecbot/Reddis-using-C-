#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <stdio.h> // error handling
#include <cstring> // to zero out adress for structs

int main(){
    // Obtain socket handle
    int fd = socket(AF_INET,SOCK_STREAM,0);

    std::cout << "start";
    return 0;
};
