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
            if(qclients_.size() > 0 || pending_client_sessions_req_resp_.size() > 0){
                qwe("xmmm","");
            }else {
                cv_.wait(lk);
            }

            while (!qclients_.empty()){
                pending_client_sessions_req_resp_.push(std::move(qclients_.front()));
                qclients_.pop();
            }

            if(pending_client_sessions_req_resp_.front().ewouldblock_flag){
                pending_client_sessions_req_resp_.push(std::move(pending_client_sessions_req_resp_.front()));
                pending_client_sessions_req_resp_.pop();
            }
        }

        qwe("qclients_ size: ", qclients_.size());

            /*
            std::string resp ("HTTP/1.1 200 OK\n"
                               "Content-Type: text/html\n"
                               "\r\n\r\n"
                               "<html><head><title>Hello World</title></head><h1>Hello World</h1></html>");
                               */

            //epeksergasia tou request kai lipsi tou minimatos pros apostoli
            onClientRequest(pending_client_sessions_req_resp_.front());

            int snd;
            size_t total_bytes = 0;
            size_t bytesleft = pending_client_sessions_req_resp_.front().send_message.size();
            while (total_bytes < pending_client_sessions_req_resp_.front().send_message.size()){
                snd = send(pending_client_sessions_req_resp_.front().socket,
                           pending_client_sessions_req_resp_.front().send_message.data() + total_bytes,
                           bytesleft, 0);

                if (snd == -1){
                    if (errno == EWOULDBLOCK || errno == EAGAIN){
                        //thetw to flag se true, wste sto epono loop na to vgalei apo tin
                        //prwti thesi tou queue kai na to valei stin teleftaia, wste na epeksergastoun
                        //alla sockets an yparxoun.
                        pending_client_sessions_req_resp_.front().ewouldblock_flag = true;
                        break;
                    }
                    //allo sfalma opote teleiwnw tin epeksergasia
                    pending_client_sessions_req_resp_.front().send_message.clear();
                    pending_client_sessions_req_resp_.front().send_message_has_more_bytes = false;
                    break;
                }

                if (snd == 0){
                    //remote peer closed the connection
                    pending_client_sessions_req_resp_.front().send_message.clear();
                    pending_client_sessions_req_resp_.front().send_message_has_more_bytes = false;
                    break;
                }

                total_bytes += snd;
                bytesleft -= snd;
            }//while send loop
            pending_client_sessions_req_resp_.front().send_message_index += total_bytes;

            pending_client_sessions_req_resp_.front().send_message.erase(
                                                pending_client_sessions_req_resp_.front().send_message.begin(),
                                                pending_client_sessions_req_resp_.front().send_message.begin() + total_bytes);

            if (pending_client_sessions_req_resp_.front().send_message.size() == 0 &&
                    pending_client_sessions_req_resp_.front().send_message_has_more_bytes == false){
                if (!pending_client_sessions_req_resp_.front().keep_alive){
                    //close(pending_client_sessions_req_resp_.front().socket);
                    std::lock_guard<std::mutex> qlk(HTML_MessageProcessor::qclients_close_mutex);
                    HTML_MessageProcessor::qclients_close.push_back(std::move(pending_client_sessions_req_resp_.front()));

                    //QTcpSocket s;
                    //s.connectToHost("127.0.0.1", 12349);
                    //s.waitForConnected(5000);
                    //s.close();
                }
                pending_client_sessions_req_resp_.pop();
            }

            //shutdown(client_session.socket, SHUT_RDWR);
            //close(client_session.socket);
        } //
    //}//
}

void HTML_MessageProcessor::onClientRequest(ClientSession &client_session){
    //ean yparxei minima pros apostoli se ekremotita
    //den kanw tipota
    if(client_session.ewouldblock_flag){
        client_session.ewouldblock_flag = false;
        return;
    }

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

    QFileInfoList list;
    std::string response_body;
    std::vector<char> response_body_vect;

    bool include_header = true;
    long pending_bytes = 0;

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
        QByteArray bytes;


        //ypologismos bytes pros apostoli.
        int FILE_CHUNK = 8192; //8192=8.1mb/s, 16384/32768=8.5mb/s
        include_header = !client_session.send_message_has_more_bytes;
        file_io.seek(client_session.send_message_index - client_session.send_message_header_size);
        if (file_io.size() - client_session.send_message_index + client_session.send_message_header_size > FILE_CHUNK){
            bytes.resize(FILE_CHUNK);
            file_io.read(bytes.data(), FILE_CHUNK);
            pending_bytes = file_io.size() - FILE_CHUNK;
            client_session.send_message_has_more_bytes = true;

        } else if (file_io.size() - client_session.send_message_index + client_session.send_message_header_size <= FILE_CHUNK) {
            bytes.resize(file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            file_io.read(bytes.data(), file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            client_session.send_message_has_more_bytes = false;
        }

        response_body_vect.insert(response_body_vect.end(), bytes.begin(), bytes.end());
    }else{
        if (url != "/"){
            bool chdir = directory.setCurrent(QString::fromStdString(url));
            if(!chdir){
               qwe("could not change directory","");
            }
        }
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

        response_body = "<html><head><title>Open Web Server</title></head><body><h1>" +
                directory.currentPath().toStdString() +
                "</h1>" +
                                       os.str() +
                                       "</body></html>";
        mime_str = "text/html";
        response_body_vect.insert(response_body_vect.begin(), response_body.begin(), response_body.end());
    } //else


    std::string response_header ("HTTP/1.1 200 OK\r\n"
              "Content-Type: " + mime_str + "\r\n" +
              "Connection: close\r\n" +
              "Content-Length: " + std::to_string(response_body_vect.size() + pending_bytes) + "\r\n"
              "\r\n");

    //dimiourgw to minuma pros apostoli
    client_session.send_message.clear();
    if (include_header){
        std::vector<char> response_header_vect(response_header.begin(), response_header.end());
        client_session.send_message.insert(client_session.send_message.end(), response_header_vect.begin(), response_header_vect.end());
        client_session.send_message_header_size = response_header_vect.size();
    }
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
