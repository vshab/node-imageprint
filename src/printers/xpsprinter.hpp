#ifndef XPSPRINTER_HPP
#define XPSPRINTER_HPP

#include "printer.hpp"

#include <utility>

#include <atlbase.h>

struct IStream;
struct IWICImagingFactory;
struct IXpsOMObjectFactory;
struct IXpsOMPackage;
struct IXpsOMPath;

struct XPSRect;

/**
 * Windows XPS printer API. Prints 6x4" paper.
 * Require at least Windows 7.
 */
class XPSPrinter:
    public Printer
{

public:

    XPSPrinter(std::string printerName);

    ~XPSPrinter();

    void print(const char* data, size_t length);

private:

    // Get image size and resolution
    bool getImageSize(CComPtr<IStream> imageStream,
                      std::pair<unsigned int, unsigned int>& size,
                      std::pair<double, double>& resolution);

    // Create rectangle rect path
    CComPtr<IXpsOMPath> createRectanglePath(const XPSRect* rect);

    // Build XPS package with image stretched in document
    CComPtr<IXpsOMPackage> buildPackage(CComPtr<IStream> imageStream);

private:

    // Factories
    CComPtr<IXpsOMObjectFactory> xpsFactory;
    CComPtr<IWICImagingFactory> wicFactory;

    // Print completion event
    HANDLE completionEvent;

    std::wstring printerName;
};

#endif
