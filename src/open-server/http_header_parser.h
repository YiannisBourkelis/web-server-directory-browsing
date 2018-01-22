#ifndef HTTP_HEADER_PARSER_H
#define HTTP_HEADER_PARSER_H

#include <vector>
#include <QString>
#include <string>


class HTTP_Header_parser
{
public:
    HTTP_Header_parser();

    std::vector<char> request_header;//periexei to HTTP request
    QString request_path;

    int parse();
};

#endif // HTTP_HEADER_PARSER_H
