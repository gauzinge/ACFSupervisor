#ifndef __DTCSupervisorXMLUtils_H__
#define __DTCSupervisorXMLUtils_H__

#include <cstdlib>
#include <string>
#include <fstream>
#include <streambuf>

#include "ConsoleColor.h"

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml++/libxml++.h>


namespace Ph2TkDAQ {

    std::string transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream)
    {
        //for the namespaces
        XALAN_USING_XERCES (XMLPlatformUtils)
        XALAN_USING_XALAN (XalanTransformer)

        //initialize the Xerces and Xalan Platforms
        XMLPlatformUtils::Initialize();
        XalanTransformer::initialize();

        //declare a transformer
        XalanTransformer cXalanTransformer;

        std::stringstream cHtmlResult;

        try
        {
            xalanc_1_11::XSLTInputSource xmlIn (pInputDocument.c_str() );
            xalanc_1_11::XSLTInputSource xslIn (pStylesheet.c_str() );
            xalanc_1_11::XSLTResultTarget xmlOut (cHtmlResult);
            int cResult = cXalanTransformer.transform (xmlIn, xslIn, xmlOut);


            if (cResult == 0)
                pStream << BLUE << "Successfully transformed xml file: " << pInputDocument << " to html using stylesheet " << pStylesheet << RESET << std::endl;
            else
                pStream << RED << "There was an Error transforming the input xml file: " << pInputDocument << " using the stylesheet " << pStylesheet << RESET << std::endl;

            XalanTransformer::terminate();
            XMLPlatformUtils::Terminate();
            XalanTransformer::ICUCleanUp();
        }
        catch (const std::exception& e)
        {
            pStream << RED << "Exception: " << e.what() << RESET << std::endl;
        }

        return cHtmlResult.str();
    }

    void print_indentation (unsigned int indentation)
    {
        for (unsigned int i = 0; i < indentation; ++i)
            std::cout << " ";
    }

    void print_node (const xmlpp::Node* node, unsigned int indentation = 0)
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


    void handle_select_node (xmlpp::Node* node, const std::string& pValue)
    {
        xmlpp::Node::NodeList list = node->get_children();
        xmlpp::Node::NodeList::iterator iter = list.begin();
        //now iter points to the first <option> tag
        xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*> (*iter);
        xmlpp::Attribute* attribute = nodeElement->get_attribute ("value");

        if (attribute == nullptr) // attribute is called name instead
            attribute = nodeElement->get_attribute ("name");

        std::cout << BOLDYELLOW << "Attribute: " << attribute->get_name() << " was: " << attribute->get_value() << " & will be: " << pValue << std::endl;
        attribute->set_value ( pValue);
        std::cout << "Check: " << attribute->get_value() << RESET <<  std::endl;
        //}
    }

    void handle_input_node (xmlpp::Node* node, const std::string& pValue)
    {
        xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*> (node);
        xmlpp::Attribute* attribute = nodeElement->get_attribute ("type");

        if (attribute == nullptr)
            std::cout << BOLDRED << "ERROR: input tag has no type attribute! - this should never happen!" << RESET << std::endl;
        else
        {
            std::string cType  = attribute->get_value();

            if (cType == "text")
            {
                attribute = nodeElement->get_attribute ("value");

                //text input
                if (attribute != nullptr) //check required for empty hidden text boxes
                {
                    std::cout << BOLDYELLOW << "Attribute: " << attribute->get_name() << " was: " << attribute->get_value() << " & will be: " << pValue << std::endl;
                    attribute->set_value ( pValue);
                    std::cout << "Check: " << attribute->get_value() << RESET <<  std::endl;
                }
            }
            else
                print_node (node);
        }
    }

    void handle_node (xmlpp::Node* node, const std::string& pValue)
    {
        std::string nodename = node->get_name();
        std::cout << GREEN << nodename << RESET << std::endl;

        if (nodename == "select")
            handle_select_node (node, pValue);
        else if (nodename == "input")
            handle_input_node (node, pValue);
    }



    void updateHTMLForm (std::string& cFormString, std::vector<std::pair<std::string, std::string>> pFormData, std::ostringstream& pStream)
    {
        // Parse HTML and create a DOM tree
        xmlDoc* doc = htmlReadDoc ( (xmlChar*) cFormString.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

        if (doc == nullptr)
            pStream << RED << "HTML HW Form string not parsed successfully." << RESET << std::endl;

        // Encapsulate raw libxml document in a libxml++ wrapper
        xmlNode* r = xmlDocGetRootElement (doc);

        if (r == nullptr)
            pStream << RED << "HTML HW Form is an empty string" << RESET << std::endl;

        xmlpp::Element* root = new xmlpp::Element (r);

        for (auto cPair : pFormData)
        {
            // Grab the node I am interested in
            std::stringstream xpath;

            //xpath << "//*[@name=\"<<locator<<\"]/p[1]/b/font/text()";
            xpath << "//*[@name=\"" << cPair.first << "\"]";

            try
            {
                auto elements = root->find (xpath.str() );
                std::cout << BOLDRED << "Find Result size: " << elements.size() << RESET << std::endl;

                for (auto element : elements)
                {
                    std::cout << BOLDBLUE << cPair.first << ":  " << cPair.second << RESET << std::endl;
                    handle_node (element, cPair.second);
                }

            }
            catch (const xmlpp::exception& e)
            {
                pStream << RED << "Exception: " << e.what() << RESET << std::endl;
            }

            //std::cout << dynamic_cast<xmlpp::ContentNode*> (elements[0])->get_content() << std::endl;
        }

        delete root;
        xmlFreeDoc (doc);
        xmlCleanupParser();    // Free globals
    }
}
#endif
