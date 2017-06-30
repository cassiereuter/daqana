#ifndef __SROOTDRIVER_H__
#define __SROOTDRIVER_H__

#include <TFile.h>
#include <TTree.h>


#include <vector>
#include <string>

#include "sevent.hh"
#include "driver.hh"

#include <stdint.h>

using namespace std;


class srootdriver
{
public:
    srootdriver();
    srootdriver(driver *drv);
    ULong64_t SlowFill(slowevent *old_sev, ULong64_t old_stime);
    void writeParameters(driver *drv);
    void Close();
    
private:
    TTree *stree;
    
    TFile *f;
    
    Int_t 	slowid;
    Double_t	sdata;
    ULong64_t 	old_stime;
    ULong64_t	new_stime;
    ULong64_t	stimestamp;
    Int_t 	nSlowParams;
    Double_t 	*slowdata;
    
    Int_t 	slow_entry;
    
};

#endif // __SROOTDRIVER_H__