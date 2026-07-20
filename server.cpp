#include <sys/socket.h> // networking functions
#include <netinet/in.h> //sock adresess
#include <unistd.h> // read and write and closes
#include <iostream>
#include <cstring> // to zero out adress for structs
#include <cerrno>
#include <cstring>
#include <vector>
#include <poll.h> // for poll() function
#include <fcntl.h> // for fcntl() function
#include "common.h"

static void msg(const char* msg){
    fprintf(stderr, "%s\n", msg);
}

static void msg_errno(const char* msg){
    fprintf(stderr, " [errno: %d] %s\n", errno, msg);
}

// struct that stores the state the client is in during each event loop iteration
struct Conn{
    int fd = -1;
    // fd list for readiness API
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;//tells event loop to destroy conection
    std::vector<uint8_t> incoming;// stores incoming data from client
    std::vector<uint8_t> output;// stores outgoing data to client
};

int poll(struct pollfd* fds, nfds_t nfds, int timeout);// makes OS tell us when a fd is ready to read or write


//make listening socket non blocking 
static void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcntl eror");
        return;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if(errno){
        die("fcntl error");
    }
}

static Conn *handle_accept(int fd){
    //accept a new connection
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*) &client_addr, &addrlen);
    if(connfd < 0){
        msg_errno("accept() error");
        return NULL;
    } // if accept() fails, return NULL

    uint32_t ip = client_addr.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,ntohs(client_addr.sin_port));

    // set the new connection to non-blocking mode
    fd_set_nb(connfd);
    
    // create a new Conn object for the new connection
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true; // read the 1sts request
    return conn;
}

// append data to a buffer
static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len){
    buf.insert(buf.end(),data,data + len);
}

// remove data from the front of a buffer
static void buf_consume(std::vector<uint8_t> &buf, size_t n){
    buf.erase(buf.begin(), buf.begin() + n);
}

static bool try_one_request(Conn *conn){
    // Try to parse buffer
    // header
    if(conn->incoming.size() < 4){ // not enough data to parse a message (4 bytes = 32 bit integer)
        return false;
    }
    uint32_t len = 0;
    memcpy(&len,conn->incoming.data(),4); // copy the first 4 bytes of incoming buffer to len
    if(len > max_msg){ // protocol errror
        msg("message too long");
        conn->want_close = true;
        return false;
    }
    // body
    if(4 + len > conn->incoming.size()){
        return false; // not enough data to parse a message
    }

    // Process the parsed message
    // Generate response
    const uint8_t *request = &conn->incoming[4]; // pointer to the start of the message body
    //do something with the request
    printf("client says: len: %d data: %.*s\n", len, len < 100? len:100, request);
    //generate response
    buf_append(conn->output, (const uint8_t*)&len,4); // send back the length of message
    buf_append(conn->output, request, len);// send back the message

    // Remove the message from incoming
    buf_consume(conn->incoming, 4 + len);
    return true; // success
}

// send data to client
static void handle_write(Conn *conn){
    assert(conn->output.size() > 0); // make sure there is data to send
    ssize_t rv = write(conn->fd, &conn->output[0], conn->output.size());
    if(rv < 0 && errno == EAGAIN){ // error handling
        return; // NOT READY
    }
    if(rv < 0){
        msg_errno("write() error");
        conn->want_close = true;
        return;
    }

    // Remove the sent data from the output
    buf_consume(conn->output, (size_t)rv);

    // update the write readiness flag
    if(conn->output.size() == 0){ // all data writen
        conn->want_read = true;
        conn->want_write = false;
    } // else: want write
}

static void handle_read(Conn *conn){
    // Do a non blocking read
    uint8_t buf[64*1024]; // buffer to read data into
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if(rv < 0 && errno == EAGAIN){// handle IO error or EOF
        return;
    }
    if(rv < 0){
        msg_errno("read() error");
        conn->want_close = true;
        return;
    }
    // handle EOF
    if(rv == 0){
        if(conn->incoming.size() == 0){
            msg("client closed connection");
        }else{
            msg("unexpected EOF");
        }
        conn->want_close = true;
        return;
    }
    // Add new data to the incoming buffer
    buf_append(conn->incoming,buf,(size_t)rv);
    // Try to parse buffer
    // Process the parsed message
    // Remove the message from incoming
    while(try_one_request(conn)){}; // we make it a loop so we can handle multiple requests in one read

    // Update readiness flag
    if(conn->output.size() > 0){ // has a response
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn); // try to send the response immediately
    } // else: want read
}


void serverCon(){
    // Obtain socket handle
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){die("socket()");} // exits if socket dosent succed(dosent return 0)
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

    fd_set_nb(fd); // set the listening socket to non-blocking mode

    //listen (create socket and place established connections in a quewe)
    rv = listen(fd, SOMAXCONN);
    if(rv){ die("listen()"); }

    //create event loop to accpet connections
    // map of all client connections, keyed by fd
    std::vector<Conn *> connections;

    // actual event loop
    std::vector<struct pollfd> fds;
    while(true){
        // prepare arguments for poll()
        fds.clear();
        // add listening socket to pollfd list
        struct pollfd pfd = {fd, POLLIN, 0}; // fd = fd; POLLIN = events; 0 = revents (starts empty, will fill in later)
        fds.push_back(pfd);
        // add every conected client to pollfd list
        for(Conn* conn:connections){ //loop through every slot in connections (since its indexed by fd some slots might be empty)
            if(!conn){// if empty slot, skip it
                continue;
            }
            struct pollfd pfd = {conn->fd,POLLERR,0}; // create pollfd struct for each client
            // create poll() flags
            if(conn->want_read){
                pfd.events |= POLLIN; //watch for incoming data
            }
            if(conn->want_write){
                pfd.events |= POLLOUT; //watch for when we can send data to client
            }
            fds.push_back(pfd); // add to the list
        }
        
        //wait for events to happen
        int rv = poll(fds.data(),(nfds_t)fds.size(), -1); // -1 = wait forever
        if(rv < 0 && errno != EINTR){continue;} // if errno is eintr, its not an error
        if(rv < 0){die("poll()");} // if poll() fails, exit program

        // handle the listening socket
        if(fds[0].revents){
            if(Conn *conn = handle_accept(fd)){
                // if the connection is valid, add it to the connections vector
                if(connections.size() <= (size_t)conn->fd){
                    connections.resize(conn->fd + 1);
                }
                assert(!connections[conn->fd]);
                connections[conn->fd] = conn;
            }
        }
        // handle conection (client) sockets
        for(size_t i = 1; i < fds.size(); i++){ // skips the 1st element becuase its the listnening sockety
            uint32_t ready = fds[i].revents;
            if(ready == 0){continue;}// if no events, skip
            Conn* conn = connections[fds[i].fd];
            if(ready & POLLIN){
                assert(conn->want_read);
                handle_read(conn);
            }
            if(ready & POLLOUT){
                assert(conn->want_write);
                handle_write(conn);
            }
            // close connection on error
            if((ready & POLLERR) || conn->want_close){
                (void)close(conn->fd);
                connections[conn->fd] = NULL;
                delete conn;
            }
        }
    }
}

int main(){
    serverCon();
    return 0;
}