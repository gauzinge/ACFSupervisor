/* Copyright 2017 Imperial Collge London
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Programmer :     Georg Auzinger
   Version :        1.0
   Date of creation:01/09/2017
   Support :        mail to : georg.auzinger@SPAMNOT.cern.ch
   FileName :       XMLUtils.h
*/

#ifndef __XMLUtils_H__
#define __XMLUtils_H__

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <map>

#include "ConsoleColor.h"
#include "Utils.h"

//#include <xalanc/Include/PlatformDefinitions.hpp>
//#include <xercesc/util/PlatformUtils.hpp>
//#include <xalanc/XalanTransformer/XalanTransformer.hpp>

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xmlsave.h>
#include <libxml++/libxml++.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>


namespace Ph2TkDAQ {

    class XMLUtils
    {
      public:
        //XSLT transform
        static std::string transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream, bool pIsFile = true);
        //method to update HTML string from form input
        const static std::map<std::string, std::string> updateHTMLForm (std::string& pFormString, std::vector<std::pair<std::string, std::string>>& pFormData, std::ostringstream& pStream, bool pStripUnchanged = false);

      private:
        //these return true if pValue is different form what is in the form
        static bool handle_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_select_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_input_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static void createNewSettingNode (std::vector<std::pair<std::string, std::string>>::iterator cPair, xmlpp::Element* pRoot, std::ostringstream& pStream);
        static void createNewConditionDataNode (std::vector<std::pair<std::string, std::string>>::iterator cPair, xmlpp::Element* pRoot, std::ostringstream& pStream);

        static void print_indentation (unsigned int indentation);
        static void print_node (const xmlpp::Node* node, unsigned int indentation = 0);
    };
}
#endif
