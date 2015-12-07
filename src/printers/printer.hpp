#ifndef PRINTER_HPP
#define PRINTER_HPP

#include <string>

/**
 * Platform-specific printer interface
 */
class Printer
{

public:

    Printer(std::string printerName)
    {
    }

    virtual ~Printer()
    {
    }

    virtual void print(const char* data, size_t length) = 0;

};

#endif
