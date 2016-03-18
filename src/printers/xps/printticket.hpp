#ifndef XPS_PRINT_TICKET_HPP
#define XPS_PRINT_TICKET_HPP

#include "printschema.hpp"
#include "printcapabilities.hpp"

struct IStream;

namespace PrintSchema
{

namespace PrintTicket
{
    // Construct and write printticket into the stream.
    bool writePrintTicket(PageMediaSize const& pageMediaSize,
                          PrintCapabilities::Context const& context,
                          IStream* printTicketStream);
}

}

#endif
