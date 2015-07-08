#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# daqprocessor_single_calibrate.py -i <input_dir> -l -s -p <process_level>
#
# Arguments
#                -i <input_dir>     : directory with Modulation data (should have same num as run)
#                -l                 : produce long root files 
#                -s                 : process slow control data
#                -p <process_level> :  0 = full reprocess of run
#                                      1 = calibrate + reprocess + analyze
#                                      2 = analyze only
#
# Process a run in 3 steps: 
#  (1) slow data
#  (2) process subset of raw data (#0-#9) to get energy calibration
#  (3) run with the energy calibration found in (2) on the full dataset 
#
# IMPORTANT VARIABLES:
# modulation_basedir should contain (1) daqana and (2) analysis packages from github
#
# A.P. Colijn - colijn@nikhef.nl
#
modulation_basedir = "/user/z37/Modulation"
# output_basedir to be set to directory where the output structure should be
output_basedir = "/data/atlas/users/acolijn/Modulation"
#  run dir: where do you want all the scipts to live?
run_dir = modulation_basedir + "/stoomboot/scripts"

import sys
sys.path.append('python')
from processorlib import *

############################################################################################


# global initialization of the run processing

print('MAIN:: Welcome to the modulation daq-processor...')
# parse the IO arguments below
filebase, dummy1, grafOn, longRoot, dummy2, dummy3, processLevel = parseArguments(sys.argv[1:])

# retrieve the run name
run = filebase.split('/')[-1]
if (run == ""):
    run = filebase.split('/')[len(filebase.split('/'))-2]

# compose the output directory
outdir = output_basedir + '/' + run

# make the output directory if it is not there yet
if not os.path.exists(outdir):
    cmd = 'mkdir ' + outdir
    os.system(cmd)

# make the output directory for the calibration files if it does not yet exist
cal_output = output_basedir+'/calibration'
if not os.path.exists(cal_output):
    cmd = 'mkdir ' + cal_output
    os.system(cmd)
    
# make the output directory for the analysis files if it does not yet exist
ana_output = output_basedir+'/analysis'
if not os.path.exists(ana_output):
    cmd = 'mkdir ' + ana_output
    os.system(cmd)

# calibration filename
calibration = cal_output+'/CAL_'+run+'.root'


############################################################################################

#
# process slow control data
#
def process_slow_data():
    #  get the files from the data directory
    filenames, slownames = getFilenames(filebase)
    nb_files = len(filenames)
    nb_sfiles = len(slownames)
    slownames.sort()
    
    print('MAIN:: Beginning to parse slow data')
    for i in range(nb_sfiles):
        daqfile = generateDriverFile(outdir,slownames[i], 'NULL.root')
        slow_cmd_string = './slowdaq -i ' + daqfile
        os.system(slow_cmd_string)
        cmd_string = 'rm -f ' + daqfile
        os.system(cmd_string)
    print('MAIN:: Done parsing slow data, moving on to fast data')

############################################################################################

#
# process fast data without calibration and then make the energy calibration
#
def make_calibration(calib):
    #
    # make the calibration
    #
    print('MAIN:: Make energy calibration')
    calscript = run_dir+'/do_calibrate_'+run+'.C'
    fout = open(calscript,'w')
    
    #
    # compose the calibration execution script
    #
    fout.write('#include "/user/z37/Modulation/analysis/calibration/ecal.C" \n');
    fout.write('void do_calibrate_'+run+'(){ \n')
    fout.write('  ecal e("'+outdir+'/calibration/","'+calib+'"); \n')
    fout.write('  e.Loop(); \n')
    fout.write('}\n')
    
    fout.close()
    
    #
    # execute the calibration
    #
    cmd_string = 'root -b -q ' + calscript
    os.system(cmd_string)

    return

############################################################################################

#
# run with calibration
#
def process_fast_data(calib):
#  get the files from the data directory
    filenames, slownames = getFilenames(filebase)
    nb_files = len(filenames)
    outdir_tot = outdir


    # no calibration? just run over 10 files...
# # # new calibration processes all    if (calib == 'NULL.root' and nb_files>10):
# # # new calibration processes all        nb_files = 10
    
    if (calib == 'NULL.root'):
        outdir_tot = outdir + '/calibration/'
        # make the output directory if it is not there yet
        if not os.path.exists(outdir_tot):
            cmd = 'mkdir ' + outdir_tot
            os.system(cmd)

    print('MAIN:: Run daqana with/without energy calibration')
    for file_id in range(0, nb_files):
        #for file_id in range(0, 10):
        print('FILE          file_id:',file_id)
        # generate driver file
        filename = filenames[file_id]
        daqfile = generateDriverFile(outdir_tot,filename,calib)
        
        cmd_string = './daqana -i ' + daqfile
        if (calib != 'NULL.root'):
          # include the slow data
          cmd_string = cmd_string + ' -s -l'
        else:
          # no slow data when we do the calibration
          cmd_string = cmd_string + ' -l'

        if(grafOn):
	  cmd_string = cmd_string + ' -g'
        
        print('MAIN:: Processing ' + filename)
        os.system(cmd_string)
        print('MAIN:: Processing complete for ' + filename)
        print('MAIN:: Remove ' + daqfile)
        cmd_string = 'rm -f ' + daqfile
        os.system(cmd_string)

############################################################################################

#
# After run analysis
#
def do_analysis():

    print('MAIN:: Make analyzer script and run it ....')
    
    analyzerscript = run_dir +'/do_analyzer_'+run+'.C'
    analyzer_file = ana_output+'/ANA_'+run+'.root'
    fout = open(analyzerscript,'w')
    
    #
    # compose the analyzer execution script
    #
    fout.write('#include "/user/z37/Modulation/analysis/calibration/analyzer.C" \n');
    fout.write('void do_analyzer_'+run+'(){ \n')
    fout.write('  analyzer ana("'+outdir+'/","'+analyzer_file+'"); \n')
    fout.write('  ana.Loop(); \n')
    fout.write('} \n')
    fout.close()

    #
    # execute the analysis
    #
    cmd_string = 'root -b -q ' + analyzerscript
    os.system(cmd_string)


###############################################################################

#
# MAIN python code (see instructions on top)
#

print('daqprocessor::MAIN process level = ',processLevel);

#
# run without calibration on a subset of the data to make the calibration input
#
if (processLevel <= 0):
  process_fast_data('NULL.root')

#
#  make energy calibration
#
if (processLevel <= 1):
  make_calibration(calibration)

#
# run with calibration
#
if (processLevel <= 1):
  # process the slow data first
  process_slow_data()
  # process the fast data
  process_fast_data(calibration)

#
# do the post-processor analysis
#
if (processLevel <= 2):
  do_analysis()

print('MAIN:: Exit from the daq-processor. bye-bye.')

###############################################################################

