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
        xsltStylesheetPtr cStylesheet = xsltParseStylesheetFile ( (const xmlChar*) pStylesheet.c_str() );
        xmlDocPtr cInputDoc = xmlReadMemory (cInput.str().c_str(), cInput.str().length(), "teststring.xsl", 0, 0); //last to arguments are no endcoding (read from header) and no further options
        //xmlSubstituteEntitiesDefault (1);
        xmlLoadExtDtdDefaultValue = 1;
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
        xmlCleanupParser();
        xsltCleanupGlobals();

    }
    catch (std::exception& e)
    {
        pStream << RED << "Error parsing the document with libxslt: " << e.what() << RESET << std::endl;
    }

    return cHtmlResult;
}

const std::map<std::string, std::string> XMLUtils::updateHTMLForm (std::string& pFormString, std::vector<std::pair<std::string, std::string>>& pFormData, std::ostringstream& pStream, bool pStripUnchanged)
{
    //map to return containing unique pairs of strings with the settings from the form
    std::map<std::string, std::string> cFormData;
    pStream << std::endl;

    try
    {
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

            auto elements = root->find (xpath.str() );

            bool cIsChanged;

            if (elements.size() == 0)
            {

                if (cPair->first.find ("setting_") != std::string::npos && cPair->first.find ("_value") == std::string::npos)
                {
                    //we found a field that represents a new setting, in this case we need to create new nodes
                    createNewSettingNode (cPair, root, pStream);
                    pStream << BOLDYELLOW << "A new setting " << BOLDBLUE << cPair->second << BOLDYELLOW << " was added - adding to the xml buffer!" << RESET << std::endl;
                    continue;
                }
                else if (cPair->first.find ("ConditionData_") != std::string::npos && cPair->first.size() < 17 )
                {
                    //we found a new condition data field
                    createNewConditionDataNode (cPair, root, pStream);
                    pStream << BOLDYELLOW << "A new Condition Data Field " << BOLDBLUE << cPair->second << BOLDYELLOW << " was added - adding to the xml buffer!" << RESET << std::endl;
                    continue;
                }
                else
                    pStream << RED << "The element " << cPair->first << " was not found in the HTML string and needs to be added " << RESET << std::endl;
            }

            //this else needs to go once I have the node copied
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

        //proper xhtml output
        xmlChar* buff;
        int buffersize;

        xmlDocDumpFormatMemory (doc, &buff, &buffersize, 1);

        const std::string cTmpString = reinterpret_cast<const char*> (buff);
        pFormString = cTmpString;
        cleanup_after_Update (pFormString);

        delete root;
        //this worked fine on centos7 but produces a segault in SLC6...
        //xmlFreeDoc (doc);
        xmlCleanupParser();    // Free globals

    }
    catch (const std::exception& e)
    {
        pStream << RED << "Exception: " << e.what() << RESET << std::endl;
    }

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

void XMLUtils::createNewSettingNode (std::vector<std::pair<std::string, std::string>>::iterator cPair, xmlpp::Element* pRoot, std::ostringstream& pStream)
{
    //calculate the index of the new node
    size_t cPos = cPair->first.find_last_of ("_");
    std::string cNewSearchPath = cPair->first.substr (0, cPos + 1 );
    int cIndex = atoi (cPair->first.substr (cPos + 1).c_str() );

    if (cIndex > 0)
    {
        //create a new xpath expression to find the last sibling if it exists (the index is greater than 0)
        std::stringstream xpath;
        xpath << "//*[@name=\"" << cNewSearchPath << cIndex - 1 << "\"]";
        auto elements = pRoot->find (xpath.str() );

        if (!elements.size() )
        {
            //no siblings found
            pStream << RED << "Unfortunately there are no siblings for " << cPair->first << "  in the HTML string and thus the new member cant be added! " << RESET << std::endl;
            return;
        }
        else
        {
            xmlpp::Node* cParent = elements.at (0);

            for (int i = 0; i < 3; i++) // to get to the <table> tag containing the settings
                cParent = cParent->get_parent();

            xmlpp::Node* cNewRow = cParent->add_child ("tr");
            xmlpp::Node* cNewCell = cNewRow->add_child ("td");
            cNewCell = cNewCell->add_child ("input");
            xmlpp::Element* cElement = dynamic_cast<xmlpp::Element*> (cNewCell);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", cPair->first);
            cElement->set_attribute ("value", cPair->second);
            cElement->set_attribute ("size", "30");
            cNewCell = cNewRow->add_child ("td");
            cNewCell = cNewCell->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewCell);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "setting_value_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "10");
        }
    }
    else
        pStream << RED << "Unfortunately there are no siblings for " << cPair->first << "  in the HTML string and thus the new member cant be added! " << RESET << std::endl;

}

