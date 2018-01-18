#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <vector>

class ClientSession
{
public:
    ClientSession();

    int socket;
    int fds_index;
    bool keep_alive = true;
    bool ewouldblock_flag = true;
    std::vector<char> recv_message;
    std::vector<char> send_message;

};

#endif // CLIENTSESSION_H
