#ifndef XPS_PRINT_CAPABILITIES_HPP
#define XPS_PRINT_CAPABILITIES_HPP

#include <list>
#include <map>

#include "printschema.hpp"

namespace PrintSchema
{

namespace PrintCapabilities
{
    struct Context
    {
        unsigned int version;
        std::map<std::wstring, std::wstring> namespaces;
    };

        /*
    bool getPageMediaSizes(IStream* printCapabilitiesStream,
                           std::list<PageMediaSize>& pageMediaSizes);

    bool getPageImageableSize(IStream* printCapabilitiesStream,
                              PageImageableSize& pageImageableSize);
                              */
}

}

#endif