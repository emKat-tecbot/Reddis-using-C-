#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <stdio.h> // error handling
#include <cstring> // to zero out adress for structs

static void die(const char* msg){
    perror(msg);
    exit(1);
}

static void do_something(int connfd){
    char rbuf[64] = {}; // buffer: stores clients message
    ssize_t n = read(connfd,rbuf,sizeof(rbuf)-1); //read message from conection and put it in rbuf
    if(n < 0){ 
        perror("read() error");
        return;
    }
    std::cout << "client says: " << rbuf << endl;
    char wbuf[] = "world"; //message sent to the client
    write(connfd, wbuf, strlen(wbuf));
}

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
        do_something(connfd); //interact with client (read and write)
        close(connfd); // close conection
    }

}
// conecting from the client side
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
    write(fd,msg,strlen(msg));
    char rbuf[64] = {}; //stores servers response
    ssize_t n = read(fd,rbuf,sizeof(rbuf)-1);
    if (n < 0){
        die("read");
    }
    std::cout << "server says: " << rbuf << endl;
    close(fd);
}

int main(){
    clientCon();
    return 0;
}