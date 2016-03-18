#include "nodeimageprinter.hpp"

#ifdef _WIN32
#include "printers/xps/xpsprinter.hpp"
typedef XPSPrinter PlatformPrinter;
#else
#include "printers/fileprinter.hpp"
typedef FilePrinter PlatformPrinter;
#endif

#include <iostream>

Nan::Persistent<v8::Function> NodeImagePrinter::constructor;

NodeImagePrinter::NodeImagePrinter(std::string name)
{
    workerThread.start();

    workerThread.exec([this, name]()
    {
        printer.reset(new PlatformPrinter(name));
    });
}

NAN_MODULE_INIT(NodeImagePrinter::Init)
{
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Printer").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "print", Print);
    Nan::SetPrototypeMethod(tpl, "done", Done);

    constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());

    Nan::Set(target, Nan::New("exports").ToLocalChecked(),
                     Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(NodeImagePrinter::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowTypeError("Should not be called as function!");
        return;
    }

    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("Argument must be String");
        return;
    }

    v8::String::Utf8Value printerName(info[0]);

    NodeImagePrinter* nodeImagePrinter = new NodeImagePrinter(*printerName);
    nodeImagePrinter->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(NodeImagePrinter::Print)
{
    if (!info[0]->IsObject())
    {
        Nan::ThrowTypeError("Argument must be Object");
        return;
    }

    NodeImagePrinter* nodeImagePrinter = Nan::ObjectWrap::Unwrap<NodeImagePrinter>(info.This());

    // Persist the buffer
    std::shared_ptr<Nan::Persistent<v8::Object>> buffer(new Nan::Persistent<v8::Object>(info[0]->ToObject()));

    const char* data = node::Buffer::Data(info[0]);
    const size_t length = node::Buffer::Length(info[0]);

    nodeImagePrinter->workerThread.exec([nodeImagePrinter, data, length]()
    {
        nodeImagePrinter->printer->print(data, length);
    },
    [buffer]() mutable
    {
        buffer->Reset();
    });
}

NAN_METHOD(NodeImagePrinter::Done)
{
    NodeImagePrinter* nodeImagePrinter = Nan::ObjectWrap::Unwrap<NodeImagePrinter>(info.This());

    nodeImagePrinter->workerThread.exec([nodeImagePrinter]()
    {
        nodeImagePrinter->printer.reset();
    });

    // Wait for all the functions and results are done and only then allow
    // object to be destroyed.
    nodeImagePrinter->Ref();
    nodeImagePrinter->workerThread.stop([nodeImagePrinter]()
    {
        nodeImagePrinter->Unref();
    });
}
