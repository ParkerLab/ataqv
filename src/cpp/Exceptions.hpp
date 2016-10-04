//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>


class FileException: public std::runtime_error {
public:
    explicit FileException(const std::string& msg) : std::runtime_error(msg) { }
};

#endif  // EXCEPTIONS_HPP
