#ifndef HTML_MESSAGEPROCESSOR_H
#define HTML_MESSAGEPROCESSOR_H

#include <queue>
#include "clientsession.h"
#include <thread>
#include <mutex>
#include <condition_variable>

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

    std::mutex qclients_mutex_;
    std::condition_variable cv_;
    std::queue<ClientSession> pending_client_sessions_req_resp_;

};

#endif // HTML_MESSAGEPROCESSOR_H
