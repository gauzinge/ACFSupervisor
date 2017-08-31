#include "DTCSupervisor/XMLUtils.h"

using namespace Ph2TkDAQ;

std::string XMLUtils::transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream, bool pIsFile)
{
    //extern int xmlLoadExtDtdDefaultValue;
    //input
    std::stringstream cInput;
    //output
    std::string cHtmlResult;

    if (pIsFile)
    {
        std::ifstream cInFile (pInputDocument.c_str() );

        if (cInFile)
        {
            cInput << cInFile.rdbuf();
            cInFile.close();
        }
    }
    else
        cInput.str (pInputDocument);

    try
    {
        xmlDocPtr cInputDoc = xmlReadMemory (cInput.str().c_str(), cInput.str().length(), "teststring.xsl", 0, 0); //last to arguments are no endcoding (read from header) and no further options
        //xmlSubstituteEntitiesDefault (1);
        xmlLoadExtDtdDefaultValue = 1;
        xsltStylesheetPtr cStylesheet = xsltParseStylesheetFile ( (const xmlChar*) pStylesheet.c_str() );
        xmlDocPtr cOutput = xsltApplyStylesheet (cStylesheet, cInputDoc, 0);

        //save the output to string
        xmlChar* xmlbuff;
        int buffersize;
        xmlDocDumpFormatMemory (cOutput, &xmlbuff, &buffersize, 1);
        const std::string cTmpString = reinterpret_cast<const char*> (xmlbuff);
        cHtmlResult = cTmpString;

        xmlFreeDoc (cInputDoc);
        xmlFreeDoc (cOutput);
        xsltFreeStylesheet (cStylesheet);

    }
    catch (std::exception& e)
    {
        pStream << RED << "Error parsing the document with libxslt: " << e.what() << RESET << std::endl;
    }

    xmlCleanupParser();
    xsltCleanupGlobals();
    return cHtmlResult;
}

const std::map<std::string, std::string> XMLUtils::updateHTMLForm (std::string& pFormString, std::vector<std::pair<std::string, std::string>>& pFormData, std::ostringstream& pStream, bool pStripUnchanged)
{
    //map to return containing unique pairs of strings with the settings from the form
    std::map<std::string, std::string> cFormData;
    pStream << std::endl;
    // Parse HTML and create a DOM tree
    xmlDoc* doc = htmlReadDoc ( (xmlChar*) pFormString.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr)
        pStream << RED << "HTML HW Form string not parsed successfully." << RESET << std::endl;

    // Encapsulate raw libxml document in a libxml++ wrapper
    xmlNode* r = xmlDocGetRootElement (doc);

    if (r == nullptr)
        pStream << RED << "HTML HW Form is an empty string" << RESET << std::endl;

    xmlpp::Element* root = new xmlpp::Element (r);

    auto cPair = std::begin (pFormData);

    while (cPair != std::end (pFormData) )
    {
        // Grab the node I am interested in
        std::stringstream xpath;

        xpath << "//*[@name=\"" << cPair->first << "\"]";

        try
        {
            auto elements = root->find (xpath.str() );

            bool cIsChanged;

            for (auto element : elements)
            {
                std::string cOldValue = "";
                cIsChanged = handle_node (element, cOldValue, cPair->second);

                if (cIsChanged)
                {
                    pStream << BOLDYELLOW << "Node " << BOLDBLUE << cPair->first << BOLDYELLOW << " has changed from " << BOLDBLUE << cOldValue << BOLDYELLOW << " new value is: " << BOLDRED << cPair->second << RESET << std::endl;
                    break;
                }
            }

            //if the node has  not changed and i want to strip the unchanged ones
            //erase it from the input vector
            if (pStripUnchanged)
            {
                if (!cIsChanged)
                    cPair = pFormData.erase (cPair);
                // the node has changed and i want to strip unchanged
                else
                {
                    cFormData[cPair->first] = cPair->second;
                    cPair++;
                }
            }
            else
            {
                cFormData[cPair->first] = cPair->second;
                cPair++;
            }
        }
        catch (const std::exception& e)
        {
            pStream << RED << "Exception: " << e.what() << RESET << std::endl;
        }
    }

    //proper xhtml output
    xmlChar* buff;
    int buffersize;

    xmlDocDumpFormatMemory (doc, &buff, &buffersize, 1);

    const std::string cTmpString = reinterpret_cast<const char*> (buff);
    pFormString = cTmpString;
    cleanup_after_Update (pFormString);

    delete root;
    xmlFreeDoc (doc);
    xmlCleanupParser();    // Free globals

    return cFormData;
    //way of retaining the input
    //https://stackoverflow.com/questions/41576852/write-htmldocptr-to-string
    //xmlBufferPtr buffer = xmlBufferCreate();

    //if (buffer == nullptr)
    //{
    //pStream << RED << "Error creating buffer for updated HTML form" << RESET << std::endl;
    //return;
    //}

    //xmlSaveCtxtPtr saveCtxtPtr = xmlSaveToBuffer (buffer, NULL, XML_SAVE_NO_DECL);

    //if (xmlSaveDoc (saveCtxtPtr, doc) < 0)
    //{
    //pStream << RED << "Error saving updated HTML form" << RESET << std::endl;
    //return;
    //}

    //xmlSaveClose (saveCtxtPtr);
    //const xmlChar* xmlCharBuffer = xmlBufferContent (buffer);
    //const std::string cTmpString = reinterpret_cast<const char*> (xmlCharBuffer);
    //pFormString = cTmpString;
    //cleanup_after_Update (pFormString);
}

