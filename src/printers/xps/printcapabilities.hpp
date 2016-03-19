#ifndef XPS_PRINT_CAPABILITIES_HPP
#define XPS_PRINT_CAPABILITIES_HPP

#include <map>

#include "printschema.hpp"

struct IStream;

namespace PrintSchema
{

namespace PrintCapabilities
{
    // Context for current capabilities: version and device-specific namespaces.
    struct Context
    {
        unsigned int version;
        std::map<std::wstring, std::wstring> namespaces;
    };

    /*
    @@ TODO

    bool getPageMediaSizes(IStream* printCapabilitiesStream,
                           std::list<PageMediaSize>& pageMediaSizes);
    */

    bool getPageImageableSize(IStream* printCapabilitiesStream,
                              PageImageableSize& pageImageableSize);
}

}

#endif
