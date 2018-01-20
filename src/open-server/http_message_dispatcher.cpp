#include "http_message_dispatcher.h"

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

#include "html_messagecomposer.h"


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

//static variables
std::mutex HTTP_Message_Dispatcher::cache_mutex;
std::map<std::string, std::vector<char>> HTTP_Message_Dispatcher::cache_;

HTTP_Message_Dispatcher::HTTP_Message_Dispatcher()
{
    std::thread worker (&HTTP_Message_Dispatcher::dispatchMessageQueue, this);
    worker_ = std::move(worker);
}



void HTTP_Message_Dispatcher::onNewMessageArrived(const ClientSession &client_session)
{
    {
        std::lock_guard<std::mutex> lk(client_sessions_req_resp_mutex);
        client_sessions_req_resp_queue_.push(std::move(client_session));
    }
    cv_.notify_one();
}

void HTTP_Message_Dispatcher::dispatchMessageQueue()
{
    while (true) {
        // Wait until main() sends data
        std::unique_lock<std::mutex> lk(client_sessions_req_resp_mutex);
        {
            if(client_sessions_req_resp_queue_.size() > 0){
                qwe("xmmm","");
            }else {
                cv_.wait(lk);
            }


            if(client_sessions_req_resp_queue_.front().ewouldblock_flag && client_sessions_req_resp_queue_.size() > 1){
                client_sessions_req_resp_queue_.push(std::move(client_sessions_req_resp_queue_.front()));
                client_sessions_req_resp_queue_.pop();
            }

        }

            /*
            std::string resp ("HTTP/1.1 200 OK\n"
                               "Content-Type: text/html\n"
                               "\r\n\r\n"
                               "<html><head><title>Hello World</title></head><h1>Hello World</h1></html>");
                               */

            //epeksergasia tou request kai lipsi tou minimatos pros apostoli
            onClientRequest(client_sessions_req_resp_queue_.front());

            //ean i proigoumeni apostoli teleiwse me EWOULDBLOCK
            //kanw poll sto socket wste na min katanalwnontai cpu cycles
            /*
            if (pending_client_sessions_req_resp_.front().ewouldblock_flag){
                struct pollfd wait_fd[1];
                wait_fd[0].events = POLLIN;
                wait_fd[0].fd = pending_client_sessions_req_resp_.front().socket;
                poll(wait_fd, 1, 5000);
            }
            */

            int snd;
            size_t total_bytes = 0;
            size_t bytesleft = client_sessions_req_resp_queue_.front().send_message.size();
            while (total_bytes < client_sessions_req_resp_queue_.front().send_message.size()){
                snd = send(client_sessions_req_resp_queue_.front().socket,
                           client_sessions_req_resp_queue_.front().send_message.data() + total_bytes,
                           bytesleft, 0);

                if (snd == -1){
                    if (errno == EWOULDBLOCK || errno == EAGAIN){
                        //thetw to flag se true, wste sto epono loop na to vgalei apo tin
                        //prwti thesi tou queue kai na to valei stin teleftaia, wste na epeksergastoun
                        //alla sockets an yparxoun.
                        client_sessions_req_resp_queue_.front().ewouldblock_flag = true;
                        break;
                    }
                    //allo sfalma opote teleiwnw tin epeksergasia
                    client_sessions_req_resp_queue_.front().send_message.clear();
                    client_sessions_req_resp_queue_.front().send_message_has_more_bytes = false;
                    break;
                }

                if (snd == 0){
                    //remote peer closed the connection
                    client_sessions_req_resp_queue_.front().send_message.clear();
                    client_sessions_req_resp_queue_.front().send_message_has_more_bytes = false;
                    break;
                }

                total_bytes += snd;
                bytesleft -= snd;
            }//while send loop
            client_sessions_req_resp_queue_.front().send_message_index += total_bytes;

            //diagrafw oso apo to minima mporese na stalthei
            client_sessions_req_resp_queue_.front().send_message.erase(
                                                client_sessions_req_resp_queue_.front().send_message.begin(),
                                                client_sessions_req_resp_queue_.front().send_message.begin() + total_bytes);

            //ean exei stalei olo to minima kai den xreiazetai synexeia
            //tote diagrafw to session apo to queue
            if (client_sessions_req_resp_queue_.front().send_message.size() == 0 &&
                    client_sessions_req_resp_queue_.front().send_message_has_more_bytes == false){
                if (!client_sessions_req_resp_queue_.front().keep_alive){
                    //close(pending_client_sessions_req_resp_.front().socket);
                    std::lock_guard<std::mutex> qlk(HTML_MessageProcessor::qclients_close_mutex);
                    HTML_MessageProcessor::qclients_close.push_back(std::move(client_sessions_req_resp_queue_.front()));

                    //QTcpSocket s;
                    //s.connectToHost("127.0.0.1", 12349);
                    //s.waitForConnected(5000);
                    //s.close();
                }
                client_sessions_req_resp_queue_.pop();
            }

            //shutdown(client_session.socket, SHUT_RDWR);
            //close(client_session.socket);
        } //
    //}//
}

