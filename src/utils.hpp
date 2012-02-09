#include <vector>
#include <set>
#include <string>
#include <istream>
#include "datadefs.hpp"
#include "treedata.hpp"

using namespace std;
using datadefs::num_t;


namespace utils {

  // Removes missing values from the provided data vector
  vector<num_t> removeNANs(const vector<num_t>& x);

  int str2int(const string& str);

  // Chomps a string, i.e. removes all the trailing end-of-line characters
  string chomp(const string& str);
  
  // Maps a delimited list of key-value pairs 
  map<string,string> keys2vals(string str, 
			       const char delimiter, 
			       const char separator);
  
  // Splits a delimited string
  vector<string> split(const string str, const char delimiter);

  // Splits a delimited stream
  vector<string> split(istream& streamObj, const char delimiter);

  // Reads a list of items from a file
  vector<string> readListFromFile(const string& fileName, const char delimiter);

  // WILL BECOME OBSOLETE
  set<string> readFeatureMask(Treedata& treeData, const string& fileName);

  // MAY BECOME OBSOLETE
  void pruneFeatures(Treedata& treeData, const string& targetName, const size_t minSamples);

}

