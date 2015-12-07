#include "xpsprinter.hpp"

#include <cstdlib>

#include <Objbase.h>
#include <Objidl.h>
#include <Shlwapi.h>
#include <Wincodec.h>
#include <XpsPrint.h>

#pragma comment(lib, "XpsPrint.lib")
#pragma comment(lib, "Windowscodecs.lib")

struct XPSRect :
    XPS_RECT {};

XPSPrinter::XPSPrinter(std::string printerName):
    Printer(printerName)
{
    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    xpsFactory.CoCreateInstance(__uuidof(XpsOMObjectFactory));
    wicFactory.CoCreateInstance(CLSID_WICImagingFactory1);

    completionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    this->printerName.resize(printerName.length());
    mbstowcs(&this->printerName.front(), printerName.c_str(), printerName.length());
}

XPSPrinter::~XPSPrinter()
{
    CoUninitialize();

    if (completionEvent)
    {
        CloseHandle(completionEvent);
    }
}

bool XPSPrinter::getImageSize(CComPtr<IStream> imageStream,
                              std::pair<unsigned int, unsigned int>& size,
                              std::pair<double, double>& resolution)
{
    CComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory->CreateDecoderFromStream(imageStream,
                                                   NULL,
                                                   WICDecodeMetadataCacheOnLoad,
                                                   &decoder)))
    {
        return false;
    }

    CComPtr<IWICBitmapFrameDecode> bitmapSource;
    if (FAILED(decoder->GetFrame(0, &bitmapSource)))
    {
        return false;
    }

    if (FAILED(bitmapSource->GetSize(&size.first, &size.second)))
    {
        return false;
    }

    if (FAILED(bitmapSource->GetResolution(&resolution.first, &resolution.second)))
    {
        return false;
    }

    // Move the stream to the beggining after read is done
    LARGE_INTEGER zero = { 0 };
    imageStream->Seek(zero, STREAM_SEEK_SET, NULL);

    return true;
}

CComPtr<IXpsOMPath> XPSPrinter::createRectanglePath(const XPSRect* rect)
{
    XPS_POINT startPoint = { rect->x, rect->y };
    CComPtr<IXpsOMGeometryFigure> rectFigure;
    if (FAILED(xpsFactory->CreateGeometryFigure(&startPoint, &rectFigure)))
    {
        return NULL;
    }

    if (FAILED(rectFigure->SetIsClosed(TRUE)))
    {
        return NULL;
    }

    if (FAILED(rectFigure->SetIsFilled(TRUE)))
    {
        return NULL;
    }

    XPS_SEGMENT_TYPE segmentTypes[3] = {
        XPS_SEGMENT_TYPE_LINE,
        XPS_SEGMENT_TYPE_LINE,
        XPS_SEGMENT_TYPE_LINE
    };
    FLOAT segmentData[6] = {
        rect->x,               rect->y + rect->height,
        rect->x + rect->width, rect->y + rect->height,
        rect->x + rect->width, rect->y
    };
    BOOL segmentStrokes[3] = {
        TRUE, TRUE, TRUE
    };
    if (FAILED(rectFigure->SetSegments(3, 6, segmentTypes, segmentData, segmentStrokes)))
    {
        return NULL;
    }

    CComPtr<IXpsOMGeometry> imageRectGeometry;
    if (FAILED(xpsFactory->CreateGeometry(&imageRectGeometry)))
    {
        return NULL;
    }

    CComPtr<IXpsOMGeometryFigureCollection> geomFigureCollection;
    if (FAILED(imageRectGeometry->GetFigures(&geomFigureCollection)))
    {
        return NULL;
    }

    if (FAILED(geomFigureCollection->Append(rectFigure)))
    {
        return NULL;
    }

    CComPtr<IXpsOMPath> rectPath;
    if (FAILED(xpsFactory->CreatePath(&rectPath)))
    {
        return NULL;
    }

    if (FAILED(rectPath->SetGeometryLocal(imageRectGeometry)))
    {
        return NULL;
    }

    return rectPath;
}

