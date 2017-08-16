#ifndef _DTCSupervisor_version_H_
#define _DTCSupervisor_version_H_

#include "config/PackageInfo.h"

#define DTCSUPERVISOR_VERSION_MAJOR 0
#define DTCSUPERVISOR_VERSION_MINOR 1
#define DTCSUPERVISOR_VERSION_PATCH 0
#undef DTCSUPERVISOR_PREVIOUS_VERSIONS

#define DTCSUPERVISOR_VERSION_CODE PACKAGE_VERSION_CODE(DTCSUPERVISOR_VERSION_MAJOR, DTCSUPERVISOR_VERSION_MINOR, DTCSUPERVISOR_VERSION_PATCH)
#ifndef DTCSUPERVISOR_PREVIOUS_VERSIONS
#define DTCSUPERVISOR_FULL_VERSION_LIST PACKAGE_VERSION_STRING(DTCSUPERVISOR_VERSION_MAJOR, DTCSUPERVISOR_VERSION_MINOR, DTCSUPERVISOR_VERSION_PATCH)
#else
#define DTCSUPERVISOR_FULL_VERSION_LIST DTCSUPERVISOR_PREVIOUS_VERSIONS ","
PACKAGE_VERSION_STRING (DTCSUPERVISOR_VERSION_MAJOR, DTCSUPERVISOR_VERSION_MINOR, DTCSUPERVISOR_VERSION_PATCH)
#endif

namespace DTCSupervisor {
    const std::string package = "DTCSupervisor";
    const std::string versions = DTCSUPERVISOR_FULL_VERSION_LIST;
    const std::string summary = "Early test implementation of uTCA DTCSupervisor";
    const std::string description = "Early test implementation of uTCA DTCSupervisor";
    const std::string authors = "Georg Auzinger";
    const std::string link = "http://xdaq.web.cern.ch";
    config::PackageInfo getPackageInfo();
    void checkPackageDependencies() throw (config::PackageInfo::VersionException);
    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
