#include <atlbase.h>
#include <msxml6.h>

#include <type_traits>

#include "printticket.hpp"

using namespace PrintSchema;

const wchar_t* psfNamespace = L"http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";

bool addAttribute(IXMLDOMDocument* domDocument, IXMLDOMNode* node, const wchar_t* attribute, const wchar_t* value)
{
    CComPtr<IXMLDOMNamedNodeMap> attributesMap;
    node->get_attributes(&attributesMap);

    CComPtr<IXMLDOMNode> attributeNode;
    domDocument->createNode(CComVariant(NODE_ATTRIBUTE), CComBSTR(attribute), CComBSTR(L""), &attributeNode);
    attributeNode->put_nodeValue(CComVariant(value));

    attributesMap->setNamedItem(attributeNode, NULL);

    return true;
}

bool createPSFElement(IXMLDOMDocument* domDocument, const wchar_t* tagName, const wchar_t* name, CComPtr<IXMLDOMNode>& node)
{
    domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(tagName), CComBSTR(psfNamespace), &node);
    addAttribute(domDocument, node, L"name", name);

    return true;
}

bool createPSFValueElement(IXMLDOMDocument* domDocument, unsigned int value, CComPtr<IXMLDOMNode>& node)
{
    domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(L"psf:Value"), CComBSTR(psfNamespace), &node);

    std::wstring stringValue = std::to_wstring(value);
    CComBSTR BSTRValue(stringValue.c_str());

    node->put_text(BSTRValue);

    CComPtr<IXMLDOMNode> typeNode;
    domDocument->createNode(CComVariant(NODE_ATTRIBUTE), CComBSTR(L"xsi:type"), CComBSTR(L"http://www.w3.org/2001/XMLSchema-instance"), &typeNode);
    typeNode->put_nodeValue(CComVariant(L"xsd:integer"));

    CComPtr<IXMLDOMNamedNodeMap> attributesMap;
    node->get_attributes(&attributesMap);

    attributesMap->setNamedItem(typeNode, NULL);

    return true;
}

bool PrintTicket::writePrintTicket(PageMediaSize const& pageMediaSize,
                                   PrintCapabilities::Context const& context,
                                   IStream* printTicketStream)
{
    // Create DOM Document
    CComPtr<IXMLDOMDocument> domDocument;
    domDocument.CoCreateInstance(__uuidof(DOMDocument60));

    // Add Processing Instruction
    CComPtr<IXMLDOMProcessingInstruction> processingInstruction;
    domDocument->createProcessingInstruction(L"xml", L"version='1.0'", &processingInstruction);
    domDocument->appendChild(processingInstruction, NULL);

    // Create PrintTicket root node
    CComPtr<IXMLDOMNode> printTicketNode;
    domDocument->createNode(CComVariant(NODE_ELEMENT), CComBSTR(L"psf:PrintTicket"), CComBSTR(psfNamespace), &printTicketNode);

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
        printTicketNode->appendChild(node, &pageMediaSizeFeatureNode);
    }

    CComPtr<IXMLDOMNode> optionNode;
    {
        CComPtr<IXMLDOMNode> node;
        createPSFElement(domDocument, L"psf:Option", pageMediaSize.name.c_str(), node);
        pageMediaSizeFeatureNode->appendChild(node, &optionNode);
    }

    CComPtr<IXMLDOMNode> mediaSizeHeightNode;
    {
        CComPtr<IXMLDOMNode> node;
        createPSFElement(domDocument, L"psf:ScoredProperty", L"psk:MediaSizeHeight", node);
        optionNode->appendChild(node, &mediaSizeHeightNode);
    }

    {
        CComPtr<IXMLDOMNode> value;
        createPSFValueElement(domDocument, pageMediaSize.height, value);
        mediaSizeHeightNode->appendChild(value, NULL);
    }

    CComPtr<IXMLDOMNode> mediaSizeWidthNode;
    {
        CComPtr<IXMLDOMNode> node;
        createPSFElement(domDocument, L"psf:ScoredProperty", L"psk:MediaSizeWidth", node);
        optionNode->appendChild(node, &mediaSizeWidthNode);
    }

    {
        CComPtr<IXMLDOMNode> value;
        createPSFValueElement(domDocument, pageMediaSize.width, value);
        mediaSizeWidthNode->appendChild(value, NULL);
    }

    // Create PageOrientation
    CComPtr<IXMLDOMNode> pageOrientationFeatureNode;
    {
        CComPtr<IXMLDOMNode> node;
        createPSFElement(domDocument, L"psf:Feature", L"psk:PageOrientation", node);
        printTicketNode->appendChild(node, &pageOrientationFeatureNode);
    }

    {
        CComPtr<IXMLDOMNode> node;
        createPSFElement(domDocument, L"psf:Option", L"psk:Landscape", node);
        pageOrientationFeatureNode->appendChild(node, NULL);
    }

    domDocument->appendChild(printTicketNode, NULL);

    domDocument->save(CComVariant(printTicketStream));

    return true;
}
