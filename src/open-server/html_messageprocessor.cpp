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
#include <string>
#include <sstream>
#include <iostream>
#include <QString>
#include <QRegExp>

//static variables declaration
std::vector<ClientSession> HTML_MessageProcessor::qclients_close;
std::mutex HTML_MessageProcessor::qclients_close_mutex;

HTML_MessageProcessor::HTML_MessageProcessor()
{
    std::thread worker (&HTML_MessageProcessor::processMessageQueue, this);
    worker_ = std::move(worker);
}

void HTML_MessageProcessor::processMessageQueue()
{
    while (true) {
        // Wait until main() sends data
        {
            std::unique_lock<std::mutex> lk(qclients_mutex_);
            cv_.wait(lk);
            while (!qclients_.empty()){
                csess_.push(std::move(qclients_.front()));
                qclients_.pop();
            }
        }

        qwe("qclients_ size: ", qclients_.size());

        ClientSession client_session;
        while (!csess_.empty()){
            client_session = std::move(csess_.front());
            csess_.pop();

            /*
            std::string resp ("HTTP/1.1 200 OK\n"
                               "Content-Type: text/html\n"
                               "\r\n\r\n"
                               "<html><head><title>Hello World</title></head><h1>Hello World</h1></html>");
                               */
            std::string resp;
            std::string request(client_session.message.begin(), client_session.message.end());
            onClientRequest(request, resp);

            int snd;
            size_t total_bytes = 0;
            while (total_bytes < resp.size()){
                snd = send(client_session.socket, resp.data(), resp.size(), 0);
                total_bytes += snd;

                if (snd == 1){
                    if (errno == EWOULDBLOCK){
                        qwe("send loop ewouldblock","");
                        //diagrafo ta bytes pou exoun stalei mexrei stigmis
                        client_session.message.erase(client_session.message.begin() + total_bytes);
                        csess_.push(std::move(client_session));
                        break;
                    }
                }

                if (snd == 0){
                    //remote peer closed the connection
                    break;
                }

            }//while send loop

            {
                if (!keep_alive){
                    //std::unique_lock<std::mutex> qlk(HTML_MessageProcessor::qclients_close_mutex);
                    //HTML_MessageProcessor::qclients_close.push_back(std::move(client_session));
                }
            }

            //shutdown(client_session.socket, SHUT_RDWR);
            //close(client_session.socket);
        }
    }
}

void HTML_MessageProcessor::onClientRequest(const std::string &request, std::string &response){
//lamvanw to directory pou zitithike
//GET dir /r/n
int rv = request.find_first_of("\r\n");
std::string get_line(request.begin() + 4, request.begin() + rv - 1);
int last_space = get_line.find_last_of(" ", get_line.size());
std::string url(get_line.begin(), get_line.begin() + last_space);

std::ostringstream os;
QDir directory;
if (url != "/"){
    bool chdir = directory.setCurrent(QString::fromStdString(url));
    if(!chdir){
       qwe("could not change directory","");
    }
}

QFileInfoList list = directory.entryInfoList();




  for (auto file:list){
      if(file.isDir()){
   os << "<br /><a href=""" << file.absoluteFilePath().toHtmlEscaped().toStdString()  << """>" << file.fileName().toStdString() << "</a>";
      }else {
       os << "<br />" << file.fileName().toHtmlEscaped().toStdString();
      }
  }

  std::string response_body("<html><head><title>Hello World!!!!!</title></head><body><h1>Hello World 1</h1>" +
                               os.str() +
                               "</body></html>");


   std::string response_header ("HTTP/1.1 200 OK\n"
              "Content-Type: text/html\n"
              "Content-Length: " + std::to_string(response_body.size()) +
              "\r\n\r\n");

   response = response_header + response_body;

   std::cout << response << std::endl;
   qwe(response,"");
}

void HTML_MessageProcessor::onNewClientMessage(const ClientSession &client_session)
{
    std::string get_request (client_session.message.begin(), client_session.message.end());
    qwe(get_request,"");

    {
        std::lock_guard<std::mutex> lk(qclients_mutex_);
        qclients_.push(std::move(client_session));
    }
    cv_.notify_one();
}