void XMLUtils::createNewConditionDataNode (std::vector<std::pair<std::string, std::string>>::iterator cPair, xmlpp::Element* pRoot, std::ostringstream& pStream)
{
    //calculate the index of the new node
    size_t cPos = cPair->first.find_last_of ("_");
    std::string cNewSearchPath = cPair->first.substr (0, cPos + 1 );
    int cIndex = atoi (cPair->first.substr (cPos + 1).c_str() );

    //has to be greater than 1 for at least one condition data to exist
    if (cIndex > 1)
    {

        //create a new xpath expression to find the last sibling if it exists (the index is greater than 0)
        std::stringstream xpath;
        xpath << "//*[@name=\"" << cNewSearchPath << cIndex - 1 << "\"]";
        auto elements = pRoot->find (xpath.str() );

        if (!elements.size() )
        {
            //no siblings found
            pStream << RED << "Unfortunately there are no siblings for " << cPair->first << "  in the HTML string and thus the new member cant be added! " << RESET << std::endl;
            return;
        }
        else
        {
            //success, let's get parsing
            xmlpp::Node* cParent = elements.at (0);

            for (int i = 0; i < 2; i++) // to get to the <ul> tag containing the condition data
                cParent = cParent->get_parent();

            xmlpp::Node* cNewli = cParent->add_child ("li");
            xmlpp::Element* cElement = dynamic_cast<xmlpp::Element*> (cNewli);
            cElement->add_child_text ("Type:  ");
            xmlpp::Node* cNewselect = cNewli->add_child ("select");
            cElement = dynamic_cast<xmlpp::Element*> (cNewselect);
            cElement->set_attribute ("name", cPair->first);
            cElement->set_attribute ("id", "conddata_" + std::to_string (cIndex) );
            cElement->set_attribute ("onchange", "DisplayFields(this.value, " + std::to_string (cIndex) + ");");
            xmlpp::Node* cNewOption = cNewselect->add_child ("option");
            cElement = dynamic_cast<xmlpp::Element*> (cNewOption);
            cElement->set_attribute ("name", cPair->second);
            //set the text node here
            cElement->add_child_text (cPair->second);
            //now add the default ones
            cNewOption = cNewselect->add_child ("option");
            cElement = dynamic_cast<xmlpp::Element*> (cNewOption);
            cElement->set_attribute ("name", "I2C");
            cElement->add_child_text ("I2C");
            cNewOption = cNewselect->add_child ("option");
            cElement = dynamic_cast<xmlpp::Element*> (cNewOption);
            cElement->set_attribute ("name", "User");
            cElement->add_child_text ("User");
            cNewOption = cNewselect->add_child ("option");
            cElement = dynamic_cast<xmlpp::Element*> (cNewOption);
            cElement->set_attribute ("name", "HV");
            cElement->add_child_text ("HV");
            cNewOption = cNewselect->add_child ("option");
            cElement = dynamic_cast<xmlpp::Element*> (cNewOption);
            cElement->set_attribute ("name", "TDC");
            cElement->add_child_text ("TDC");

            //now the label for the input for the register
            xmlpp::Node* cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_Register_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_RegisterLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("Register:");
            xmlpp::Node* cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_Register_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_Register_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "10");
            cElement->set_attribute ("style", "display:none");
            //now the UID
            cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_UID_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_UIDLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("UID:");
            cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_UID_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_UID_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "5");
            cElement->set_attribute ("style", "display:none");
            //now the FeId
            cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_FeId_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_FeIdLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("FeId:");
            cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_FeId_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_FeId_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "5");
            cElement->set_attribute ("style", "display:none");
            //now the FeId
            cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_CbcId_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_CbcIdLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("CbcId:");
            cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_CbcId_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_CbcId_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "5");
            cElement->set_attribute ("style", "display:none");
            //now the Sensor
            cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_Sensor_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_SensorLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("Sensor:");
            cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_Sensor_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_Sensor_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "5");
            cElement->set_attribute ("style", "display:none");
            cNewLabel = cNewli->add_child ("label");
            cElement = dynamic_cast<xmlpp::Element*> (cNewLabel);
            cElement->set_attribute ("for", "conddata_Value_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_ValueLabel_" + std::to_string (cIndex) );
            cElement->set_attribute ("style", "display:none");
            cElement->add_child_text ("Value:");
            cNewInput = cNewli->add_child ("input");
            cElement = dynamic_cast<xmlpp::Element*> (cNewInput);
            cElement->set_attribute ("type", "text");
            cElement->set_attribute ("name", "ConditionData_Value_" + std::to_string (cIndex) );
            cElement->set_attribute ("id", "conddata_Value_" + std::to_string (cIndex) );
            cElement->set_attribute ("value", "");
            cElement->set_attribute ("size", "5");
            cElement->set_attribute ("style", "display:none");

        }
    }
    else
        pStream << RED << "Unfortunately there are no siblings for " << cPair->first << "  in the HTML string and thus the new member cant be added! " << RESET << std::endl;
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
