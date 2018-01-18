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
    bool ewouldblock_flag = false;
    std::vector<char> recv_message;
    std::vector<char> send_message;
    bool send_message_has_more_bytes = false;
    long send_message_index = 0; //posa bytes exoun stalei apo to arxeio, ean einai megalo
    long send_message_header_size = 0;

};

#endif // CLIENTSESSION_H
