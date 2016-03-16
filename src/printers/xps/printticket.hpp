#include "printschema.hpp"
#include "printcapabilities.hpp"

namespace PrintSchema
{

namespace PrintTicket
{
    bool writePrintTicket(PageMediaSize const& pageMediaSize,
                          PrintCapabilities::Context const& context,
                          IStream* printTicketStream);
}

}
