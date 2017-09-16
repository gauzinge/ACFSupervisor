new local daq sends via standard TCP sockets that are easy to initialize
data is sent in the Slink format but a FEROL header needs to be added every 4kb (these are 2 64 bit words)
an example is here:
https://github.com/mommsen/evb/blob/master/src/common/DummyFEROL.cc

below is the mail from Alessandro:
Hi Tom,

You are right that the data sender needs to be changed. You need to adapt to the FEROL data format, but also need to send the fragment over TCP instead of I2O messages. I don't know if Giacomo and Andrea finally managed to get things working.

I have C++ code which emulates the FEROL by generating dummy data. The code can be found here:
https://svnweb.cern.ch/trac/cmsos/browser/releases/baseline13/trunk/daq/evb/src/common/DummyFEROL.cc

or if you prefer github:
https://github.com/mommsen/evb/blob/master/src/common/DummyFEROL.cc

The most important steps are:
- During the Configure state transition the socket stream to the EVM / RU (destinationHost / destinationPort) has to be open. This is done in evb::test::DummyFEROL::openConnection().

- During the Halt state transition, the connection should be closed:
    evb::test::DummyFEROL::closeConnection()

    - The data is sent to the EVM / RU by writing to the socket. An example how you can do it can be found in evb::test::DummyFEROL::sendData (toolbox::mem::Reference* bufRef).

        - The SLINK FED format is the same, i.e. the FED header and trailers are unchanged. However, you need to add the FEROL header (https://twiki.cern.ch/twiki/bin/viewauth/CMS/CMD_FEROL_DOC#2_Description). An example how I do it for dummy data can be found in the method evb::readoutunit::LocalStream<ReadoutUnit,Configuration>::getFedFragment in
                https://svnweb.cern.ch/trac/cmsos/browser/releases/baseline13/trunk/daq/evb/include/evb/readoutunit/LocalStream.h

                Note that the fragmentTracker_ is used for generating empty FED data with header / trailers. Thus, this you need to replace with your real data source. Also note that the FEROL header needs to be repeated every 4 kB. Thus, if your FED fragment is larger than 4 kB (incl. the FEROL header), you need to split your payload and insert another header.

                    Feel free to ask if anything is not clear or if you get stuck. Please note that I will be on vacations next week. Thus, the response time to emails could be longer than usual.

                            Cheers,
                            Remi
