#include "message_composer.h"
#include "qglobal.h"

MessageComposer::MessageComposer()
{

}

MessageComposer::~MessageComposer()
{

}

void MessageComposer::onNewClientConnect()
{

}

void MessageComposer::onClientDisconnect()
{

}

void MessageComposer::onClientDataArrived(const int socket, int fds_index, const std::vector<char> &data, int data_size)
{
    Q_UNUSED(socket);
    Q_UNUSED(fds_index);
    Q_UNUSED(data);
    Q_UNUSED(data_size);
}
