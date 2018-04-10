import os.path
import json
import glob
import sys

BUILD_NUMBER = 'buildNumber'
CYCLES = 'cycles:u'
GEOCYCLES = 'geoCycles'
GEOINSTRUCTIONS = 'geoInstructions'
GEOTRACKER = 'geoTracking.json'
ICGREP = 'icgrep'
INSTRUCTIONS = 'instructions:u'
TEST_SUITE_SIZE = 27.0
TRACKER = 'performanceTracking.json'

if len(sys.argv) != 3:
	print "Please run python format.py <file path> <build number>.  You didn't.  So now I exit."
	sys.exit()

# inputFile = glob.glob('*perf.json')		# This script really shouldn't assume anything about the folder it's in...
# if len(inputFile) != 1:
# 	print "Please make sure there is exactly one file in format.py's directory matching *perf.json"
# 	sys.exit()

inputFile = sys.argv[1]

if os.path.isfile(TRACKER):
	
	with open (inputFile) as dataFile:
		inData = json.load(dataFile)

	with open (TRACKER) as outFile:
		outData = json.load(outFile)
	
	geoCycles = 1
	geoInstructions = 1

	for section in inData[ICGREP]:
		for test in inData[ICGREP][section]:
			geoCycles = geoCycles*( ( inData[ICGREP][section][test][CYCLES] )**(1.0/TEST_SUITE_SIZE) )
			geoInstructions = geoInstructions*( ( inData[ICGREP][section][test][INSTRUCTIONS] )**(1.0/TEST_SUITE_SIZE) )

			outData[ICGREP][section][test][CYCLES].append(inData[ICGREP][section][test][CYCLES]) 
			outData[ICGREP][section][test][INSTRUCTIONS].append(inData[ICGREP][section][test][INSTRUCTIONS])

	with open (TRACKER, 'w') as outFile:
		outFile.write(json.dumps(outData, indent=4))

	with open (GEOTRACKER) as datafile:
		inData = json.load(datafile)
	
	inData[GEOCYCLES].append(geoCycles)
	inData[GEOINSTRUCTIONS].append(geoInstructions)
	inData[BUILD_NUMBER].append(sys.argv[2])

	with open (GEOTRACKER, 'w') as outFile:
		outFile.write(json.dumps(inData, indent=4))

else:
	
	with open (inputFile) as dataFile:
		inData = json.load(dataFile)

	geoCycles = 1
	geoInstructions = 1
	
	for section in inData[ICGREP]:
		for test in inData[ICGREP][section]:
			geoCycles = geoCycles*( ( inData[ICGREP][section][test][CYCLES] )**(1.0/TEST_SUITE_SIZE) )
			geoInstructions = geoInstructions*( ( inData[ICGREP][section][test][INSTRUCTIONS] )**(1.0/TEST_SUITE_SIZE) )

			inData[ICGREP][section][test][CYCLES] = [ inData[ICGREP][section][test][CYCLES] ]
			inData[ICGREP][section][test][INSTRUCTIONS] = [ inData[ICGREP][section][test][INSTRUCTIONS] ]

	with open (TRACKER, 'w') as outFile:
		outFile.write(json.dumps(inData, indent=4))

	with open (GEOTRACKER, 'w') as outFile:
		outFile.write(json.dumps({GEOCYCLES: [geoCycles], GEOINSTRUCTIONS: [geoInstructions], BUILD_NUMBER: [sys.argv[2]]}, indent=4))



