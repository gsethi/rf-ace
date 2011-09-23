//treedata.hpp
//
//

#ifndef TREEDATA_HPP
#define TREEDATA_HPP

#include <cstdlib>
#include <map>
#include <fstream>
#include "datadefs.hpp"
//#include "node.hpp"
#include "mtrand.h"

using namespace std;
//using datadefs::cat_t;
using datadefs::num_t;

class Treedata  {
public:
  //Initializes the object and reads in a data matrix
  Treedata(string fileName);
  Treedata(Treedata& treedata);
  Treedata(Treedata& treedata, const vector<size_t>& featureIcs);
  ~Treedata();
  
  void keepFeatures(const vector<size_t>& featureIcs);

  //Returns the number of features
  size_t nFeatures();

  num_t pearsonCorrelation(size_t featureidx1, size_t featureidx2);

  void getMatchingTargetIcs(const string& targetStr, set<size_t>& targetIcs);

  string getFeatureName(const size_t featureIdx);
  //string get_targetheader();

  //Returns the number of samples
  size_t nSamples();

  //Returns the number of real samples the target (resp. any feature) has
  size_t nRealSamples(const size_t featureIdx);
  size_t nCategories(const size_t featureIdx);

  //Prints the treedata matrix in its internal form
  void print();
  void print(const size_t featureIdx);

  void getFeatureData(const size_t featureIdx, vector<num_t>& data);
  void getFeatureData(const size_t featureIdx, const size_t sampleIdx, num_t& data);
  void getFeatureData(const size_t featureIdx, const vector<size_t>& sampleIcs, vector<num_t>& data);

  string getRawFeatureData(const size_t featureIdx, const size_t sampleIdx);

  inline void getRandomData(const size_t featureIdx, num_t& data) {data = features_[featureIdx].data[randomInteger_() % nSamples_]; }
  inline void getRandomIndex(const size_t n, size_t& idx) { idx = randomInteger_() % n; }
  
  //Generates a bootstrap sample from the real samples of featureIdx. Samples not in the bootstrap sample will be stored in oob_ics,
  //and the number of oob samples is stored in noob.
  void bootstrapFromRealSamples(const bool withReplacement, 
                                const num_t sampleSize, 
                                const size_t featureIdx, 
                                vector<size_t>& ics, 
                                vector<size_t>& oobIcs);

  void permuteContrasts();

  bool isFeatureNumerical(size_t featureIdx);

  void impurity(vector<num_t>& data, bool isFeatureNumerical, num_t& impurity, size_t& nreal);

  //WILL DECOME DEPRECATED
protected: 

  //WILL BECOME DEPRECATED
  friend class StochasticForest;

private:

  struct Feature {
    vector<num_t> data;
    bool isNumerical;
    map<string,num_t> mapping;
    map<num_t,string> backMapping;
    size_t nCategories;
    string name;
  };

  enum FileType {UNKNOWN, AFM, ARFF};

  void readFileType(string& fileName, FileType& fileType);

  void readAFM(ifstream& featurestream, vector<vector<string> >& rawMatrix, vector<string>& featureHeaders, /* vector<string>& sampleHeaders, */ vector<bool>& isFeatureNumerical);
  void readARFF(ifstream& featurestream, vector<vector<string> >& rawMatrix, vector<string>& featureHeaders, vector<bool>& isFeatureNumerical);

  void parseARFFattribute(const string& str, string& attributeName, bool& isFeatureNumerical);

  bool isValidNumericalHeader(const string& str);
  bool isValidCategoricalHeader(const string& str);
  bool isValidFeatureHeader(const string& str);

  //bool isPositiveInteger(const string& str, size_t& positiveInteger);

  //NOTE: original contents in ics will be replaced.
  void permute(vector<size_t>& ics);

  //Permutes data.
  void permute(vector<num_t>& data);
  
  template <typename T> void transpose(vector<vector<T> >& mat);
    

  vector<Feature> features_;
  size_t nSamples_;
  size_t nFeatures_;

  MTRand_int32 randomInteger_;
};

#endif
