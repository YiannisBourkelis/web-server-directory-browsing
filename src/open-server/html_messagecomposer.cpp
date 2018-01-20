#include "html_messagecomposer.h"
#include <iostream>
#include <string>

HTML_MessageComposer::HTML_MessageComposer()
{

}

void HTML_MessageComposer::onNewClientConnect()
{

}

void HTML_MessageComposer::onClientDisconnect()
{

}

void HTML_MessageComposer::onClientDataArrived(const int socket, int fds_index, const std::vector<char> &data, const int data_size)
{
    auto stored_client = messages_.find(socket);

    if (stored_client == messages_.end()){
        //den yparxoun data gia afto to socket
        //opote ta kataxwrw olla
        //std::vector<char> tmpdata(data.begin(), data.begin() + data_size);
        ClientSession tmpcsession;
        tmpcsession.socket = socket;
        tmpcsession.fds_index = fds_index;
        std::move(data.begin(), data.begin() + data_size, std::back_inserter(tmpcsession.recv_message));
        messages_[socket] = std::move(tmpcsession);
        //elegxo ean to mynima exe symplirwthei
        verifyMessageComplete(messages_.find(socket));
    } else {
        //proyparxoun dedomena, opote ta kanw merge
        //TODO: na to kanw me std::move
        stored_client->second.recv_message.insert(stored_client->second.recv_message.end(),
                                                  std::make_move_iterator(data.begin()),
                                                  std::make_move_iterator(data.end()));
        //elegxo ean to mynima exe symplirwthei
        verifyMessageComplete(stored_client);
    }
}

bool HTML_MessageComposer::verifyMessageComplete(const std::map<int, ClientSession>::iterator msg_it)
{
    //ean exei lifthei olokliro to request, pou simainei oti teleiwnei me \r\n\r\n
    //epistrefw true, alliws epistrefw false kai perimeno to epomeno minima gia na to enosw.
    if (msg_it->second.recv_message.size() > 3){
            std::vector<char>::iterator tmp = msg_it->second.recv_message.end();
            if ( (*(--tmp) == 10) && // \n
                 (*(--tmp) == 13) && // \r
                 (*(--tmp) == 10) && // \n
                 (*(--tmp) == 13) ){ // \r
            msg_processor_->onNewClientMessage(std::move(msg_it->second));
            messages_.erase(msg_it);           
            return true;
        }//if
    }
    return false;
}