CComPtr<IXpsOMPackage> XPSPrinter::buildPackage(CComPtr<IStream> imageStream)
{
    CComPtr<IXpsOMPackage> xpsPackage;
    if (FAILED(xpsFactory->CreatePackage(&xpsPackage)))
    {
        return NULL;
    }

    CComPtr<IOpcPartUri> opcPartUri;
    if (FAILED(xpsFactory->CreatePartUri(L"/DocumentSequence.fdseq", &opcPartUri)))
    {
        return NULL;
    }

    CComPtr<IXpsOMDocumentSequence> xpsFDS;
    if (FAILED(xpsFactory->CreateDocumentSequence(opcPartUri, &xpsFDS)))
    {
        return NULL;
    }

    opcPartUri.Release();
    if (FAILED(xpsFactory->CreatePartUri(L"/Document.fdoc", &opcPartUri)))
    {
        return NULL;
    }

    CComPtr<IXpsOMDocument> xpsFD;
    if (FAILED(xpsFactory->CreateDocument(opcPartUri, &xpsFD)))
    {
        return NULL;
    }

    opcPartUri.Release();
    if (FAILED(xpsFactory->CreatePartUri(L"/Page.fpage", &opcPartUri)))
    {
        return NULL;
    }

    // 4x6" rotated
    // Got theese constants from printer properties.
    XPS_SIZE pageSize = { 6.213f * 96.0f,  4.06f * 96.0f };
    CComPtr<IXpsOMPage> xpsPage;
    if (FAILED(xpsFactory->CreatePage(&pageSize, L"en-US", opcPartUri, &xpsPage)))
    {
        return NULL;
    }

    opcPartUri.Release();
    if (FAILED(xpsFactory->CreatePartUri(L"/Image.fimg", &opcPartUri)))
    {
        return NULL;
    }

    CComPtr<IXpsOMImageResource> imageResource;
    if (FAILED(xpsFactory->CreateImageResource(imageStream, XPS_IMAGE_TYPE_PNG, opcPartUri, &imageResource)))
    {
        return NULL;
    }

    std::pair<unsigned int, unsigned int> size;
    std::pair<double, double> resolution;
    if (!getImageSize(imageStream, size, resolution))
    {
        return NULL;
    }

    // Whole image viewBox
    XPS_RECT viewBox = { 0.0f, 0.0f, 0.0f, 0.0f };
    viewBox.width = FLOAT((double)size.first * 96.0 / resolution.first);
    viewBox.height = FLOAT((double)size.second * 96.0 / resolution.second);

    // Whole page viewPort
    XPS_RECT viewPort = { 0.0f, 0.0f, 0.0f, 0.0f };
    viewPort.width = pageSize.width;
    viewPort.height = pageSize.height;

    CComPtr<IXpsOMImageBrush> imageBrush;
    if (FAILED(xpsFactory->CreateImageBrush(imageResource, &viewBox, &viewPort, &imageBrush)))
    {
        return NULL;
    }

    // Page-sized rect
    XPSRect rect;
    rect.x = 0.0f;
    rect.y = 0.0f;
    rect.width = pageSize.width;
    rect.height = pageSize.height;

    CComPtr<IXpsOMPath> imageRectPath = createRectanglePath(&rect);
    if (!imageRectPath)
    {
        return NULL;
    }

    if (FAILED(imageRectPath->SetAccessibilityShortDescription(L"ImageRectPath")))
    {
        return NULL;
    }

    if (FAILED(imageRectPath->SetFillBrushLocal(imageBrush)))
    {
        return NULL;
    }

    CComPtr<IXpsOMVisualCollection> pageVisuals;
    if (FAILED(xpsPage->GetVisuals(&pageVisuals)))
    {
        return NULL;
    }

    if (FAILED(pageVisuals->Append(imageRectPath)))
    {
        return NULL;
    }

    CComPtr<IXpsOMPageReference> xpsPageRef;
    if (FAILED(xpsFactory->CreatePageReference(&pageSize, &xpsPageRef)))
    {
        return NULL;
    }

    if (FAILED(xpsPackage->SetDocumentSequence(xpsFDS)))
    {
        return NULL;
    }

    CComPtr<IXpsOMDocumentCollection> fixedDocuments;
    if (FAILED(xpsFDS->GetDocuments(&fixedDocuments)))
    {
        return NULL;
    }

    if (FAILED(fixedDocuments->Append(xpsFD)))
    {
        return NULL;
    }

    CComPtr<IXpsOMPageReferenceCollection> pageRefs;
    if (FAILED(xpsFD->GetPageReferences(&pageRefs)))
    {
        return NULL;
    }

    if (FAILED(pageRefs->Append(xpsPageRef)))
    {
        return NULL;
    }

    if (FAILED(xpsPageRef->SetPage(xpsPage)))
    {
        return NULL;
    }

    return xpsPackage;
}

void XPSPrinter::print(const char* data, size_t length)
{
    // Create memory stream
    CComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream((const BYTE *)data, length));

    // Build package
    CComPtr<IXpsOMPackage> package = buildPackage(stream);
    if (!package)
    {
        return;
    }

    // Start printing job
    CComPtr<IXpsPrintJob> job;
    CComPtr<IXpsPrintJobStream> jobStream;
    if (FAILED(StartXpsPrintJob(printerName.c_str(),
                                L"NodeImagePrinterJob",
                                NULL,
                                NULL,
                                completionEvent,
                                NULL,
                                0,
                                &job,
                                &jobStream,
                                NULL)))
    {
        return;
    }

    // Write the package and finish the job.
    if (SUCCEEDED(package->WriteToStream(jobStream, FALSE)))
    {
        jobStream->Close();

        // Wait for the job is done
        if (completionEvent)
        {
            WaitForSingleObject(completionEvent, INFINITE);
        }
    }
    else if (job)
    {
        job->Cancel();
    }
}
