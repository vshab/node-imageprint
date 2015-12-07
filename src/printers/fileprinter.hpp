#ifndef FILEPRINTER_HPP
#define FILEPRINTER_HPP

#include "printer.hpp"

/**
 * Dummy printer.
 * Simply write image to the file.
 */
class FilePrinter:
    public Printer
{

public:

    FilePrinter(std::string printerName);

    void print(const char* data, size_t length);

private:

    std::string printerName;

};

#endif
