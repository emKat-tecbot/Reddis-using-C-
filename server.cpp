#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <cstring> // to zero out adress for structs
#include "common.h"

//conecting from the server side
void serverCon(){
    // Obtain socket handle
    int fd = socket(AF_INET,SOCK_STREAM,0);
    int val = 1; // keep SO_REUSEADDR activated so os dosent prevent you from using the port again after crash or restart
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //create network adress struct
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234); // convert port to bid eddian so netwrok understands the port
    addr.sin_addr.s_addr = htonl(0); //accept connection from anywhere (wildcard IP 0.0.0.0)

    //give adress info to os (bind)
    int rv = bind(fd,(const struct sockaddr*)&addr, sizeof(addr));
    if(rv){die("bind()");} // exits if bind dosent succed(dosent return 0)

    //listen (create socket and place established connections in a quewe)
    rv = listen(fd,SOMAXCONN);
    if(rv){die("listen()");}

    // accept conections
    while(true){
        //create struct that stores client info (port and IP)
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        //accept conections
        int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
        if (connfd < 0){ 
            continue; // dont crash server(
        }
        // only serve one client connection at a time
        while(true){
            // accept client
            struct sockaddr_in client_addr = {};
            int connfd = accept(fd,(struct sockaddr* )&client_addr, &addrlen);
            if(connfd < 0){
                continue; // accept() returns a negative number if wrong
            }
            // serve one client connection at once
            while(true){
                int32_t r = one_request(connfd);
                if(r){break;}
            }
        }
        close(connfd); // close conection
    }

}

int main(){
    serverCon();
    return 0;
}
