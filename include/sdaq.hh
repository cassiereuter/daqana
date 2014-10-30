#ifndef __SDAQ_H__
#define __SDAQ_H__

#include "sevent.hh"
#include "driver.hh"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdint.h>
#include <TROOT.h>

#define NBYTE_PER_INT     2
#define N_TIME_INT        4
#define N_HEADER_INT      5
#define CHANNEL_OFFSET    100
#define ARRAY_HEADER_SIZE 4
#define NBYTE_PER_DBL     8

using namespace std;

class sdaq
{
public:
    sdaq();
    sdaq(driver* dr);
    //~sdaq();
    //void sdaqClose();
    slowevent* readSlowEvent();
    Int_t   GetSlowFileSize();
    
private:
    Double_t readDouble();
    ULong64_t readU64();
    
    Int_t  slowByteRead;
    Int_t  nBytePerArray;
    ULong64_t	initial_timestamp;
    Double_t	deltat;
    ifstream slowfile;
    slowevent *sev;
    
};
#endif // __SEVENT_H__