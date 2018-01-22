#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <vector>
#include <map>
#include <http_header_parser.h>

class ClientSession
{
public:
    ClientSession();

    int socket;//to socket pou client sto opoio ginetai i apostoli dedomenwn
    int fds_index = 0;//i thesi pou vrisketai to socket mesa sto pollfd array, sto poll
    bool keep_alive = true;//ean sto HTTP request exei zitithei keep-alive
    bool ewouldblock_flag = false;//gia megala mynimata, ean den ginei apostoli me ena send, thetei afto to flag se true
    std::vector<char> send_message;//periexei ta dedomena pros apostoli
    bool send_message_has_more_bytes = false;//ta megala arxeia den fortwnontai oloklira sto send_message alla tmimatika. se afti tin periptwsi thetei to flag se true
    long send_message_index = 0; //posa bytes exoun synolika stalei apo to arxeio, ean einai megalo. perilamvanei to megethos tou HTTP response header
    long send_message_header_size = 0;//to megethos tou HTTP response header. Afairontas to apo to send_message_index exw ta synolika bytes pou stalthikan
    HTTP_Header_parser request;//to request opws stalthike apo ton client

    //cache specific
    std::map<QString, std::vector<char>>::iterator cache_iterator;
    bool is_cached = false;

};

#endif // CLIENTSESSION_H
