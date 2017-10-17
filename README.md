## DTC Supervisor README page

This is the readme page of the new DTC Supervisor. Below you can find instructions on how to install the application and an overview of the basic functionality. This SW is built on top of CMS XDAQ versions 13 and 14 and requires a recent gcc version that supports c++11. Additional requirements are listed in the Requirements section.

### Requirements

To compile and run this software, you need an installation of CERN Scientific Linux 6 or CERN CentOS 7. On SLC6 you need to install Devtoolset-2.1 (via yum) to get gcc 4.8 which is required for the c++11 features. On CC7, the default compiler version is gcc 4.8. Additional requirements are XDAQ 13 on SLC6 or XDAQ14 on CC7 - both versions can be installed via yum once the corresponding .repo file is downloaded and placed in /etc/yum.repos.d/xdaq.repo.

Other packages that can be downloaded and installed via yum are:

* libxml2
* libxslt
* libxml++-2.6
* libuuid-devel
* boost-devel (only on CC7)
* CERN ROOT > 5.34 (> 6.10 for online Plots)

This software internally links against Ph2_ACF (https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF) which can be checked out via git. It requires uHAL (cactus.cern.ch) and a recent version of ROOT. Install instructions can be found on the github page.

### Installation

To compile the code, have a look at the provided setup.sh script and adapt the paths to the various dependences accordingly. Once this is done and all the dependences are installed, the software should compile with the included makefile. In order to run it, the setup.sh file creates an alias "test" that will call the appropriate xdaq command. Also have a look at the provided .xml configuration file in the xml/ directory. This allows to set the default alues for many of the parameters, ports, URLs and might need some editing on your system. 

```xml
<?xml version='1.0'?>
<xc:Partition xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30">

    <!-- Declare a context that contain applcation -->
    <xc:Context url="http://yourhostname:40400">

        <!-- Declare a DTCSupervisor application -->
        <xc:Application class="Ph2TkDAQ::DTCSupervisor" id="2" instance="0" network="local">
            <properties xmlns="urn:xdaq-application:DTCSupervisor" xsi:type="soapenc:Struct">
                <HWDescriptionFile xsi:type="xsd:string">${DTCSUPERVISOR_ROOT}/xml/HWDescription.xml</HWDescriptionFile>
                <DataDirectory xsi:type="xsd:string">${DTCSUPERVISOR_ROOT}/Data/</DataDirectory>
                <ResultDirectory xsi:type="xsd:string">${DTCSUPERVISOR_ROOT}/Results/</ResultDirectory>
                <RunNumber xsi:type="xsd:integer">-1</RunNumber>
                <NEvents xsi:type="xsd:unsignedInt">100</NEvents>
                <WriteRAW xsi:type="xsd:boolean">false</WriteRAW>
                <WriteDAQ xsi:type="xsd:boolean">false</WriteDAQ>
                <ShortPause xsi:type="xsd:integer">10</ShortPause>
                <THttpServerPort xsi:type="xsd:integer">8080</THttpServerPort>
                <SendData xsi:type="xsd:boolean">false</SendData>
                <DataSourceHost xsi:type="xsd:string">localhost</DataSourceHost>
                <DataSourcePort xsi:type="xsd:integer">404001</DataSourcePort>
                <DataSinkHost xsi:type="xsd:string">localhost</DataSinkHost>
                <DataSinkPort xsi:type="xsd:integer">404002</DataSinkPort>
                <PlaybackMode xsi:type="xsd:boolean">true</PlaybackMode>
                <PlaybackFile xsi:type="xsd:string">${DTCSUPERVISOR_ROOT}/Data/</PlaybackFile>
            </properties>
        </xc:Application>

        <!-- Shared object library that contains the inplementation -->
        <xc:Module>${DTCSUPERVISOR_ROOT}/lib/linux/x86_64_centos7/libDTCSupervisor.so</xc:Module>

    </xc:Context>
</xc:Partition>
```

### Interface
![User Interface](/fig/DTC_Main.jpg "Main Page")
The sofware is intended as a XDAQ wrapper to the Ph2_ACF framework and thus some knowledge of the framework and configuration files is required. It uses the HwDescription.xml files of Ph2_ACF which can be edited via the webinterface.
![User Interface](/fig/DTC_config.jpg "Config Page")
The webgui implements the XDAQ finite state machine and provides additional options to configure data sharing with other XDAQ applications via TCP sockets.
![User Interface](/fig/DTC_DS.jpg "Data sending and Playback Page")

