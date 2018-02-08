#include "config/version.h"
#include "xcept/version.h"
#include "xdaq/version.h"
#include "ACFSupervisor/version.h"

GETPACKAGEINFO (ACFSupervisor)
void ACFSupervisor::checkPackageDependencies() throw (config::PackageInfo::VersionException)
{
    CHECKDEPENDENCY (config);
    CHECKDEPENDENCY (xcept);
    CHECKDEPENDENCY (xdaq);
}
std::set<std::string, std::less<std::string> > ACFSupervisor::getPackageDependencies()
{
    std::set<std::string, std::less<std::string> > dependencies;
    ADDDEPENDENCY (dependencies, config);
    ADDDEPENDENCY (dependencies, xcept);
    ADDDEPENDENCY (dependencies, xdaq);
    return dependencies;
}
