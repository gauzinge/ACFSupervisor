## DTC Supervisor README page

This is the readme page of the new DTC Supervisor. Below you can find instructions on how to install the application and an overview of the basic functionality. This SW is built on top of CMS XDAQ versions 13 and 14 and requires a recent gcc version that supports c++11. Additional requirements are listed in the Requirements section.

### Requirements

To compile and run this software, you need an installation of CERN Scientific Linux 6 or CERN CentOS 7. On SLC6 you need to install Devtoolset-2.1 (via yum) to get gcc 4.8 which is required for the c++11 features. On CC7, the default compiler version is gcc 4.8. Additional requirements are XDAQ 13 on SLC6 or XDAQ14 on CC7 - both versions can be installed via yum once the corresponding .repo file (https://svnweb.cern.ch/trac/cmsos) is downloaded and placed in /etc/yum.repos.d/ as xdaq.repo. Make sure to disable the gpg-check by adding the line gpgcheck=0 to each resource. Then install the following packages:

```sh
sudo yum groupinstall extern-coretools coretools extern-powerpack powerpack xdaq2rc
```

Other packages that can be downloaded and installed via yum are:

* libxml2
* libxslt
* libxml++-2.6
* libuuid-devel
* boost-devel (only on CC7)
* CERN ROOT > 5.34 (> 6.10 for online Plots)

This software internally links against Ph2_ACF (https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF) which can be checked out via git. It requires uHAL (cactus.cern.ch) and a recent version of ROOT. Install instructions can be found on the github page. To install cactus, place the recent cactus.repo in /etc/yum.repos.d/ as ipbus-sw.repo and then 

```sh
sudo yum groupinstall uhal
```

For CERN ROOT do 

```sh
sudo yum install root root-net-http root-graf3d-gl root-physics libusb-devel root-montecarlo-eg root-graf3d-eve root-geom
```
This will give you ROOT v5.34 on SLC6 and ROOT v6.10 on CC7. If you are on CC7, also install the boost headers via

```sh
sudo yum install boost-devel
```

Ph2_ACF itself is built via CMAKE, so you need a version > 2.8, also installed via yum. To build, do the following from the Ph2_ACF root directory:

```sh
cd build/
cmake ..
make -jN
cd ..
source setup.sh
```

### Installation

To compile the code, have a look at the provided setup.sh script and adapt the paths to the various dependences accordingly (Ph2_ACF, CACTUS, BOOST). Once this is done and all the dependences are installed, the software should compile with the included makefile. In order to run it, the setup.sh file creates an alias "test" that will call the appropriate xdaq command:
```sh
/opt/xdaq/bin/xdaq.exe -p 40400 -c path/DTCSupervisor/xml/DTCSupervisor.xml
```
You can then reach the web GUI on your browser with the following address

http://yourhost.yourdomain:40400/urn:xdaq-application:lid=2/MainPage

Also have a look at the provided DTCSupervisor.xml configuration file in the xml/ directory. This allows to set the default alues for many of the parameters, ports, URLs and might need some editing on your system. 

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
The sofware is intended as a XDAQ wrapper to the Ph2_ACF framework and thus some knowledge of the framework and configuration files is required. It uses the HwDescription.xml files of Ph2_ACF which can be edited via the web GUI. The main page pictured above has controls for the finite state machine, the current HWDescription.xml file and a navigation bar. In addition it shows the current state and an event counter. In the main tab, you can configure the main run settings, select which procedures you want to run and adapt the settings for them - the page shows and hides settings automatically that are specific to certain types of runs. Other options allow to set the run number (if -1, the current run number will be picked up from the .runnumber.txt file in the Ph2_ACF root directory) and enabling writing of raw (.raw) or SLink (.daq) data files. In the bottom there is a log window that will show the Ph2_ACF command line output. 
![User Interface](/fig/DTC_config.jpg "Config Page")
The config page allows you to edit the currently loaded HWDescription.xml file via the web GUI or to load a different file. Note that edits are only possible in the Initial and Configured state of the finite state machine. 
![User Interface](/fig/DTC_DS.jpg "Data sending and Playback Page")
The Playback and DS page allows you to either play back data from a previously recorded file (.raw or .daq data for raw and SLink type data) and forward it to a CMS EVM-BU readout chain via TCP sockets. Changes are only possible in the Initial or Halted state. On this page you can also configure and enable the data sending mechanism.
![User Interface](/fig/DTC_FW.jpg "Firmware Page")
The FW page allows to change the FW running on the FC7 or upload and download images to and from the SD card. It uses the corresponding functions of Ph2_ACF and some operations can result in a segfault of the application - this is to be expected as the network settings of the FC7 may change during FW changes. Operations on this page can only be performed in the Initial state. Before you can perform any changes, uploads or downloads, you need to list the images on the SD card via the coresponding button. 

The work-in-progress Calibration and DQM page will soon show live rootplots from the running processes via THttp Server.

### Remarks
The use of this software should be relatively self-explanatory if you are familiar with the concept of CMS DAQ finite state machines and other applications built on top of XDAQ. Most web GUI controls are obvious but some knowledge of Ph2_ACF (mainly config files) is required to run the system. This software is under active development and new features may be added at a later point.

Should you discover any bugs or have questions about features, please contact georg.auzinger@SPAMNOT.cern.ch.

