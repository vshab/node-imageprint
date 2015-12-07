#include "fileprinter.hpp"

#include <fstream>

FilePrinter::FilePrinter(std::string printerName):
    Printer(printerName),
    printerName(printerName)
{
}

void FilePrinter::print(const char* data, size_t length)
{
    std::ofstream f(printerName + ".png", std::ofstream::out | std::ofstream::binary);

    f.write(data, length);

    f.close();
}
