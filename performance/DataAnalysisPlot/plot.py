import os.path
import json
import glob
import matplotlib
# Force matplotlib to not use any Xwindows backend.
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys

BUILD = 'Build'
BUILD_NUMBER = 'buildNumber'
CYCLES = 'cycles:u'
GEOCYCLES = 'geoCycles'
GEOINSTRUCTIONS = 'geoInstructions'
GEOTRACKER = 'geoTracking.json'
ICGREP = 'icgrep'
INSTRUCTIONS = 'instructions:u'
TRACKER = 'performanceTracking.json'

if os.path.isfile(GEOTRACKER) != True:
	print "No " + GEOTRACKER + " file found in this folder. Exit"
	sys.exit()

if os.path.isfile(TRACKER) != True:
	print "No " + TRACKER + " file found in this folder. Exit"
	sys.exit()

with open (GEOTRACKER) as datafile:
	inData = json.load(datafile)

buildsUncoded = inData[BUILD_NUMBER]
builds = []

for word in buildsUncoded:
	builds.append(word.encode("utf-8"))

x = range(1, len(inData[GEOCYCLES])+1)
y = inData[GEOCYCLES]

plt.xticks(x, builds)
plt.plot(x, y)
plt.xlabel(BUILD)
plt.ylabel('Geometric Mean of Cycles')
plt.title('Geometric Mean of Cycles vs Build')
plt.savefig(GEOCYCLES)
plt.clf()

y = inData[GEOINSTRUCTIONS]

plt.xticks(x, builds)
plt.plot(x, y)
plt.xlabel(BUILD)
plt.ylabel('Geometric Mean of Instructions')
plt.title('Geometric Mean of Instructions vs Build')
plt.savefig(GEOINSTRUCTIONS)
plt.clf()


with open (TRACKER) as outFile:
	inData = json.load(outFile)

for section in inData[ICGREP]:
	for test in inData[ICGREP][section]:

		x = range(1, len(inData[ICGREP][section][test][CYCLES])+1)
		y = inData[ICGREP][section][test][CYCLES]

		plt.xticks(x, builds)
		plt.plot(x, y)
		plt.xlabel(BUILD)
		plt.ylabel(CYCLES.replace(':u', ""))
		plt.title(section.replace('.', "-") + "_" + test.replace('.', "-") + "_" + CYCLES.replace(':u', ""))
		plt.savefig(section.replace('.', "-") + "_" + test.replace('.', "-") + "_" + CYCLES.replace(':u', ""))
		plt.clf()

		y = inData[ICGREP][section][test][INSTRUCTIONS]

		plt.xticks(x, builds)
		plt.plot(x, y)
		plt.xlabel(BUILD)
		plt.ylabel(INSTRUCTIONS.replace(':u', ""))
		plt.title(section.replace('.', "-") + "_" + test.replace('.', "-") + "_" + INSTRUCTIONS.replace(':u', ""))
		plt.savefig(section.replace('.', "-") + "_" + test.replace('.', "-") + "_" + INSTRUCTIONS.replace(':u', ""))
		plt.clf()





