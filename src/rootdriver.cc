#include "rootdriver.hh"

#include "TParameter.h"
#include "TDirectory.h"
#include "TNamed.h"

#include <iostream>

rootdriver::rootdriver(){}

rootdriver::rootdriver(driver *drv, Bool_t tmpbool, Bool_t slow){
    //
    // open the output root file
    //
    f = new TFile(drv->getRootFile().c_str(),"RECREATE");

    longRoot = tmpbool;
    slowOn = slow;
    
    // initialize the Branch variables
    chanNum = -1;
    integral = 0.;
    pkheight = 0.;
    timestamp = 0;
    isTestPulse = false;
    errorCode = 0;
    energyRatio = 0;
    
    // calibration constants
    cout <<"rootdriver::rootdriver init calibration constants "<<endl;
    for(int ich = 0; ich < NUMBER_OF_CHANNELS; ich++) {
  
        for (int ipar=0; ipar<MAX_PARAMETERS; ipar++){
            calibration_constant[ich][ipar] = 0.0;
        }
        calibration_constant[ich][1] = 1.0;
        _cal_tmin = 0;
        _cal_tmax = 999999999999999; // never get out of range anymore....
        _cal_c0 = 0;
        _cal_c1 = 0;
        _cal_c2 = 0;
    }
    cout <<"rootdriver::rootdriver done"<<endl;
    
    // read the calibration constants if you wish
    calFile = drv->getCalibrationFile();
    if(calFile != "NULL.root"){
        if(CALIBRATION_MODE == 0){
          _cal = new TFile(calFile.c_str(),"READONLY");

          TParameter<double>* cal;
          char tmp[100];
          for(int ich = 0; ich<NUMBER_OF_CHANNELS; ich++){
              for (int ipar=0; ipar<MAX_PARAMETERS; ipar++){
                  
                  sprintf(tmp,"cal_ch%02d_c%i",ich,ipar);
                  cal = (TParameter<double>*)_cal->Get(tmp);
                  calibration_constant[ich][ipar] = cal->GetVal();
                
              }
          }
          _cal->Close();
          foundCal = kTRUE;
       } else {
          _b_cal_tmin = 0;
          _b_cal_tmax = 0;

          _b_cal_c0 = 0;
          _b_cal_c1 = 0;
          _b_cal_c2 = 0;
          //
          // initialize the calibration root tree
          //
          _cal = new TFile(calFile.c_str(),"READONLY");
          _cal->GetObject("cal",_cal_tree);
          _cal_tree->SetBranchAddress("cal_tmin",&_cal_tmin,&_b_cal_tmin);
          _cal_tree->SetBranchAddress("cal_tmax",&_cal_tmax,&_b_cal_tmax);
          _cal_tree->SetBranchAddress("c0",&_cal_c0,&_b_cal_c0);
          _cal_tree->SetBranchAddress("c1",&_cal_c1,&_b_cal_c1);
          _cal_tree->SetBranchAddress("c2",&_cal_c2,&_b_cal_c2);

          cal_index = 0;
          //
          // read the first set of calibration constants
          //
          readCalibration(0);
          foundCal = kFALSE;
       }
    }
    
    // extra variables for extended root file
    baseline    = 0;
    baselineRMS = 0;
    f->cd();
    // Define tree and branches
    tree = new TTree("T", "Source data");
    
    tree->Branch("channel", &chanNum, "channel/I");
    tree->Branch("integral", &integral, "integral/F");
    tree->Branch("height", &pkheight, "height/F");
    tree->Branch("time", &timestamp, "time/D");
    tree->Branch("istestpulse", &isTestPulse, "istestpulse/bool");
    tree->Branch("error", &errorCode, "error/I");
    
    if(longRoot){
        tree->Branch("baseline", &baseline, "baseline/f");
        tree->Branch("rms", &baselineRMS, "baselineRMS/f");
        tree->Branch("ratio", &energyRatio, "ratio/f");
    }
    
    //
    // Open the slow control datafile + initialize the data + add branches to the output tree
    //
    
    // variables for slow data
    Int_t nSlowParameters;
    const char* slowbranchname;
    char* temp;
    Int_t nSlowParams = drv->getNSlowParams(); // valid for XML version CM1.0
    //slowdata = (Double_t*) malloc(sizeof(Double_t) * nSlowParams);
    cout << "rootdriver:: Number of slow parameters: " << nSlowParams << endl;
    for (Int_t i=0; i<nSlowParams; i++) {
        slowdata.push_back(0.0);
    }
    
    if(slowOn){
        cout << "rootdriver:: slow file name is = " <<  drv->getTempSlowFile().c_str() << endl;
        fs = new TFile(drv->getTempSlowFile().c_str(), "READONLY");
        temp_slowtree = (TTree*)fs->Get("ST");
        
        const char* type = "/D";
        char buffer[256];
        for (Int_t i=0; i<nSlowParams; i++){
            fs->cd();
            slowbranchname = drv->getSlowBranchName(i).c_str();
            //cout << slowbranchname << endl;
            strncpy(buffer, slowbranchname, sizeof(buffer));
            strncat(buffer, type, sizeof(buffer));
            //            cout << "SLOWBRANCHNAME >>"<<slowbranchname<<"<<" << endl;
            //            cout << "BUFFER         >>"<<buffer<<"<<" << endl;
            temp_slowtree->SetBranchAddress(slowbranchname, &slowdata[i]);
            //stree->Branch(slowbranchname, &slowdata[i], buffer);
            f->cd();
            tree->Branch(slowbranchname, &slowdata[i], buffer);
        }
        fs->cd();
        //stree->Branch("stime", &stime, "slowtimestamp/l");
        temp_slowtree->SetBranchAddress("stime", &stimestamp);
        // read the number of slow events
        number_of_slow_events = temp_slowtree->GetEntriesFast();
        // initialize the slow event variables
        slow_entry = 0;
        readSlowEvent(slow_entry);
    }
}

