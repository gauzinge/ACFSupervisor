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
   FileName :       version.h
*/

#ifndef _ACFSupervisor_version_H_
#define _ACFSupervisor_version_H_

#include "config/PackageInfo.h"

#define ACFSUPERVISOR_VERSION_MAJOR 1
#define ACFSUPERVISOR_VERSION_MINOR 0
#define ACFSUPERVISOR_VERSION_PATCH 0
#undef ACFSUPERVISOR_PREVIOUS_VERSIONS

#define ACFSUPERVISOR_VERSION_CODE PACKAGE_VERSION_CODE(ACFSUPERVISOR_VERSION_MAJOR, ACFSUPERVISOR_VERSION_MINOR, ACFSUPERVISOR_VERSION_PATCH)
#ifndef ACFSUPERVISOR_PREVIOUS_VERSIONS
#define ACFSUPERVISOR_FULL_VERSION_LIST PACKAGE_VERSION_STRING(ACFSUPERVISOR_VERSION_MAJOR, ACFSUPERVISOR_VERSION_MINOR, ACFSUPERVISOR_VERSION_PATCH)
#else
#define ACFSUPERVISOR_FULL_VERSION_LIST ACFSUPERVISOR_PREVIOUS_VERSIONS ","
PACKAGE_VERSION_STRING (ACFSUPERVISOR_VERSION_MAJOR, ACFSUPERVISOR_VERSION_MINOR, ACFSUPERVISOR_VERSION_PATCH)
#endif

namespace ACFSupervisor {
    const std::string package = "ACFSupervisor";
    const std::string versions = ACFSUPERVISOR_FULL_VERSION_LIST;
    const std::string summary = "uTCA ACFSupervisor for Ph2 Tk DAQ";
    const std::string description = "uTCA ACFSupervisor for Ph2 Tk DAQ";
    const std::string authors = "Georg Auzinger";
    const std::string link = "http://xdaq.web.cern.ch";
    config::PackageInfo getPackageInfo();
    void checkPackageDependencies() throw (config::PackageInfo::VersionException);
    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
