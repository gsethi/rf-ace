#ifndef FEATURE_HPP
#define FEATURE_HPP

#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <map>
#include <string>

#include "datadefs.hpp"

using namespace std;
using datadefs::num_t;

class Feature {
public:

  enum Type { NUM, CAT, TXT, UNKNOWN };

  vector<num_t> data;
  map<string,num_t> mapping;
  map<num_t,string> backMapping;
  vector<unordered_set<uint32_t> > hashSet;
  string name;

  Feature();
  Feature(Type newType, const string& newName, const size_t nSamples);
  Feature(const vector<num_t>& newData, const string& newName);
  Feature(const vector<string>& newStringData, const string& newName, bool doHash = false);
  ~Feature();

  void setNumSampleValue(const size_t sampleIdx, const num_t val);
  void setCatSampleValue(const size_t sampleIdx, const string& str);
  void setTxtSampleValue(const size_t sampleIdx, const string& str);

  bool isNumerical() const;
  bool isCategorical() const;
  bool isTextual() const;

  uint32_t getHash(const size_t sampleIdx, const size_t integer) const;
  bool hasHash(const size_t sampleIdx, const uint32_t hashIdx) const;

  num_t entropy() const;

private:

  Type type_;

};


#endif
