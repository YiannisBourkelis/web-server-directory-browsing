/* ***********************************************************************
 * (C) 2018 by Yiannis Bourkelis (hello@andama.org)
 * ***********************************************************************/

#include <QCoreApplication>
#include "poll_server.h"
#include "html_messagecomposer.h"
#include "signal.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    #ifndef WIN32
    //gia na mi prokaleitai crash otan paw na grapsw se socket pou exei kleisei
    //http://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
    signal(SIGPIPE, SIG_IGN);
    #endif

    PollServer pollserver;

    HTML_MessageComposer html_msg_composer;
    HTML_MessageProcessor html_msg_processor;
    html_msg_composer.msg_processor_.reset(&html_msg_processor);
    pollserver.msgComposer.reset(&html_msg_composer);

    pollserver.start(12349, PollServer::IPv4);

    return a.exec();
}
