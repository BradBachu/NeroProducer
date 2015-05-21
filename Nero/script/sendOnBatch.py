import os,sys
import re
from glob import glob
from optparse import OptionParser, OptionGroup
from subprocess import call, check_output

parser = OptionParser(usage = "usage");
parser.add_option("-n","--nJobs",dest="nJobs",type="int",help="number of jobs. (will be adapted to have more or less the same number of files)",default=1);
parser.add_option("-i","--input",dest="input",type="string",help="input pset",default="test/NeroProducer.py");
parser.add_option("-d","--dir",dest="dir",type="string",help="working directory",default="test/mydir");
parser.add_option("-e","--eos",dest="eos",type="string",help="eos directory to scout, will not read the files in the pSet",default="");
parser.add_option("","--put-in",dest="put",type="string",help="eos directory to cp the results ",default="");
parser.add_option("-q","--queue",dest="queue",type="string",help="batch Queue",default="");
parser.add_option("","--only-submit",dest="onlysubmit",action='store_true',help="submit all sh files in opts.dir. Igonere other options",default=False);

(opts,args)=parser.parse_args()

EOS = "/afs/cern.ch/project/eos/installation/0.3.84-aquamarine/bin/eos.select"

## check if working directory exists
cmd = "[ -d "+ opts.dir+" ]"
status = call( cmd , shell=True)

if status == 0:
	print "Directory",opts.dir,"already exists"
	cmd =  "rmdir " + opts.dir
	status = call(cmd, shell = True)
	if status !=0:
		if not opts.onlysubmit:
			print "Directory",opts.dir,"is not empty"
			raise Exception('Directory not empty')

## DEBUG

def chunks(l, n):
    """ Yield successive n-sized chunks from l.
    """
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

def chunksNum( l, tot):
    """ Yield successive n-sized chunks form l, in total number tot"""
    if len(l) % tot == 0:
    	ChunkSize = len(l) / tot
    else:
    	ChunkSize = (len(l) / tot) + 1
    return chunks(l,ChunkSize)

def submit(list, queue = "1nh"):
    """ Submit the jobs in the list """
    for sh in list:
    	cmd = "bsub -q " + queue + " -o " + re.sub(".sh$",".txt",sh) + " -J job" + re.sub(".*sub","",re.sub(".sh","",sh)) + " "+ sh
	call(cmd, shell = True)
    
if opts.onlysubmit:
	toBeSubmitted = glob(opts.dir + "/*.sh" )
	if opts.queue == "":
		print "Empty queue option"
		exit(1)
	submit(toBeSubmitted, opts.queue)
	exit(0)
	
toBeSubmitted= []

## creating a working directory
cmd = ["mkdir","-p", opts.dir]
call(cmd)

## get list of files from pset
if opts.eos == "":
	exec( re.sub('/','.',"from "+opts.input+" import fileList") )
else:	
	cmd = EOS+ " find -f " + opts.eos
	print "Going to call cmd:",cmd
	outputList = check_output(cmd,shell=True)
	fileList0 = outputList.split() ## change lines into list
	fileList = [ '"' + re.sub("/eos/cms","",f) +'"' for f in fileList0 ]


fileChunks = chunksNum(fileList, opts.nJobs)

mylen=0
for idx,fl in enumerate(fileChunks):
	mylen += len(fl)
	psetFileName = "NeroProducer_%d.py"%(idx)
	pset = opts.dir + "/" + psetFileName 
	#create file .sh
	sh = open( opts.dir + "/" + "sub_%d.sh"%idx, "w")
	print >> sh, "#!/bin/bash"
	print >> sh, '[ "${WORKDIR}" == "" ] && { mkdir -p /tmp/$USER/ ; export WORKDIR=/tmp/${USER}/; }'
	print >> sh, "cd " + os.environ['PWD']
	#print >> sh, "cmsenv"
	print >> sh, "eval `scramv1 runtime -sh`" ## this is cmsenv
	print >> sh, "cd " + opts.dir
	print >> sh, 'touch sub_%d.run'%idx
	print >> sh, 'cd $WORKDIR'
	print >> sh, "cmsRun " + os.environ['PWD'] + "/" + opts.dir + "/" + psetFileName #+ " 2>&1 > log_%d.log"%idx
	print >> sh, "EXIT=$?"
	print >> sh, "cd " + os.environ['PWD'] # support both absolute and relative path
	print >> sh, "cd " + opts.dir
	print >> sh, 'rm sub_%d.run'%idx
	print >> sh, 'echo "exit status is ${EXIT}"'
	if opts.put != "":
		print >> sh, '[ "${EXIT}" == "0" ] && { cmsMkdir ' + opts.put +'  && cmsStage ${WORKDIR}/NeroNtuples.root ' + opts.put + "/" +' && touch sub_%(idx)d.done || echo "cmsStage fail" > sub_%(idx)d.fail; }'%{'idx':idx}
	else:
		print >> sh, '[ "${EXIT}" == "0" ] && { cp ${WORKDIR}/NeroNtuples.root ./NeroNtuples_%(idx)d.root && touch sub_%(idx)d.done || echo "cp fail" > sub_%(idx)d.fail ; }'%{'idx':idx}

	print >> sh, '[ "${EXIT}" == "0" ] || echo ${EXIT} > sub_%d.fail'%idx
	print >> sh, ""
	sh.close()

	cmd = "chmod u+x " + opts.dir +"/" + "sub_%d.sh"%idx
	call(cmd, shell = True)
	
	toBeSubmitted.append( os.environ['PWD'] + "/" +  opts.dir + "/" + "sub_%d.sh"%idx )
	
	#copy pset
	cmd= ["cp","-v", opts.input, pset ]
	call(cmd)
	#prepare string
	#print "---------------------------------------------------------"
	#print "FileChunks is", fl
	#print "---------------------------------------------------------"
	flStr = "fileList = [ "+ ",".join(fl) + "]";
	#replace
	cmd = "sed -i'' 's:###FILELIST###:"+flStr+":g' " + pset 
	call(cmd,shell=True)
	#re.sub('###FILELIST###')

print "MY LEN is ", mylen ,"==",len(fileList)


if opts.queue != "":
	submit(toBeSubmitted,opts.queue)