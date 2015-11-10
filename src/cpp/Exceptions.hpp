//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>


class FileException: public std::runtime_error {
public:
    FileException() : std::runtime_error("") { }
    FileException(std::string msg) : std::runtime_error(msg) { }
};

#endif  // EXCEPTIONS_HPP
