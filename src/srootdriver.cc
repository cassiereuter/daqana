#include "srootdriver.hh"

#include <iostream>

srootdriver::srootdriver(){}

srootdriver::srootdriver(driver *drv){
    
    // variables for slow data
    Int_t nSlowParameters;
    const char* slowbranchname;
    char* temp;
    nSlowParams = drv->getNSlowParams(); // valid for XML version CM1.0
    slowdata = (Double_t*) malloc(sizeof(Double_t) * nSlowParams);
    //cout << "Number of slow parameters: " << nSlowParams << endl;
    for (Int_t i=0; i<nSlowParams; i++) slowdata[i]=0;
    
    // for fast and slow data
    slow_entry = 1;
    
    string slow_filename = drv->getSlowFile();
    string slow_rootname = drv->getTempSlowFile();
    int name_length = strlen(slow_filename.c_str());
    //cout << slow_filename.substr(name_length-5, name_length) << endl;
    
    // open the root file
    if (slow_filename.substr(name_length-10, name_length) == "000000.slo") {
        f = new TFile(slow_rootname.c_str(),"RECREATE"); // TODO check this
        // Define tree and branches
        stree = new TTree("ST", "Slow data");
        const char* type = "/D";
        char buffer[256];
        for (Int_t i=0; i<nSlowParams; i++){
            slowbranchname = drv->getSlowBranchName(i).c_str();
            //cout << slowbranchname << endl;
            strncpy(buffer, slowbranchname, sizeof(buffer));
            strncat(buffer, type, sizeof(buffer));
            //cout << buffer << endl;
            stree->Branch(slowbranchname, &slowdata[i], buffer);
        }
        
        stree->Branch("stime", &stimestamp, "slowtimestamp/l");
    }
    else {
        f = new TFile(slow_rootname.c_str(),"UPDATE"); // TODO check this
        stree = (TTree*)f->Get("ST");
        for (Int_t i = 0; i<nSlowParams; i++) {
            slowbranchname = drv->getSlowBranchName(i).c_str();
            stree->SetBranchAddress(slowbranchname, &slowdata[i]);
        }
        stree->SetBranchAddress("stime", &stimestamp);
    }
    
}

ULong64_t srootdriver::SlowFill(slowevent *sev, ULong64_t old_stime){
    slowid 	= sev->getSlowID();
    sdata	= sev->getSlowData();
    new_stime  = sev->getSlowTimeStamp();
    
    //cout << "Slow ID: " << slowid << " Data: " << sdata << " Timestamp: " << new_stime << endl;
    
    //cout << old_stime << ", " << new_stime << endl;
    
    
    //if (new_stime == old_stime && slowid < nSlowParams) {
    //if (slowid > 7) cout << "WTF!!!!!! The slow ID was: " << slowid << " the data was: " << sdata << " and the timestamp was " << new_stime << endl;
    //else {
    //cout << "The slow ID was: " << slowid << " the data was: " << sdata << " and the timestamp was " << new_stime << endl;
    slowdata[slowid] = sdata;
    stimestamp = new_stime;
    //}
    //}
    //else {
    
    if (slowid == nSlowParams-1 && old_stime != 0) {
        //cout << "I filled in slow data here" << endl;
        stree->Fill();
        
    }
    //else cout << "PUFFFFF" << endl;
    //cout << stimestamp << endl;
    //cout << "0: " << slowdata[0] << " 1: " << slowdata[1] << " 2: " << slowdata[2] << " 3: " << slowdata[3] << " 4: " << slowdata[4] << " 5: " << slowdata[5] << " 6: " << slowdata[6] << " 7: " << slowdata[7] << endl;
    //cout << "I have filled the tree!" << endl;
    //slowdata[slowid] = sdata;
    //stimestamp = new_stime;
    
    
    //}
    
    
    return new_stime;
}

void srootdriver::Close(){
    stree->Write("", TObject::kOverwrite);
    f->Flush();
    f->Close();
}
