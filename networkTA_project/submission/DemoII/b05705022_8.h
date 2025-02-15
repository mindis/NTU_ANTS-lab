#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "ThreadPool.h"
using namespace std;

#define IP   "127.0.0.1"
#define PORT 8080
const int MAX_CLIENTS = 5;
const int MAX_WAITING_TIME = 60;

class ClientInfo {

    friend class Server;

    ClientInfo(string n, string i, int b): 
            _name(n), _ip(i), _port(0), _balance(b), _isOnline(true) {}        

    string _name;
    string _ip;
    int _port;
    int _balance;
    bool _hasLoggedin;
    bool _isOnline;
};

class Server {
public:
    Server();
    ~Server() { 
        close(_sock); _pool->shutdown(); 
        delete _pool; 
        for (auto& u: _users) delete u;
    }
    void run();
    void handle(int client, sockaddr_in* info);
    ClientInfo* find_user(string) const;
    bool reg();
    string list() const;
    void report_unknown(int, ClientInfo*, string&);
private:
    int                     _sock;
    struct sockaddr_in      _addr;
    vector<ClientInfo*>     _users;
    ThreadPool*             _pool;
};

#endif // SERVER_H