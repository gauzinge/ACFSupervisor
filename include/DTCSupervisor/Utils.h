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
   FileName :       Utils.h
*/

#ifndef __Utils_H__
#define __Utils_H__

#include <iostream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
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

    inline std::string appendSlash (const std::string& pFilePath)
    {
        std::string cResult;

        if (pFilePath.back() != '/')
            cResult = pFilePath + "/";
        else
            cResult = pFilePath;

        return cResult;
    }

    inline std::string removeDot (const std::string& pWord)
    {
        std::string cWord;

        if (pWord.find (".") == 0) //if string starts with a dot
        {
            size_t cPos = pWord.find_first_not_of (".");
            cWord = pWord.substr (cPos);
        }
        else
            cWord = pWord;

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
        //if (pString.rfind (pPattern) != 0 )
        //pString.erase (0, pString.rfind (pPattern) );
        if (pString.find (pPattern) != std::string::npos )
            pString.erase (0, pString.find (pPattern) );
    }

    inline void cleanup_after_XSLT (std::string& pString)
    {
        remove_xml_header (pString, "<div onload");
    }
    inline void cleanup_after_XSLT_Settings (std::string& pString)
    {
        remove_xml_header (pString, "<table");
    }

    inline void cleanup_after_Update (std::string& pString)
    {
        if (pString.find ("<div onload") != std::string::npos)
            remove_xml_header (pString, "<div onload");
        else if (pString.find ("<table") != std::string::npos)
            remove_xml_header (pString, "<table");

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
        return cString;
    }
    inline std::string cleanup_before_XSLT_Settings (std::string pString)
    {
        std::string cString = pString;
        remove_xml_header (cString, "<table");
        return cString;
    }

    inline void cleanup_log_string (std::string& pLogString)
    {
        cleanupHTMLString (pLogString, "\n", "<br/>");
        cleanupHTMLString (pLogString, RESET, "</span>");
        cleanupHTMLString (pLogString, BOLDYELLOW, "</span><span style=\"color:red\">");
        cleanupHTMLString (pLogString, YELLOW, "</span><span style=\"color:red\">");
        cleanupHTMLString (pLogString, BOLDBLUE, "</span><span style=\"color:blue\">");
        cleanupHTMLString (pLogString, BLUE, "</span><span style=\"color:blue\">");
        cleanupHTMLString (pLogString, BOLDRED, "</span><span style=\"color:red\">");
        cleanupHTMLString (pLogString, RED, "</span><span style=\"color:red\">");
        cleanupHTMLString (pLogString, BOLDGREEN, "</span><span style=\"color:green\">");
        cleanupHTMLString (pLogString, GREEN, "</span><span style=\"color:green\">");
        cleanupHTMLString (pLogString, BOLDMAGENTA, "</span><span style=\"color:magenta\">");
        cleanupHTMLString (pLogString, MAGENTA, "</span><span style=\"color:magenta\">");
        cleanupHTMLString (pLogString, BOLDCYAN, "</span><span style=\"color:magenta\">");
        cleanupHTMLString (pLogString, CYAN, "</span><span style=\"color:magenta\">");
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

    inline bool mkdir (const std::string& pPath)
    {
        struct stat buffer;

        if (stat (pPath.c_str(), &buffer) != 0)
        {
            //create the directory as it does not exist
            std::stringstream cCommand;
            cCommand << "mkdir " << pPath << std::endl;
            std::system (cCommand.str().c_str() );
            return false;
        }
        else if (buffer.st_mode & S_IFDIR)
        {
            //this is a directory
            return true;
        }
        else
            //it's a file or whatever
            return false;
    }

    inline std::string parseExternalResource (std::string pStylesheet, std::ostream& pStream = std::cout)
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

    inline std::string inttostring (const int& pInt)
    {
        return std::to_string (pInt);
    }
    inline int stringtoint (const char* pString)
    {
        int val = 0;

        while ( *pString )
            val = val * 10 + (*pString++ - '0');

        return val;
    }

    inline void complete_file_paths ( std:: string& pXMLString )
    {
        //try to initialize the HWDescription from the GUI's fXMLHWString
        //make sure that the address_table_string is expanded with the correct environment variable
        //if we don't find an environment variable in the path to the address tabel, or the path does not point to Ph2_ACF root we prepend the Ph2ACF ROOT
        if (pXMLString.find ("file:://${") == std::string::npos || pXMLString.find ("file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") + "/";
            cleanupHTMLString (pXMLString, "file://", cCorrectPath);
        }

        //the same goes for the CBC File Path
        if (pXMLString.find ("<CBC_Files path=\"${") == std::string::npos || pXMLString.find ("<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}");
            cleanupHTMLString (pXMLString, "<CBC_Files path=\".", cCorrectPath);
        }
    }

    inline bool find_run_number (std::string pFilename, int& pRunNumber)
    {
        //extract a run number from a filename and return true if there is one and false if not
        //the run number is 5 digits and found before the underscore
        if (pFilename.find ("_") == std::string::npos) return false;
        else
        {
            size_t pos = pFilename.find ("_");
            std::string cNumString = pFilename.substr (pos + 1, 5);
            pRunNumber = atoi (cNumString.c_str() );
            return true;
        }
    }
}
#endif
