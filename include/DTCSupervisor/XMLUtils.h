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
}

#endif
