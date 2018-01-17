#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <string>
#include <iostream>


template<typename T>
void qwe(std::string console_message, T val_){
#ifdef QT_DEBUG
    std::cout << console_message << " " << val_ << std::endl;
#endif
}

#endif // HELPER_FUNCTIONS_H