bool XMLUtils::handle_select_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue)
{
    bool cIsChanged = false;
    //in the form as it is implemented, the current value will always be shown as the first option
    //thus all we need to to is get the first option tag, change it's value and corresponding node text
    xmlpp::Node::NodeList list = node->get_children();
    xmlpp::Node::NodeList::iterator iter = list.begin();

    //now iter points to the first <option> tag: we need to modify it's value and node text
    for (; iter != std::end (list); iter++)
    {
        if ( (*iter)->get_name() == "option")
        {
            xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*> (*iter);

            if (nodeElement != nullptr)
            {
                xmlpp::TextNode* nodeText;
                xmlpp::Attribute* attribute = nodeElement->get_attribute ("value");

                if (attribute == nullptr) // attribute is called name instead
                    attribute = nodeElement->get_attribute ("name");

                if (attribute != nullptr && attribute->get_value() != pValue)
                {
                    cIsChanged = true;

                    if (nodeElement->has_child_text() )
                    {
                        nodeText = nodeElement->get_child_text();
                        nodeText->set_content (pValue);
                    }

                    pOldValue = attribute->get_value();
                    attribute->set_value ( pValue);
                }
            }

            //since I just need to treat the first option
            break;
        }
    }

    return cIsChanged;
}

bool XMLUtils::handle_input_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue)
{
    bool cDifferent = false;
    xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*> (node);
    xmlpp::Attribute* attribute = nodeElement->get_attribute ("type");

    if (attribute == nullptr)
        std::cout << BOLDRED << "ERROR: input tag has no type attribute! - this should never happen!" << RESET << std::endl;
    else
    {
        std::string cType  = attribute->get_value();

        if (cType == "text" || cType == "number")
        {
            attribute = nodeElement->get_attribute ("value");

            //text input
            if (attribute != nullptr) //check required for empty hidden text boxes
            {
                if (attribute->get_value() != pValue)
                {
                    cDifferent = true;
                    pOldValue = attribute->get_value();
                    attribute->set_value ( pValue);
                }
            }
        }
        else
            print_node (node);
    }

    return cDifferent;
}

bool XMLUtils::handle_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue)
{
    std::string nodename = node->get_name();

    bool cDifferent = false;

    if (nodename == "select")
        cDifferent = handle_select_node (node, pOldValue, pValue);
    else if (nodename == "input")
        cDifferent = handle_input_node (node, pOldValue, pValue);

    return cDifferent;
}

void XMLUtils::print_indentation (unsigned int indentation)
{
    for (unsigned int i = 0; i < indentation; ++i)
        std::cout << " ";
}

