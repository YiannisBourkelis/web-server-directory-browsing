#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <vector>

class ClientSession
{
public:
    ClientSession();

    int socket;
    int fds_index;
    std::vector<char> message;
};

#endif // CLIENTSESSION_H
