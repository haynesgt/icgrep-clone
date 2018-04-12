#include <string>
#include <vector>
#include <fstream>
#include <boost/algorithm/string/classification.hpp> 
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <algorithm>
#include "regexGen.h"
#include "stringGen.h"
#include "pugixml/src/pugixml.hpp"
#include "icgrep-test/icgrep-test.h"

using namespace std;

std::vector<string> vectorizeLine(string line){
	std::vector<string> elements;
	boost::split(elements, line, boost::is_any_of(","), boost::token_compress_on);
	return elements;
}

void clearDir(string path){
	namespace fs = boost::filesystem;
  	fs::path path_to_remove(path);
	for (fs::directory_iterator end_dir_it, it(path_to_remove); it!=end_dir_it; ++it) {
		fs::remove_all(it->path());
	}
}

std::vector<string> setOptions(std::vector<string> header, string xml){
	if (!xml.empty()){
		std::vector<string> options;
		pugi::xml_document doc;
		doc.load_file(xml.c_str());
		for (auto op : header){
			bool added = false;
			for (auto parameter : doc.first_child().children()){
				if (parameter.attribute("name").value() == op){
					string sch(parameter.child("Switch").child_value());
					if (sch == "on"){
						options.push_back(op);
						added =  true;
					}
				}
			}
			if (!added){
				options.push_back("off");
			}
			
		}
		return options;
	}
	
	return header;
	

}
int main (int argc, char *argv[]) { 
	if (argc > 3) {
		cout << "incorrect arguments!\n";
    	cout<< "usage: " << argv[0] << " <combinatorial csv file name> <options xml file name>\n";
    }
	else {
		IcgrepTest ictest;

		clearDir("../icgrep/combine/regs");
		clearDir("../icgrep/combine/files");

		ictest.prepare();

		string csv = (argc > 1)? argv[1] : "../icgrep/combine/icGrep-output.csv";
		string xml = (argc > 2)? argv[2] : "";

		ifstream file;
		file.open(csv);
		if (!file.is_open()){
			cout<<"Could not open input file\n";
		}

		string line;
		vector<string> row;
		getline(file, line);
		vector<string> header = setOptions(vectorizeLine(line), xml);
		int rownum = 1;
		while(getline(file, line)){
			row = vectorizeLine(line);

			RegexGen reGen(header, row);
			ictest.buildTest(reGen.RE, reGen.flags, reGen.syntax, rownum);
			++rownum;
		}
		ictest.execute();
		ictest.getResult();
		file.close();
		
	}

}