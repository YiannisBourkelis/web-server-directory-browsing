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

        //ClientSession client_session;
        while (!csess_.empty()){
            //client_session = std::move(csess_.front());
            //csess_.pop();

            /*
            std::string resp ("HTTP/1.1 200 OK\n"
                               "Content-Type: text/html\n"
                               "\r\n\r\n"
                               "<html><head><title>Hello World</title></head><h1>Hello World</h1></html>");
                               */
            onClientRequest(csess_.front());

            int snd;
            size_t total_bytes = 0;
            size_t bytesleft = csess_.front().send_message.size();
            while (total_bytes < csess_.front().send_message.size()){
                snd = send(csess_.front().socket, csess_.front().send_message.data()+total_bytes, bytesleft, 0);

                if (snd == -1){
                    if (errno == EWOULDBLOCK || errno == EAGAIN){
                        //qwe("send loop ewouldblock","");
                        //diagrafo ta bytes pou exoun stalei mexrei stigmis
                        //client_session.recv_message.erase(client_session.recv_message.begin() + total_bytes);
                        //csess_.push(std::move(client_session));
                        break;
                    }
                    //allo sfalma opote teleiwnw tin epeksergasia
                    csess_.front().send_message.clear();
                    break;
                }

                if (snd == 0){
                    //remote peer closed the connection
                    csess_.front().send_message.clear();
                    break;
                }

                total_bytes += snd;
                bytesleft -= snd;
            }//while send loop

            csess_.front().send_message.erase(csess_.front().send_message.begin(),
                                              csess_.front().send_message.begin() + total_bytes);

            if (csess_.front().send_message.size() == 0){
                if (!csess_.front().keep_alive){
                    std::unique_lock<std::mutex> qlk(HTML_MessageProcessor::qclients_close_mutex);
                    HTML_MessageProcessor::qclients_close.push_back(std::move(csess_.front()));
                    QTcpSocket s;
                    //s.connectToHost("127.0.0.1", 12349);
                    //s.waitForConnected(5000);
                    //s.close();
                }
                csess_.pop();
            }

            //shutdown(client_session.socket, SHUT_RDWR);
            //close(client_session.socket);
        } //while
    }
}

void HTML_MessageProcessor::onClientRequest(ClientSession &client_session){
    //ean yparxei minima pros apostoli se ekremotita
    //den kanw tipota
    if(client_session.send_message.size() > 0) return;

    //metatrepw to client request se string gia na to analysw
    std::string request(client_session.recv_message.begin(), client_session.recv_message.end());

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

    QFileInfoList list;
    std::string response_body;
    std::vector<char> response_body_vect;

    QMimeDatabase db;
    QMimeType mime;
    std::string mime_str;
    //ean to url einai arxeio epistrefw arxeio, alliws lista me
    //ta directory kai ta arxeia ws html
    QFileInfo check_file(QString::fromStdString(url));
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()){
        //einai arxeio
        mime = db.mimeTypeForFile(check_file);
        mime_str = mime.name().toStdString();

        QFile file_io(check_file.absoluteFilePath());
        file_io.open(QIODevice::ReadOnly);
        QByteArray bytes = file_io.readAll();
        response_body_vect.insert(response_body_vect.end(), bytes.begin(), bytes.end());
    }else{
        //directory
        list = directory.entryInfoList();
        for (auto file:list){
          if(file.isDir()){
              //einai directory
        os << "<br /><a href=""" << file.absoluteFilePath().toHtmlEscaped().toStdString()  << """>"
           << QString("<DIR> ").toHtmlEscaped().toStdString()
           << file.fileName().toHtmlEscaped().toStdString() << "</a>";
          }else {
              //einai arxeio
        os << "<br /><a href=""" << file.absoluteFilePath().toHtmlEscaped().toStdString()  << """>"
         << file.fileName().toHtmlEscaped().toStdString() << "</a>";
          }
        }

        response_body = "<html><head><title>Hello World!!!!!</title></head><body><h1>Hello World 1</h1>" +
                                       os.str() +
                                       "</body></html>";
        mime_str = "text/html";
        response_body_vect.insert(response_body_vect.begin(), response_body.begin(), response_body.end());
    } //else


    std::string response_header ("HTTP/1.1 200 OK\r\n"
              "Content-Type: " + mime_str + "\r\n" +
              "Connection: close\r\n" +
              "Content-Length: " + std::to_string(response_body_vect.size()) + "\r\n"
              "\r\n");

    //dimiourgw to minuma pros apostoli
    std::vector<char> response_header_vect(response_header.begin(), response_header.end());
    client_session.send_message.insert(client_session.send_message.end(), response_header_vect.begin(), response_header_vect.end());
    client_session.send_message.insert(client_session.send_message.end(), response_body_vect.begin(), response_body_vect.end());

    //ypologizw keep-alive
    std::transform(request.begin(), request.end(), request.begin(), ::tolower);
    if(request.find("http/1.0") != std::string::npos){
        client_session.keep_alive = false;
    }
    else if(request.find("keep-alive") != std::string::npos){
        client_session.keep_alive  = true;
    } else {
        client_session.keep_alive = false;
    }
}

void HTML_MessageProcessor::onNewClientMessage(const ClientSession &client_session)
{
    std::string get_request (client_session.recv_message.begin(), client_session.recv_message.end());
    qwe(get_request,"");

    {
        std::lock_guard<std::mutex> lk(qclients_mutex_);
        qclients_.push(std::move(client_session));
    }
    cv_.notify_one();
}
