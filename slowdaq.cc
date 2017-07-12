#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "driver.hh"
#include "sdaq.hh"
#include "sevent.hh"
#include "srootdriver.hh"
#include <TCanvas.h>

using namespace std;
//
// MAIN program
//
int main(int argc, char **argv)
{
    // switches
    int c = 0;
    string       DriverFilename;
    // parse switches
    while((c = getopt(argc,argv,"i:")) != -1)
    {
        switch(c)   {
            case 'i': // name of driver
                DriverFilename = optarg;
                break;
            default:
                exit(-1);
        }
    }
    // driver for the daq processing..... provided from the python script
    cout << "slowdaq:: driver file = " << DriverFilename << endl;
    bool slowOn = true;
    driver* myDriver = new driver(DriverFilename, slowOn);
    // TApplication is needed to plot a canvas with an event
    // create an instance of the daq datatype: controls all the binary file handling
    sdaq myDaq(myDriver);
    // root management
    srootdriver myRoot(myDriver);
    ULong64_t new_stime;
    
    // loop over the events
    int totalnumberofevents = myDriver->getNEvent();
    
    slowevent *sev;
    int totalslowevents = myDaq.GetSlowFileSize();
    cout << "slowdaq:: Found " << totalslowevents << " slow events" << endl;
    ULong64_t old_stime = 0;
    
    for (int iSlowEv = 0; iSlowEv < totalslowevents; iSlowEv++){
        if(iSlowEv%10000 == 0) cout << "     processed "<<iSlowEv<<" slow events"<<endl;
        sev = myDaq.readSlowEvent();
        new_stime = myRoot.SlowFill(sev, old_stime);
        old_stime = new_stime;
//        delete sev;
    }
    //cout << "I am done processing slow events!!!! " << endl;
    
    
    //delete theApp;
    // write run parameters to file
    //myRoot.writeParameters(myDriver);
    myRoot.Close();
    delete myDriver;
    return 0;
}
