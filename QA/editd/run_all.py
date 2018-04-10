from subprocess import *
from string import *
import sys, os, optparse

if __name__ == "__main__":
	option_parser = optparse.OptionParser(usage='python %prog [options] <grep_executable>', version='1.0')
	option_parser.add_option('-e', '--exec_dir', dest = 'exec_dir', type='string', default='.', help = 'executable directory')
	options, args = option_parser.parse_args(sys.argv[1:])
	print(options.exec_dir)
	out_f = open('OutputFiles/reads2', 'w')
	expected_f = open('ExpectedOutput/reads2', 'r')
	call([options.exec_dir+'/editd', '-f', 'TestFiles/reads2', 'chr.fa', '-display'], stdout=out_f)
	if open('OutputFiles/reads2', 'r').read()==expected_f.read():
		print('Edit Distance test succeeded.')
	else:
		print('Edit Distance test failed.')
		exit(1)
