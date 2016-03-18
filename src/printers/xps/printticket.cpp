#include <atlbase.h>
#include <msxml6.h>

#include "throwablehresult.hpp"
#include "printticket.hpp"

using namespace PrintSchema;

const wchar_t* psfNamespace = L"http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";

void addAttribute(IXMLDOMDocument* domDocument, IXMLDOMNode* node, const wchar_t* attribute, const wchar_t* value)
{
    ThrowableHResult thr;

    CComPtr<IXMLDOMNamedNodeMap> attributesMap;
    thr = node->get_attributes(&attributesMap);

    CComPtr<IXMLDOMNode> attributeNode;
    thr = domDocument->createNode(CComVariant(NODE_ATTRIBUTE), CComBSTR(attribute), CComBSTR(L""), &attributeNode);
    thr = attributeNode->put_nodeValue(CComVariant(value));

    thr = attributesMap->setNamedItem(attributeNode, NULL);
}

void createPSFElement(IXMLDOMDocument* domDocument, const wchar_t* tagName, const wchar_t* name, CComPtr<IXMLDOMNode>& node)
{
    ThrowableHResult thr;

    thr = domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(tagName), CComBSTR(psfNamespace), &node);
    addAttribute(domDocument, node, L"name", name);
}

void createPSFValueElement(IXMLDOMDocument* domDocument, unsigned int value, CComPtr<IXMLDOMNode>& node)
{
    ThrowableHResult thr;

    thr = domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(L"psf:Value"), CComBSTR(psfNamespace), &node);

    std::wstring stringValue = std::to_wstring(value);
    CComBSTR BSTRValue(stringValue.c_str());

    thr = node->put_text(BSTRValue);

    CComPtr<IXMLDOMNode> typeNode;
    thr = domDocument->createNode(CComVariant(NODE_ATTRIBUTE), CComBSTR(L"xsi:type"), CComBSTR(L"http://www.w3.org/2001/XMLSchema-instance"), &typeNode);
    thr = typeNode->put_nodeValue(CComVariant(L"xsd:integer"));

    CComPtr<IXMLDOMNamedNodeMap> attributesMap;
    thr = node->get_attributes(&attributesMap);

    thr = attributesMap->setNamedItem(typeNode, NULL);
}

bool PrintTicket::writePrintTicket(PageMediaSize const& pageMediaSize,
                                   PrintCapabilities::Context const& context,
                                   IStream* printTicketStream)
{
    ThrowableHResult thr;

    try
    {
        // Create DOM Document
        CComPtr<IXMLDOMDocument> domDocument;
        thr = domDocument.CoCreateInstance(__uuidof(DOMDocument60));

        // Add Processing Instruction
        CComPtr<IXMLDOMProcessingInstruction> processingInstruction;
        thr = domDocument->createProcessingInstruction(L"xml", L"version='1.0'", &processingInstruction);
        thr = domDocument->appendChild(processingInstruction, NULL);

        // Create PrintTicket root node
        CComPtr<IXMLDOMNode> printTicketNode;
        thr = domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(L"psf:PrintTicket"), CComBSTR(psfNamespace), &printTicketNode);

        // Add namespaces
        addAttribute(domDocument, printTicketNode, L"xmlns:xsi", L"http://www.w3.org/2001/XMLSchema-instance");
        addAttribute(domDocument, printTicketNode, L"xmlns:xsd", L"http://www.w3.org/2001/XMLSchema");
        addAttribute(domDocument, printTicketNode, L"xmlns:psk", L"http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords");

        std::wstring stringVersion = std::to_wstring(context.version);
        addAttribute(domDocument, printTicketNode, L"version", stringVersion.c_str());

        addAttribute(domDocument, printTicketNode, L"xmlns:ns0000", L"http://schemas.microsoft.com/windows/printing/oemdriverpt/MITSUBISHI_CP_K60DW_S_1_0_0_0_");

        // Create PageMediaSize
        CComPtr<IXMLDOMNode> pageMediaSizeFeatureNode;
        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:Feature", L"psk:PageMediaSize", node);
            thr = printTicketNode->appendChild(node, &pageMediaSizeFeatureNode);
        }

        CComPtr<IXMLDOMNode> optionNode;
        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:Option", pageMediaSize.name.c_str(), node);
            thr = pageMediaSizeFeatureNode->appendChild(node, &optionNode);
        }

        CComPtr<IXMLDOMNode> mediaSizeHeightNode;
        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:ScoredProperty", L"psk:MediaSizeHeight", node);
            thr = optionNode->appendChild(node, &mediaSizeHeightNode);
        }

        {
            CComPtr<IXMLDOMNode> value;
            createPSFValueElement(domDocument, pageMediaSize.height, value);
            thr = mediaSizeHeightNode->appendChild(value, NULL);
        }

        CComPtr<IXMLDOMNode> mediaSizeWidthNode;
        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:ScoredProperty", L"psk:MediaSizeWidth", node);
            thr = optionNode->appendChild(node, &mediaSizeWidthNode);
        }

        {
            CComPtr<IXMLDOMNode> value;
            createPSFValueElement(domDocument, pageMediaSize.width, value);
            thr = mediaSizeWidthNode->appendChild(value, NULL);
        }

        // Create PageOrientation
        CComPtr<IXMLDOMNode> pageOrientationFeatureNode;
        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:Feature", L"psk:PageOrientation", node);
            thr = printTicketNode->appendChild(node, &pageOrientationFeatureNode);
        }

        {
            CComPtr<IXMLDOMNode> node;
            createPSFElement(domDocument, L"psf:Option", L"psk:Landscape", node);
            thr = pageOrientationFeatureNode->appendChild(node, NULL);
        }

        thr = domDocument->appendChild(printTicketNode, NULL);

        thr = domDocument->save(CComVariant(printTicketStream));
    }
    catch (ThrowableHResult::HResultFailed const&)
    {
        return false;
    }

    return true;
}
