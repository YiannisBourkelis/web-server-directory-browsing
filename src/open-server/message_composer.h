#ifndef MESSAGE_COMPOSER_H
#define MESSAGE_COMPOSER_H

#include <map>
#include <vector>
#include "clientsession.h"

class MessageComposer
{
public:
    //constructor
    MessageComposer();

    //destructor
    virtual ~MessageComposer();

    //public interface
    virtual void onNewClientConnect();
    virtual void onClientDisconnect();
    virtual void onClientDataArrived(const int socket, int fds_index, const std::vector<char> data, int data_size);

protected:
    std::map<int, ClientSession> messages_;
};

#endif // MESSAGE_COMPOSER_H
