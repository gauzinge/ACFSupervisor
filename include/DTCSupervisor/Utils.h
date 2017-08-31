#ifndef __Utils_H__
#define __Utils_H__

#include <iostream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <unistd.h>

#include "ConsoleColor.h"

namespace Ph2TkDAQ {

    inline std::string removeFilePrefix (const std::string& pFile)
    {
        std::string cResult;

        if (!pFile.substr (0, 7).compare ("file://") )
            cResult = pFile.substr (7);
        else cResult = pFile;

        return cResult;

    }

    inline std::string removeDot (const std::string& pWord)
    {
        std::string cWord;

        if (pWord.substr (0, 1) == ".")
            cWord = pWord.substr (1);
        else cWord = pWord;

        return cWord;
    }

    inline void cleanupHTMLString (std::string& pHTMLString, std::string search, std::string replace)
    {
        size_t pos = 0;

        while ( (pos = pHTMLString.find (search, pos) ) != std::string::npos)
        {
            //std::cout << "Found instance" << std::endl;
            pHTMLString.replace (pos, search.length(), replace );
            pos += replace.length();
        }
    }

    inline void removeLeadingLines (std::string& pString, size_t pNLines)
    {
        for (size_t i = 0; i < pNLines; i++)
            pString.erase (0, pString.find ("\n") + 1);
    }
    inline void remove_xml_header (std::string& pString, std::string pPattern)
    {
        if (pString.rfind (pPattern) != 0 )
            pString.erase (0, pString.rfind (pPattern) );
    }

    inline void cleanup_after_XSLT (std::string& pString)
    {
        //removeLeadingLines (pString, 1);
        remove_xml_header (pString, "<div onload");
        cleanupHTMLString (pString, "&lt;", "<");
    }
    inline void cleanup_after_Update (std::string& pString)
    {
        remove_xml_header (pString, "<div onload");
        cleanupHTMLString (pString, "<html>", "");
        cleanupHTMLString (pString, "</html>", "");
        cleanupHTMLString (pString, "<body>", "");
        cleanupHTMLString (pString, "</body>", "");
        cleanupHTMLString (pString, "<![CDATA[", "");
        cleanupHTMLString (pString, "]]>", "");
    }

    inline std::string cleanup_before_XSLT (std::string pString)
    {
        std::string cString = pString;
        remove_xml_header (cString, "<div onload");
        //cString.insert (0, "<!DOCTYPE html>\n");

        auto begin = cString.find ("<script");
        auto end = cString.rfind ("</script>");

        //remove the javascript part so I dont have to worry about character entities
        if (std::string::npos != begin && std::string::npos != end && begin <= end)
            cString.erase (begin, end - begin);

        cleanupHTMLString (cString, "</script>\n", "");

        return cString;
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
        return (stat (name.c_str(), &buffer) == 0);
    }

    inline std::string parseStylesheetCSS (std::string pStylesheet, std::ostringstream& pStream)
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
            std::cout << RED << "Error, CSS Stylesheet " << pStylesheet << " could not be opened!" << RESET << std::endl;

        return cResult;
    }

}
#endif
