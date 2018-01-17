/* ***********************************************************************
 * (C) 2018 by Yiannis Bourkelis (hello@andama.org)
 * ***********************************************************************/

#include <QCoreApplication>
#include "poll_server.h"
#include "html_messagecomposer.h"
#include <csignal>

void signal_handler(int signal)
{
  std::cout << signal;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    std::signal(SIGINT, signal_handler);

    PollServer pollserver;

    HTML_MessageComposer html_msg_composer;
    HTML_MessageProcessor html_msg_processor;
    html_msg_composer.msg_processor_.reset(&html_msg_processor);
    pollserver.msgComposer.reset(&html_msg_composer);

    pollserver.start(12349, PollServer::IPv4);

    return a.exec();
}