/*--------------------------------------------------------------------------------------*/
void rootdriver::readCalibration(int ilevel){
     //
     // Read the calibration constants from the calibration ntuple
     //

     _cal->cd();
     // get entry from the tree
     Long64_t tentry = _cal_tree->LoadTree(cal_index);
     _cal_tree->GetEntry(cal_index);

     if(ilevel==1) cout <<"rootdriver::readCalibration  cal_index = "<<cal_index<<endl;
     for(int ich=0; ich<NUMBER_OF_CHANNELS; ich++){
        if(ilevel==1) cout << "      ich = "<<ich<<" "<<_cal_c0->at(ich) <<" " << _cal_c1->at(ich) << endl;
        calibration_constant[ich][0] = _cal_c0->at(ich);
        calibration_constant[ich][1] = _cal_c1->at(ich);
        calibration_constant[ich][2] = _cal_c2->at(ich);
     }
     cout << "Read done"<<endl;

    // next time we will read the next calibration.... :)
    cal_index++;

    // go back to the output root tree directory...
    f->cd();

}
/*--------------------------------------------------------------------------------------*/
void rootdriver::readSlowEvent(Int_t islow){
    // read one event from the slow event tree
    Long64_t nb = 0;
    
    // if we can read the next event we get the time marker from the next entry
    if       (islow<number_of_slow_events-1){
        // read the next entry of the slow tree to get the time marker for using the next range
        nb = temp_slowtree->GetEntry(islow+1);
        time_end_of_range = stimestamp;
        // read the current entry
        nb = temp_slowtree->GetEntry(islow);
    } else if (islow == number_of_slow_events-1) { // last event
        time_end_of_range = 999999999999999; // never get out of range anymore....
        nb = temp_slowtree->GetEntry(islow);
    } else {
        cout << "rootdriver::readSlowEvent ERROR index out of range. index = "<<islow<<endl;
    }
    
}

