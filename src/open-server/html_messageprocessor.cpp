#include "html_messageprocessor.h"

#include <string>
#include <iostream>

#ifndef WIN32
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_ntop
#include <unistd.h> //close
#include <netinet/tcp.h> //gia to TCP_NODELAY
#else
//#define NOMINMAX
#include <stdio.h>
#include "winsock2.h"
#include <ws2tcpip.h>
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)
#define in_addr_t uint32_t
#pragma comment(lib, "Ws2_32.lib")
#endif


#include <condition_variable>

#include "helper_functions.h"
#include <QDir>
#include <QUrl>
#include <string>
#include <sstream>
#include <iostream>
#include <QString>
#include <QRegExp>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTcpSocket>

//static variables declaration
std::vector<ClientSession> HTML_MessageProcessor::qclients_close;
std::mutex HTML_MessageProcessor::qclients_close_mutex;

HTML_MessageProcessor::HTML_MessageProcessor()
{
    std::thread worker (&HTML_MessageProcessor::processMessageQueue, this);
    worker_ = std::move(worker);

    /*
    for (int i=0; i<10; i++){
        //HTTP_Message_Dispatcher md;
        msg_dispatchers.push_back(HTTP_Message_Dispatcher());
    }
    */
}

void HTML_MessageProcessor::processMessageQueue()
{
    int worker = 0;
    while (true) {
        // Wait until main() sends data
        std::unique_lock<std::mutex> lk(qclients_mutex_);
        cv_.wait(lk);

        //egine eidopoiisi na epeksergastw to queue
        //anathetw ta requests sta worker threads
        while (!qclients_.empty()){

            if (worker == 0){
                worker = 1;
                msgdisp0.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 1){
                worker = 2;
                msgdisp1.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 2){
                worker = 3;
                msgdisp2.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 3){
                worker = 4;
                msgdisp3.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 4){
                worker = 5;
                msgdisp4.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 5){
                worker = 6;
                msgdisp5.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 6){
                worker = 7;
                msgdisp6.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 7){
                worker = 8;
                msgdisp7.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 8){
                worker = 9;
                msgdisp8.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 9){
                worker = 10;
                msgdisp9.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 10){
                worker = 11;
                msgdisp10.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }
            if (worker == 11){
                worker = 0;
                msgdisp11.onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                continue;
            }

                /*
                msg_dispatchers[worker].onNewMessageArrived(std::move(qclients_.front()));
                qclients_.pop();
                worker++;
                if (worker == msg_dispatchers.size()){
                    worker = 0;
                }
                continue;
                */

        }//while (workeres)

    }//while

}//processMessageQueue



void HTML_MessageProcessor::onNewClientMessage(const ClientSession &client_session)
{
    std::string get_request (client_session.request.request_header.begin(), client_session.request.request_header.end());
    qwe(get_request,"");

    {
        std::lock_guard<std::mutex> lk(qclients_mutex_);
        qclients_.push(std::move(client_session));
    }
    cv_.notify_one();
}
