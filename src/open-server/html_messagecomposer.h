#ifndef HTML_MESSAGECOMPOSER_H
#define HTML_MESSAGECOMPOSER_H

#include "message_composer.h"
#include "html_messageprocessor.h"
#include <memory>


class HTML_MessageComposer : public MessageComposer
{
public:
    HTML_MessageComposer();

    //varibales
    std::unique_ptr<HTML_MessageProcessor> msg_processor_;

    //public interface
    virtual void onNewClientConnect();
    virtual void onClientDisconnect();
    virtual void onClientDataArrived(const int socket, int fds_index, const std::vector<char> data,const int data_size);
    virtual bool verifyMessageComplete(const std::map<int, ClientSession>::iterator msg_it);

private:
};

#endif // HTML_MESSAGECOMPOSER_H
