#ifndef XPS_PRINT_TICKET_HPP
#define XPS_PRINT_TICKET_HPP

#include "printschema.hpp"
#include "printcapabilities.hpp"

struct IStream;

namespace PrintSchema
{

namespace PrintTicket
{
    bool writePrintTicket(PageMediaSize const& pageMediaSize,
                          PrintCapabilities::Context const& context,
                          IStream* printTicketStream);
}

}

#endif