void rootdriver::FastFill(event *ev, driver *dr){
    //
    // fill the variables into the root tree
    //
    chanNum    = ev->getChannel() % 100;
    
    timestamp  = ev->getTimeStamp();

    //
    // read constants until in sync with the fast data..... (should be quick)
    //  
    while(!foundCal) {
      cout << timestamp << " " <<_cal_tmin<<" "<<_cal_tmax<<endl;
      if(timestamp>=_cal_tmin && timestamp<_cal_tmax) {
         foundCal = kTRUE;
      } else {
        readCalibration(0);
      }
    }
    //
    // read new set of calibration constants
    //
    if(timestamp > _cal_tmax) readCalibration(1);

    Double_t A = ev->getArea();
    integral = 0;
    // apply the calibration
    for(int ipar=0; ipar<MAX_PARAMETERS; ipar++){
        integral += calibration_constant[chanNum][ipar]*pow(A,ipar);
    }
    pkheight   = ev->getPeak();
    isTestPulse = ev->getIsTestPulse();
    errorCode   = ev->getErrorCode();
    
    if(longRoot){
        baseline    = ev->getBaseline();
        baselineRMS = ev->getBaselineRMS();
        energyRatio = ev->getEnergyRatio();
    }
    
    if (slowOn){
        // read the next slow event only if the timestamp got out of bound....
        while ( (timestamp > time_end_of_range) && (slow_entry<number_of_slow_events) ){
            slow_entry++;
            readSlowEvent(slow_entry);
        }
    }
    //
    // write to the tree
    //
    f->cd();
    tree->Fill();
}

void rootdriver::writeParameters(driver *drv){
    char pName[100];
    // make separate root folder for run info
    TDirectory *_info = f->mkdir("info");
    _info->cd();
    
    // the strings....
    TNamed *Parameter = new TNamed("datafile",drv->getDataFile().c_str());
    Parameter->Write();
    Parameter = new TNamed("location",drv->getLocation().c_str());
    Parameter->Write();
    
    // the integer parameters
    TParameter<Int_t> *intPar = new TParameter<Int_t>("nsamples",drv->getNSample());
    intPar->Write();
    intPar = new TParameter<Int_t>("npretrigger",drv->getNPreTrigger());
    intPar->Write();
    intPar = new TParameter<Int_t>("nevent",drv->getNEvent());
    intPar->Write();
    intPar = new TParameter<Int_t>("nheader",drv->getNHeader());
    intPar->Write();
    intPar = new TParameter<Int_t>("chunksize",drv->getArraySize());
    intPar->Write();
    intPar = new TParameter<Int_t>("eventsize",drv->getEventSize());
    intPar->Write();
    // the double parameters
    TParameter<Double_t> *dblPar = new TParameter<Double_t>("deltat",drv->getDeltaT());
    dblPar->Write();
    
    // sources in separate folder
    TDirectory *_sources = _info->mkdir("source");
    _sources->cd();
    for(Int_t i=0; i<8; i++) {
        sprintf(pName,"channel_%i",i);
        Parameter = new TNamed(pName,drv->getSource(i).c_str());
        Parameter->Write();
    }
    
    // serial numbers in separate folder
    TDirectory *_serial = _info->mkdir("serial");
    _serial->cd();
    for(Int_t i=0; i<8; i++) {
        sprintf(pName,"channel_%i",i);
        Parameter = new TNamed(pName,drv->getDetectorSerial(i).c_str());
        Parameter->Write();
    }
    
    // active channels in separate folder
    TDirectory *_active = _info->mkdir("active");
    _active->cd();
    for(Int_t i=0; i<8; i++) {
        sprintf(pName,"channel_%i",i);
        Parameter = new TNamed(pName,drv->getActiveChannel(i).c_str());
        Parameter->Write();
    }
    
    // trigger levels in separate folder
    TDirectory *_trigger = _info->mkdir("trigger");
    _trigger->cd();
    for(Int_t i=0; i<8; i++) {
        sprintf(pName,"channel_%i",i);
        dblPar = new TParameter<Double_t>(pName,drv->getTriggerLevel(i));
        dblPar->Write();
    }
    
    // PMT HV levels in separate folder
    TDirectory *_hv = _info->mkdir("hv");
    _hv->cd();
    for(Int_t i=0; i<8; i++) {
        sprintf(pName,"channel_%i",i);
        dblPar = new TParameter<Double_t>(pName,drv->getPMTvoltage(i));
        dblPar->Write();
    }
    
    f->cd();
}

void rootdriver::Close(){
    f->cd();
    tree->Write(0, TObject::kWriteDelete, 0); // overwrite the last auto save
    f->Close();
    
    if(slowOn) {
        temp_slowtree = NULL;
        fs->Close();
    }

    if(calFile != "NULL.root") {
        _cal_tree = NULL;
        _cal->Close();
    }

}
