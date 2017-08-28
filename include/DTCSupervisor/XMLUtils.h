#ifndef __XMLUtils_H__
#define __XMLUtils_H__

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <map>

#include "ConsoleColor.h"

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xmlsave.h>
#include <libxml++/libxml++.h>


namespace Ph2TkDAQ {
    struct LUTEntry
    {
        std::string fXPath;
        std::string fHWTreeLevel;
        int fBeId, fFeId, fCbcId;
        LUTEntry (std::string pXPath, std::string pHWTreeLevel, int BeId, int FeId, int CbcId ) :
            fXPath (pXPath),
            fHWTreeLevel (pHWTreeLevel),
            fBeId (BeId),
            fFeId (FeId),
            fCbcId (CbcId) {}
    };

    class XMLUtils
    {
      public:
        //XSLT transform
        static std::string transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream);
        //method to update HTML string from form input
        const static std::map<std::string, std::string> updateHTMLForm (std::string& pFormString, std::vector<std::pair<std::string, std::string>>& pFormData, std::ostringstream& pStream, bool pStripUnchanged = false);

      private:
        //these return true if pValue is different form what is in the form
        static bool handle_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_select_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);
        static bool handle_input_node (xmlpp::Node* node, std::string& pOldValue, const std::string& pValue);

        static void print_indentation (unsigned int indentation);
        static void print_node (const xmlpp::Node* node, unsigned int indentation = 0);

      public:
        //method to get the HWDescription.xml into a string
        //static std::string getXMLString (const std::string& pFilename, std::ostringstream& pStream);
        //method to update xml HWDescription string based on LUT
        //static void updateXMLString (std::string& pFormString, std::map<std::string, std::string> pFormValues, std::ostringstream& pStream);
        //method to parse an XPath Lut
        //static std::map<std::string, LUTEntry> parseLUT (const std::string& pFilename, std::ostringstream& pStream);
        //method to get a LUT entry
        //static LUTEntry getEntry (const std::map<std::string, LUTEntry>& pMap, const std::string& pFormNodeName)
        //{
        //if (pMap.find (pFormNodeName) != std::end (pMap) )
        //return pMap.find (pFormNodeName);
        //else return 0;
        //}
    };
}
#endif
