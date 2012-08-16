//treedata.hpp
//
//

#ifndef TREEDATA_HPP
#define TREEDATA_HPP

#include <cstdlib>
#include <map>
#include <fstream>
#include <unordered_map>

#include "datadefs.hpp"
#include "distributions.hpp"
#include "options.hpp"

using namespace std;
using datadefs::num_t;

struct Feature {
  vector<num_t> data;
  //vector<size_t> sortOrder;
  bool isNumerical;
  map<string,num_t> mapping;
  map<num_t,string> backMapping;
  string name;

  Feature();
  Feature(const vector<num_t>& newData, const string& newName);
  Feature(const vector<string>& newStringData, const string& newName);

};

class Treedata  {
public:

  // Initializes the object 
  Treedata(const vector<Feature>& features, options::General_options* parameters, const vector<string>& sampleHeaders = vector<string>(0) );

  // Initializes the object and reads in a data matrix
  // NOTE: will permute the contrasts, which is why it needs the RNG
  Treedata(string fileName, options::General_options* parameters);

  ~Treedata();

  

  // Takes a set of features that are to be retained in the Treedata object.
  // Others will be removed.
  void whiteList(const set<string>& featureNames);

  // Takes a set of features that are to be removed from the Treedata object.
  // Others will be retained.
  void blackList(const set<string>& featureNames);

  // Takes a boolean vector of flags that mark which feature is to be removed (0) and
  // which to be retained (1) in the Treedata object.
  void whiteList(const vector<bool>& featureIcs);

  // Returns the number of features
  size_t nFeatures();

  // Calculates Pearson Correlation
  // TODO: WILL BECOME OBSOLETE
  num_t pearsonCorrelation(size_t featureidx1, size_t featureidx2);

  // Returns feature index, given the name
  size_t getFeatureIdx(const string& featureName);

  // A value denoting the "one-over-last" feature in matrix
  size_t end() { return( datadefs::MAX_IDX ); }

  // Returns feature name, given the index
  string getFeatureName(const size_t featureIdx);

  // Returns sample name, given sample index
  string getSampleName(const size_t sampleIdx);

  // Returns the number of samples
  size_t nSamples();

  // Returns the number of real samples the feature has
  size_t nRealSamples(const size_t featureIdx);
  size_t nRealSamples(const size_t featureIdx1, const size_t featureIdx2);
  
  // Returns the number of categories a feature has
  size_t nCategories(const size_t featureIdx);

  vector<string> categories(const size_t featureIdx);

  // Returns the number of categories of the feature with the highest cardinality
  size_t nMaxCategories();

  // Prints the treedata matrix in its internal form
  void print();
  void print(const size_t featureIdx);

  vector<num_t> getFeatureData(const size_t featureIdx);
  num_t getFeatureData(const size_t featureIdx, const size_t sampleIdx);
  vector<num_t> getFeatureData(const size_t featureIdx, const vector<size_t>& sampleIcs);

  vector<num_t> getFilteredFeatureData(const size_t featureIdx,
				       vector<size_t>& sampleIcs);

  num_t numericalFeatureSplit(const size_t targetIdx,
			      const size_t featureIdx,
			      const size_t minSamples,
			      vector<size_t>& sampleIcs_left,
			      vector<size_t>& sampleIcs_right,
			      num_t& splitValue);

  num_t categoricalFeatureSplit(const size_t targetIdx,
				const size_t featureIdx,
				const size_t minSamples,
				vector<size_t>& sampleIcs_left,
				vector<size_t>& sampleIcs_right,
				set<num_t>& splitValues_left,
				set<num_t>& splitValues_right);

  void getFilteredFeatureDataPair(const size_t featureIdx1, 
				  const size_t featureIdx2, 
				  vector<size_t>& sampleIcs, 
				  vector<num_t>& featureData1, 
				  vector<num_t>& featureData2);
  
  /*
    void getFilteredAndSortedFeatureDataPair(const size_t targetIdx, 
    const size_t featureIdx, 
    vector<size_t>& sampleIcs, 
    vector<num_t>& targetData, 
    vector<num_t>& featureData);
        
    void getFilteredAndSortedFeatureDataPair2(const size_t targetIdx,
    const size_t featureIdx,
    vector<size_t>& sampleIcs,
    vector<num_t>& targetData,
    vector<num_t>& featureData);
  */    
    
  void getFilteredAndSortedFeatureDataPair3(const size_t targetIdx,
					    const size_t featureIdx,
					    vector<size_t>& sampleIcs,
					    vector<num_t>& targetData,
					    vector<num_t>& featureData);
  


  string getRawFeatureData(const size_t featureIdx, const size_t sampleIdx);
  string getRawFeatureData(const size_t featureIdx, const num_t data);
  vector<string> getRawFeatureData(const size_t featureIdx);
  
  // Generates a bootstrap sample from the real samples of featureIdx. Samples not in the bootstrap sample will be stored in oob_ics,
  // and the number of oob samples is stored in noob.
  void bootstrapFromRealSamples(distributions::RandInt& randInt,
				const bool withReplacement, 
                                const num_t sampleSize, 
                                const size_t featureIdx, 
                                vector<size_t>& ics, 
                                vector<size_t>& oobIcs);

  void createContrasts();
  void permuteContrasts(distributions::RandInt& randInt);

  bool isFeatureNumerical(const size_t featureIdx);

  void replaceFeatureData(const size_t featureIdx, const vector<num_t>& featureData);
  void replaceFeatureData(const size_t featureIdx, const vector<string>& rawFeatureData);

  
#ifndef TEST__
private:
#endif
  
  enum FileType {UNKNOWN, AFM, ARFF};

  void readFileType(string& fileName, FileType& fileType);

  void readAFM(ifstream& featurestream, 
	       vector<vector<string> >& rawMatrix, 
	       vector<string>& featureHeaders, 
	       vector<string>& sampleHeaders, 
	       vector<bool>& isFeatureNumerical);
  
  void readARFF(ifstream& featurestream, 
		vector<vector<string> >& rawMatrix, 
		vector<string>& featureHeaders, 
		vector<bool>& isFeatureNumerical);

  void parseARFFattribute(const string& str, string& attributeName, bool& isFeatureNumerical);

  bool isValidNumericalHeader(const string& str);
  bool isValidCategoricalHeader(const string& str);
  bool isValidFeatureHeader(const string& str);

  template <typename T> void transpose(vector<vector<T> >& mat);

  options::General_options* parameters_;
  
  vector<Feature> features_;
  vector<string> sampleHeaders_;

  unordered_map<string,size_t> name2idx_;
  
};

#endif
