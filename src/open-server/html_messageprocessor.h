#ifndef HTML_MESSAGEPROCESSOR_H
#define HTML_MESSAGEPROCESSOR_H

#include <queue>
#include "clientsession.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>

#include "http_message_dispatcher.h"

class HTML_MessageProcessor
{
public:
    HTML_MessageProcessor();

    //static variables
    static std::vector<ClientSession> qclients_close;
    static std::mutex qclients_close_mutex;

    //methods
    virtual void onNewClientMessage(const ClientSession &client_session);

    void processMessageQueue();
    void onClientRequest(ClientSession &client_session);
private:
    //variables
    std::queue<ClientSession> qclients_;

    std::thread worker_;

    HTTP_Message_Dispatcher msg_dispatcher;
    std::vector<HTTP_Message_Dispatcher> msg_dispatchers;

    std::mutex qclients_mutex_;
    std::condition_variable cv_;

    HTTP_Message_Dispatcher msgdisp0;
    HTTP_Message_Dispatcher msgdisp1;
    HTTP_Message_Dispatcher msgdisp2;
    HTTP_Message_Dispatcher msgdisp3;
    HTTP_Message_Dispatcher msgdisp4;
    HTTP_Message_Dispatcher msgdisp5;
    HTTP_Message_Dispatcher msgdisp6;
    HTTP_Message_Dispatcher msgdisp7;
    HTTP_Message_Dispatcher msgdisp8;
    HTTP_Message_Dispatcher msgdisp9;
    HTTP_Message_Dispatcher msgdisp10;
    HTTP_Message_Dispatcher msgdisp11;

};

#endif // HTML_MESSAGEPROCESSOR_H
