#ifndef HTTP_MESSAGE_DISPATCHER_H
#define HTTP_MESSAGE_DISPATCHER_H

#include "clientsession.h"

#include <queue>
#include "clientsession.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <QString>

class HTTP_Message_Dispatcher
{
public:
    HTTP_Message_Dispatcher();

    static std::mutex cache_mutex;
    static std::map<QString, std::vector<char>> cache_;

    void dispatchMessageQueue();
    void onNewMessageArrived(const ClientSession &client_session);
private:
    //variables
    std::thread  worker_;
    std::mutex  client_sessions_req_resp_mutex;
    std::queue<ClientSession> client_sessions_req_resp_queue_;

    std::condition_variable  cv_;

    //methods
    void onClientRequest(ClientSession &client_session);
};

#endif // HTTP_MESSAGE_DISPATCHER_H
