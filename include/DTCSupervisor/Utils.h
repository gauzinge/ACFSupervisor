#ifndef __DTCSupervisorUtils_H__
#define __DTCSupervisorUtils_H__

#include <cstdlib>
#include <string>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <unistd.h>

#include "ConsoleColor.h"

#include "xmlwrapp/xmlwrapp.h"
#include "xsltwrapp/xsltwrapp.h"

namespace Ph2TkDAQ {

    inline std::string removeFilePrefix (const std::string& pFile)
    {
        std::string cResult;

        if (!pFile.substr (0, 7).compare ("file://") )
            cResult = pFile.substr (7);
        else cResult = pFile;

        return cResult;

    }

    static std::string expandEnvironmentVariables (  std::string s )
    {
        s.erase (std::remove_if (s.begin(), s.end(), ::isspace), s.end() );

        if ( s.find ( "${" ) == std::string::npos ) return s;

        std::string pre  = s.substr ( 0, s.find ( "${" ) );
        std::string post = s.substr ( s.find ( "${" ) + 2 );

        if ( post.find ( '}' ) == std::string::npos ) return s;

        std::string variable = post.substr ( 0, post.find ( '}' ) );
        std::string value    = "";

        post = post.substr ( post.find ( '}' ) + 1 );

        if ( getenv ( variable.c_str() ) != NULL ) value = std::string ( getenv ( variable.c_str() ) );

        return expandEnvironmentVariables ( pre + value + post );
    }

    inline bool checkFile (const std::string& name)
    {
        struct stat buffer;
        std::cout << stat (name.c_str(), & buffer) << std::endl;
        return (stat (name.c_str(), &buffer) == 0);
    }

    std::string parseStylesheetCSS (std::string pStylesheet, std::ostringstream& pStream)
    {
        std::string cResult;

        std::ifstream cStyleStream;
        cStyleStream.open (pStylesheet.c_str() );

        if (cStyleStream.is_open() )
        {
            cStyleStream.seekg (0, std::ios::end);
            cResult.reserve (cStyleStream.tellg() );
            cStyleStream.seekg (0, std::ios::beg);
            cResult.assign ( (std::istreambuf_iterator<char> (cStyleStream) ), std::istreambuf_iterator<char>() );
            //pStream << "Successfully parsed CSS Stylesheet " << pStylesheet << std::endl;
        }
        else
            pStream << RED << "Error, CSS Stylesheet " << pStylesheet << " could not be opened!" << RESET << std::endl;


        return cResult;
    }

    std::string transformXmlDocument (const std::string& pInputDocument, const std::string& pStylesheet, std::ostringstream& pStream)
    {
        std::string cHtmlResult;

        try
        {
            xml::tree_parser cParser (pInputDocument.c_str() );
            xml::document& cDoc = cParser.get_document();
            xslt::stylesheet cStylesheet (pStylesheet.c_str() );
            xml::document& cResult = cStylesheet.apply (cDoc);
            cResult.save_to_string (cHtmlResult);
            pStream << BLUE << "Successfully transformed xml file: " << pInputDocument << " to html using stylesheet " << pStylesheet << RESET << std::endl;
        }

        catch (const std::exception& e)
        {
            pStream << RED << "Exception: " << e.what() << RESET << std::endl;
        }

        return cHtmlResult;
    }

}
#endif
