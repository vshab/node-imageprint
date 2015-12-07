#ifndef NODEIMAGEPRINTER_HPP
#define NODEIMAGEPRINTER_HPP

#include <memory>

#include <nan.h>

#include "workerthread.hpp"

class Printer;

/**
 * Node ImagePrinter.
 */
class NodeImagePrinter:
    public Nan::ObjectWrap
{

public:

    explicit NodeImagePrinter(std::string printerName);

    static NAN_MODULE_INIT(Init);

private:

    // Create new ImagePrinter instance.
    // Accepts printerName as parameter.
    static NAN_METHOD(New);

    // Print image from memory
    static NAN_METHOD(Print);

    // Be done with this printer
    static NAN_METHOD(Done);

private:

    static Nan::Persistent<v8::Function> constructor;

    WorkerThread workerThread;

    std::unique_ptr<Printer> printer;
};

#endif