void XMLUtils::print_node (const xmlpp::Node* node, unsigned int indentation)
{
    std::cout << std::endl; //Separate nodes by an empty line.

    const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*> (node);
    const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*> (node);
    const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*> (node);

    if (nodeText && nodeText->is_white_space() ) //Let's ignore the indenting - you don't always want to do this.
        return;

    Glib::ustring nodename = node->get_name();

    if (!nodeText && !nodeComment && !nodename.empty() ) //Let's not say "name: text".
    {
        print_indentation (indentation);
        std::cout << "Node name = " << nodename << std::endl;
    }
    else if (nodeText) //Let's say when it's text. - e.g. let's say what that white space is.
    {
        print_indentation (indentation);
        std::cout << "Text Node" << std::endl;
    }

    //Treat the various node types differently:
    if (nodeText)
    {
        print_indentation (indentation);
        std::cout << "text = \"" << nodeText->get_content() << "\"" << std::endl;
    }
    else if (nodeComment)
    {
        print_indentation (indentation);
        std::cout << "comment = " << nodeComment->get_content() << std::endl;
    }
    else if (nodeContent)
    {
        print_indentation (indentation);
        std::cout << "content = " << nodeContent->get_content() << std::endl;
    }
    else if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*> (node) )
    {
        //A normal Element node:

        //line() works only for ElementNodes.
        print_indentation (indentation);
        std::cout << "     line = " << node->get_line() << std::endl;

        //Print attributes:
        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();

        for (xmlpp::Element::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
        {
            const xmlpp::Attribute* attribute = *iter;
            print_indentation (indentation);
            std::cout << "  Attribute " << attribute->get_name() << " = " << attribute->get_value() << std::endl;
        }

        const xmlpp::Attribute* attribute = nodeElement->get_attribute ("title");

        if (attribute)
            std::cout << "title found: =" << attribute->get_value() << std::endl;

    }

    if (!nodeContent)
    {
        //Recurse through child nodes:
        xmlpp::Node::NodeList list = node->get_children();

        for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
        {
            print_node (*iter, indentation + 2); //recursive
        }
    }
}
//implementation of XSLT transform using XDAQ version of XALAN which unfortunately crashes when transformin html to xml
//std::string XMLUtils::transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream, bool pIsFile)
//{
//pStream << std::endl;
////for the namespaces
//XALAN_USING_XERCES (XMLPlatformUtils)
//XALAN_USING_XALAN (XalanTransformer)

//std::stringstream cInput;
//std::stringstream cHtmlResult;

//if (pIsFile)
//{
//std::ifstream cInFile (pInputDocument.c_str() );

//if (cInFile)
//{
//cInput << cInFile.rdbuf();
//cInFile.close();
//}
//}
//else
//cInput.str (pInputDocument);

//try
//{
////initialize the Xerces and Xalan Platforms
//XMLPlatformUtils::Initialize();
//XalanTransformer::initialize();

////declare a transformer
//XalanTransformer cXalanTransformer;

//xalanc_1_11::XSLTInputSource xmlIn (&cInput);
//xalanc_1_11::XSLTInputSource xslIn (pStylesheet.c_str() );

////workaround for segfault caused by input being strinstream
////https://marc.info/?l=xalan-c-users&m=101177041513073
//const xalanc_1_11::XalanDOMString theSystemID ("Stream input");
//xmlIn.setSystemId (c_wstr (theSystemID) );
////xslIn.setSystemId (c_wstr (theSystemID) );

//xalanc_1_11::XSLTResultTarget xmlOut (cHtmlResult);

//if (cXalanTransformer.transform (xmlIn, xslIn, xmlOut) != 0)
//pStream << RED << "XSLT Error: " << cXalanTransformer.getLastError() << RESET << std::endl;
//else
//pStream << BLUE << "Successfully transformed xml file: " << pInputDocument << " to html using stylesheet " << pStylesheet << RESET << std::endl;

//XalanTransformer::terminate();
//XMLPlatformUtils::Terminate();
//XalanTransformer::ICUCleanUp();
//}
//catch (const std::exception& e)
//{
//pStream << RED << "Exception: " << e.what() << RESET << std::endl;
//}

//return cHtmlResult.str();
//}
