#include <nan.h>

#include "nodeimageprinter.hpp"

void InitAll(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE exports,
             Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE module)
{
    NodeImagePrinter::Init(module);
}

NODE_MODULE(imageprinter, InitAll)
