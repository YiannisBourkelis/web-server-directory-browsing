#include "http_header_parser.h"

HTTP_Header_parser::HTTP_Header_parser()
{
    //TODO: isws xreiazetai veltistopoiisi to megethos twn arxikwn bytes pou tha ginoun reserve
    request_header.reserve(512);
}

int HTTP_Header_parser::parse()
{
    //metatrepw to client request se string gia na to analysw
    std::string request(request_header.begin(), request_header.end());

    //lamvanw to directory pou zitithike
    //GET dir /r/n
    int rv = request.find_first_of("\r\n");
    std::string get_line(request.begin() + 4, request.begin() + rv - 1);
    int last_space = get_line.find_last_of(" ", get_line.size());
    std::string url(get_line.begin(), get_line.begin() + last_space);
    //url = "" + url + "";

    request_path = QString::fromStdString(url).replace("%20", " ");

    return 0;
}
