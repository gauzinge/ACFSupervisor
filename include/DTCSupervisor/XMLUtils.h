#ifndef __XMLUtils_H__
#define __XMLUtils_H__

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

#include "ConsoleColor.h"

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xmlsave.h>
#include <libxml++/libxml++.h>


namespace Ph2TkDAQ {

    class XMLUtils
    {
      public:
        static std::string transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream);
        const static void updateHTMLForm (std::string& pFormString, std::vector<std::pair<std::string, std::string>>& pFormData, std::ostringstream& pStream, bool pStripUnchanged = false);
      private:
        //these return true if pValue is different form what is in the form
        static bool handle_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_select_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_input_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);

        static void print_indentation (unsigned int indentation);
        static void print_node (const xmlpp::Node* node, unsigned int indentation = 0);
    };
}
#endif
