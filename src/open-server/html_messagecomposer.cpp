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

void HTML_MessageComposer::onClientDataArrived(const int socket, int fds_index, const std::vector<char> data, const int data_size)
{
    auto stored_client = messages_.find(socket);

    if (stored_client == messages_.end()){
        //den yparxoun data gia afto to socket
        //opote ta kataxwrw olla
        //std::vector<char> tmpdata(data.begin(), data.begin() + data_size);
        ClientSession tmpcsession;
        tmpcsession.socket = socket;
        tmpcsession.fds_index = fds_index;
        std::move(data.begin(), data.begin() + data_size, std::back_inserter(tmpcsession.message));
        messages_[socket] = tmpcsession;
        //elegxo ean to mynima exe symplirwthei
        verifyMessageComplete(messages_.find(socket));
    } else {
        //proyparxoun dedomena, opote ta kanw merge
        stored_client->second.message.insert(stored_client->second.message.end(), data.begin(), data.end());
        //elegxo ean to mynima exe symplirwthei
        verifyMessageComplete(stored_client);
    }
}

bool HTML_MessageComposer::verifyMessageComplete(const std::map<int, ClientSession>::iterator msg_it)
{
    if (msg_it->second.message.size() > 3){
        if ( (msg_it->second.message.at(msg_it->second.message.size() - 4) == 13) && // \r
             (msg_it->second.message.at(msg_it->second.message.size() - 3) == 10) && // \n
             (msg_it->second.message.at(msg_it->second.message.size() - 2) == 13) && // \r
             (msg_it->second.message.at(msg_it->second.message.size() - 1) == 10) ){ // \n

            msg_processor_->onNewClientMessage(std::move(msg_it->second));
            messages_.erase(msg_it);           
            return true;
        }
    }
    return false;
}
