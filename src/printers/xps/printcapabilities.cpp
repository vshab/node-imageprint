#include "printcapabilities.hpp"

#include <atlbase.h>
#include <msxml6.h>
#include <stdlib.h>

#include "throwablehresult.hpp"

using namespace PrintSchema;

unsigned int getPSFPropertyValue(IXMLDOMNode* node, const wchar_t* name)
{
    const std::wstring queryText = std::wstring() + L"psf:Property[@name='" + name + L"']/psf:Value";

    ThrowableHResult thr;

    CComPtr<IXMLDOMNode> valueNode;
    CComBSTR query(queryText.c_str());
    thr = node->selectSingleNode(query, &valueNode);

    CComBSTR value;
    thr = valueNode->get_text(&value);

    return _wtoi(value);
}

bool PrintCapabilities::getPageImageableSize(IStream* printCapabilitiesStream, PageImageableSize& pageImageableSize)
{
    ThrowableHResult thr;

    try
    {
        CComPtr<IXMLDOMDocument> xmlDomDocument;
        thr = xmlDomDocument.CoCreateInstance(__uuidof(DOMDocument));

        VARIANT_BOOL success = VARIANT_TRUE;
        thr = xmlDomDocument->load(CComVariant(printCapabilitiesStream), &success);
        if (success == VARIANT_FALSE)
        {
            return false;
        }

        CComPtr<IXMLDOMNode> pageImageableSizeNode;
        {
            CComBSTR query(L"//psf:Property[@name='psk:PageImageableSize']");
            thr = xmlDomDocument->selectSingleNode(query, &pageImageableSizeNode);
        }

        pageImageableSize.width = getPSFPropertyValue(pageImageableSizeNode, L"psk:ImageableSizeWidth");
        pageImageableSize.height = getPSFPropertyValue(pageImageableSizeNode, L"psk:ImageableSizeHeight");

        CComPtr<IXMLDOMNode> imageableAreaNode;
        {
            CComBSTR query(L"//psf:Property[@name='psk:ImageableArea']");
            thr = xmlDomDocument->selectSingleNode(query, &imageableAreaNode);
        }

        pageImageableSize.imageableArea.extentHeight = getPSFPropertyValue(imageableAreaNode, L"psk:ExtentHeight");
        pageImageableSize.imageableArea.extentWidth = getPSFPropertyValue(imageableAreaNode, L"psk:ExtentWidth");
        pageImageableSize.imageableArea.originHeight = getPSFPropertyValue(imageableAreaNode, L"psk:OriginHeight");
        pageImageableSize.imageableArea.originWidth = getPSFPropertyValue(imageableAreaNode, L"psk:OriginWidth");
    }
    catch (ThrowableHResult::HResultFailed const&)
    {
        return false;
    }

    return true;
}
