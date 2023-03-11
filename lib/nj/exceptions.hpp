#ifndef ESCEPTIONS_HPP
#define ESCEPTIONS_HPP

#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
/// \brief The NotJpegException class
///
class NotJpegException : public std::runtime_error {
public:
    NotJpegException() : std::runtime_error("Given file is not a JPEG") {}
};

////////////////////////////////////////////////////////////////////////////////
/// \brief The UnsupportedException class
///
class UnsupportedException : public std::runtime_error {
public:
    UnsupportedException() : std::runtime_error("Format is not supported") {}
};

////////////////////////////////////////////////////////////////////////////////
/// \brief The SyntaxErrorException class
///
class SyntaxErrorException : public std::runtime_error {
public:
    SyntaxErrorException() : std::runtime_error("Syntax error") {}
};


#endif // ESCEPTIONS_HPP
