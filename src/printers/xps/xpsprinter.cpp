#include "xpsprinter.hpp"
#include "printticket.hpp"

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

bool XPSPrinter::processImage(CComPtr<IStream> inputImageStream,
                              std::pair<unsigned int, unsigned int>& size,
                              std::pair<double, double>& resolution,
                              CComPtr<IStream>& outputImageStream)
{
    // Decode input image
    CComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory->CreateDecoderFromStream(inputImageStream,
                                                   NULL,
                                                   WICDecodeMetadataCacheOnLoad,
                                                   &decoder)))
    {
        return false;
    }

    CComPtr<IWICBitmapFrameDecode> frameDecode;
    if (FAILED(decoder->GetFrame(0, &frameDecode)))
    {
        return false;
    }

    if (FAILED(frameDecode->GetSize(&size.first, &size.second)))
    {
        return false;
    }

    WICPixelFormatGUID pixelFormat = { 0 };
    if (FAILED(frameDecode->GetPixelFormat(&pixelFormat)))
    {
        return false;
    }

    // Encode outptu image
    CComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(wicFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &encoder)))
    {
        return false;
    }

    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &outputImageStream)))
    {
        return false;
    }

    if (FAILED(encoder->Initialize(outputImageStream, WICBitmapEncoderNoCache)))
    {
        return false;
    }

    CComPtr<IWICBitmapFrameEncode> frameEncode;
    if (FAILED(encoder->CreateNewFrame(&frameEncode, NULL)))
    {
        return false;
    }

    if (FAILED(frameEncode->Initialize(NULL)))
    {
        return false;
    }

    if (FAILED(frameEncode->SetSize(size.first, size.second)))
    {
        return false;
    }

    // Set 96 DPI to improve XPS print quality
    resolution.first = resolution.second = 96;
    if (FAILED(frameEncode->SetResolution(resolution.first, resolution.second)))
    {
        return false;
    }

    if (FAILED(frameEncode->SetPixelFormat(&pixelFormat)))
    {
        return false;
    }

    if (FAILED(frameEncode->WriteSource(static_cast<IWICBitmapSource*>(frameDecode), NULL)))
    {
        return false;
    }

    frameEncode->Commit();
    encoder->Commit();
    outputImageStream->Commit(STGC_DEFAULT);

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

CComPtr<IXpsOMPackage> XPSPrinter::buildPackage(IStream* imageStream, IStream* printTicketStream)
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
    if (FAILED(xpsFactory->CreatePartUri(L"/Image.png", &opcPartUri)))
    {
        return NULL;
    }

    std::pair<unsigned int, unsigned int> size;
    std::pair<double, double> resolution;
    CComPtr<IStream> processedImageStream;
    if (!processImage(imageStream, size, resolution, processedImageStream))
    {
        return NULL;
    }

    CComPtr<IXpsOMImageResource> imageResource;
    if (FAILED(xpsFactory->CreateImageResource(processedImageStream, XPS_IMAGE_TYPE_PNG, opcPartUri, &imageResource)))
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
    
    // Add PrintTicket
    opcPartUri.Release();
    if (FAILED(xpsFactory->CreatePartUri(L"/PrintTicket.xml", &opcPartUri)))
    {
        return NULL;
    }
    
    CComPtr<IXpsOMPrintTicketResource> printTicketResource;
    if (FAILED(xpsFactory->CreatePrintTicketResource(printTicketStream, opcPartUri, &printTicketResource)))
    {
        return NULL;
    }
    
    xpsFDS->SetPrintTicketResource(printTicketResource);

    return xpsPackage;
}

void XPSPrinter::print(const char* data, size_t length)
{
    // Write PrintTicket
    PrintSchema::PageMediaSize pageMediaSize;
    pageMediaSize.height = 158000;
    pageMediaSize.width = 105000;
    pageMediaSize.name = L"ns0000:User0000000501";
    
    PrintSchema::PrintCapabilities::Context printCapabilitiesContext;
    printCapabilitiesContext.namespaces[L"ns0000"] = L"http://schemas.microsoft.com/windows/printing/oemdriverpt/MITSUBISHI_CP_K60DW_S_1_0_0_0_";
    
    CComPtr<IStream> printTicketStream;
    CreateStreamOnHGlobal(NULL, TRUE, &printTicketStream);
    PrintSchema::PrintTicket::writePrintTicket(pageMediaSize, printCapabilitiesContext, printTicketStream);
    
    // Create memory image stream
    CComPtr<IStream> imageStream;
    imageStream.Attach(SHCreateMemStream((const BYTE *)data, length));

    // Build package
    CComPtr<IXpsOMPackage> package = buildPackage(imageStream, printTicketStream);
    if (!package)
    {
        return;
    }
    
    	    package->WriteToFile(L"package.xps",
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         FALSE);

						 return ;

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