void HTTP_Message_Dispatcher::onClientRequest(ClientSession &client_session){

    std::string resp ("HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 107\r\n"""
                       "\r\n"
                       "<html><head><body><title>Hello World - hhhhhhhhhhhhhhhhhhhh</title></head><h1>Hello World</h1><body></html>");

    //client_session.send_message.insert(client_session.send_message.end(), resp.begin(), resp.end());


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
    //url = "" + url + "";
    QString url_decoded = QString::fromStdString(url).replace("%20", " ");

    std::ostringstream os;
    QDir directory;

    QFileInfoList list;
    std::string response_body;
    std::vector<char> response_body_vect;

    bool include_header = true;
    long pending_bytes = 0;

    QMimeDatabase db;
    QMimeType mime;
    std::string mime_str = "text/html";

    //ean to url einai arxeio epistrefw arxeio, alliws lista me
    //ta directory kai ta arxeia ws html
    QFileInfo check_file(url_decoded);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()){
        //einai arxeio
        //mime = db.mimeTypeForFile(check_file);//ok einai grigori i function
        //mime_str = mime.name().toStdString();

        qint64 file_size = check_file.size();
        QFile file_io(check_file.absoluteFilePath());

        auto cache_it = HTTP_Message_Dispatcher::cache_.find(url);
        bool cached = (cache_it != HTTP_Message_Dispatcher::cache_.end());

        if(!cached){
            file_io.open(QIODevice::ReadOnly);
            std::vector<char> tmp(file_size);
            file_io.read(tmp.data(), file_size);
            HTTP_Message_Dispatcher::cache_[url] = std::move(tmp);
            cache_it = HTTP_Message_Dispatcher::cache_.find(url);
        }


        //ypologismos bytes pros apostoli.
        int FILE_CHUNK = 1048576; //8192=8.1mb/s, 16384/32768=8.5mb/s 65536 262144 524288 >1048576 2097152 4194304
        include_header = !client_session.send_message_has_more_bytes;
        //metaferomai stin thesi apo opou tha arxisw na diavazw to arxeio.
        //if(!cached){
        //    file_io.seek(client_session.send_message_index - client_session.send_message_header_size);
        //}
        if (file_size - client_session.send_message_index + client_session.send_message_header_size > FILE_CHUNK){
            //to arxeio den xoraei olokliro sto send_message buffer opote diavazw mono ena tmima tou
            //me megethos oso oristike apo to FILE_CHUNK
            //bytes.resize(FILE_CHUNK);
            //file_io.read(bytes.data(), FILE_CHUNK);

            //response_body_vect.resize(FILE_CHUNK);

            //file_io.read(response_body_vect.data(), FILE_CHUNK);
            response_body_vect.insert(response_body_vect.end(),
                                      cache_it->second.begin() + (client_session.send_message_index - client_session.send_message_header_size),
                                      cache_it->second.begin() + FILE_CHUNK + (client_session.send_message_index - client_session.send_message_header_size));


            pending_bytes = file_size - FILE_CHUNK;
            client_session.send_message_has_more_bytes = true;

        } else if (file_size - client_session.send_message_index + client_session.send_message_header_size <= FILE_CHUNK) {
            //edw erxetai ean to arxeio xwraei sto send_message buffer, i ean einai to teleftaio tmima tou arxeiou
            //bytes.resize(file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            //file_io.read(bytes.data(), file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            long vres = file_size - client_session.send_message_index + client_session.send_message_header_size;
            //response_body_vect.resize(vres);
            //if(!cached){
            //    file_io.read(response_body_vect.data(), file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            //} else {

            //std::vector<char> tt2 = cache_it->second;
            //std::vector<char> tt = cache_.find(url)->second;
                response_body_vect.insert(response_body_vect.end(),
                                          cache_it->second.begin() + (client_session.send_message_index - client_session.send_message_header_size),
                                          cache_it->second.end());
            //}
            client_session.send_message_has_more_bytes = false;
        }

        //response_body_vect.insert(response_body_vect.end(), bytes.begin(), bytes.end());
    }else{

        QString current_dir = QDir::fromNativeSeparators(url_decoded);
        if (url != "/"){
            bool chdir = directory.setCurrent(current_dir);
            if(!chdir){
               qwe("could not change directory","");
            }
        } else {
            bool chdir = directory.setCurrent("/");
            if(!chdir){
                qwe("could not change directory","");
            }
        }

        //directory
        list = directory.entryInfoList();
        std::string url_encoded;
        for (auto file:list){
            url_encoded = file.absoluteFilePath().replace(" ", "%20").toStdString();
          if(file.isDir()){;
              //einai directory
        os << "<br /><a href=""" << url_encoded << """>"
           << QString("<DIR> ").toHtmlEscaped().toStdString()
           << file.fileName().toHtmlEscaped().toStdString() << "</a>";
          }else {
              //einai arxeio
        os << "<br /><a href=""" << url_encoded << """>"
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
    } //else (directory listing)


    std::string response_header ("HTTP/1.1 200 OK\r\n"
              "Content-Type: " + mime_str + "\r\n" +
              "Connection: close\r\n" +
              "Content-Length: " + std::to_string(response_body_vect.size() + pending_bytes) + "\r\n"
              "\r\n");

    //              "Connection: close\r\n" +




    //dimiourgw to minuma pros apostoli
    client_session.send_message.clear();
    if (include_header){
        std::vector<char> response_header_vect(response_header.begin(), response_header.end());
        client_session.send_message.insert(client_session.send_message.end(),
                                           std::make_move_iterator(response_header_vect.begin()),
                                           std::make_move_iterator(response_header_vect.end()));
        client_session.send_message_header_size = response_header_vect.size();
    }
    client_session.send_message.insert(client_session.send_message.end(),
                                       std::make_move_iterator(response_body_vect.begin()),
                                       std::make_move_iterator(response_body_vect.end()));

/*
    client_session.send_message.clear();
    client_session.send_message.insert(client_session.send_message.end(), resp.begin(), resp.end());
     return;
     */


     /*
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
    */
}

/*
void HTTP_Message_Dispatcher::onClientRequest(ClientSession &client_session){
    std::string resp ("HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 107\r\n"""
                       "\r\n"
                       "<html><head><body><title>Hello World - hhhhhhhhhhhhhhhhhhhh</title></head><h1>Hello World</h1><body></html>");

    client_session.send_message.insert(client_session.send_message.end(), resp.begin(), resp.end());
    return;

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
    //url = "" + url + "";
    QString url_decoded = QString::fromStdString(url).replace("%20", " ");

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
    QFileInfo check_file(url_decoded);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()){
        //einai arxeio
        mime = db.mimeTypeForFile(check_file);//ok einai grigori i function
        mime_str = mime.name().toStdString();

        QFile file_io(check_file.absoluteFilePath());

        auto cache_it = HTTP_Message_Dispatcher::cache_.find(url);
        bool cached = (cache_it != HTTP_Message_Dispatcher::cache_.end());

        if(!cached){
            file_io.open(QIODevice::ReadOnly);
            std::vector<char> tmp(file_io.size());
            file_io.read(tmp.data(), file_io.size());
            HTTP_Message_Dispatcher::cache_[url] = std::move(tmp);
            cache_it = HTTP_Message_Dispatcher::cache_.find(url);
        }


        //ypologismos bytes pros apostoli.
        int FILE_CHUNK = 1048576; //8192=8.1mb/s, 16384/32768=8.5mb/s 65536 262144 524288 >1048576 2097152 4194304
        include_header = !client_session.send_message_has_more_bytes;
        //metaferomai stin thesi apo opou tha arxisw na diavazw to arxeio.
        //if(!cached){
        //    file_io.seek(client_session.send_message_index - client_session.send_message_header_size);
        //}
        if (file_io.size() - client_session.send_message_index + client_session.send_message_header_size > FILE_CHUNK){
            //to arxeio den xoraei olokliro sto send_message buffer opote diavazw mono ena tmima tou
            //me megethos oso oristike apo to FILE_CHUNK
            //bytes.resize(FILE_CHUNK);
            //file_io.read(bytes.data(), FILE_CHUNK);

            //response_body_vect.resize(FILE_CHUNK);

            //file_io.read(response_body_vect.data(), FILE_CHUNK);
            response_body_vect.insert(response_body_vect.end(),
                                      cache_it->second.begin() + (client_session.send_message_index - client_session.send_message_header_size),
                                      cache_it->second.begin() + FILE_CHUNK + (client_session.send_message_index - client_session.send_message_header_size));


            pending_bytes = file_io.size() - FILE_CHUNK;
            client_session.send_message_has_more_bytes = true;

        } else if (file_io.size() - client_session.send_message_index + client_session.send_message_header_size <= FILE_CHUNK) {
            //edw erxetai ean to arxeio xwraei sto send_message buffer, i ean einai to teleftaio tmima tou arxeiou
            //bytes.resize(file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            //file_io.read(bytes.data(), file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            long vres = file_io.size() - client_session.send_message_index + client_session.send_message_header_size;
            //response_body_vect.resize(vres);
            //if(!cached){
            //    file_io.read(response_body_vect.data(), file_io.size() - client_session.send_message_index + client_session.send_message_header_size);
            //} else {

            //std::vector<char> tt2 = cache_it->second;
            //std::vector<char> tt = cache_.find(url)->second;
                response_body_vect.insert(response_body_vect.end(),
                                          cache_it->second.begin() + (client_session.send_message_index - client_session.send_message_header_size),
                                          cache_it->second.end());
            //}
            client_session.send_message_has_more_bytes = false;
        }

        //response_body_vect.insert(response_body_vect.end(), bytes.begin(), bytes.end());
    }else{

        QString current_dir = QDir::fromNativeSeparators(url_decoded);
        if (url != "/"){
            bool chdir = directory.setCurrent(current_dir);
            if(!chdir){
               qwe("could not change directory","");
            }
        } else {
            bool chdir = directory.setCurrent("/");
            if(!chdir){
                qwe("could not change directory","");
            }
        }

        //directory
        list = directory.entryInfoList();
        std::string url_encoded;
        for (auto file:list){
            url_encoded = file.absoluteFilePath().replace(" ", "%20").toStdString();
          if(file.isDir()){;
              //einai directory
        os << "<br /><a href=""" << url_encoded << """>"
           << QString("<DIR> ").toHtmlEscaped().toStdString()
           << file.fileName().toHtmlEscaped().toStdString() << "</a>";
          }else {
              //einai arxeio
        os << "<br /><a href=""" << url_encoded << """>"
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
    } //else (directory listing)


    std::string response_header ("HTTP/1.1 200 OK\r\n"
              "Content-Type: " + mime_str + "\r\n" +
              "Connection: close\r\n" +
              "Content-Length: " + std::to_string(response_body_vect.size() + pending_bytes) + "\r\n"
              "\r\n");

    //              "Connection: close\r\n" +

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
*/
