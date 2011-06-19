//treedata.hpp
//
//

#ifndef TREEDATA_HPP
#define TREEDATA_HPP

#include <cstdlib>
#include <map>
#include <fstream>
#include "datadefs.hpp"
#include "node.hpp"
#include "mtrand.h"

using namespace std;
//using datadefs::cat_t;
using datadefs::num_t;

class Treedata 
{
public:
  //Initializes the object and reads in a data matrix
  Treedata(string filename);
  ~Treedata();

  //Returns the number of features
  size_t nfeatures();

  num_t corr(size_t featureidx1, size_t featureidx2);

  string get_featureheader(size_t featureidx);
  string get_targetheader();

  //Returns the number of samples
  size_t nsamples();

  //Returns the number of real samples the target (resp. any feature) has
  size_t nrealsamples();
  size_t nrealsamples(size_t featureidx);

  //Prints the treedata matrix in its internal form
  void print();
  void print(const size_t featureidx);

protected: 

  friend class Randomforest;
  friend class GBT;

  void kill(const size_t featureidx);

  //Permutes integers in range 0,1,...,(ics.size()-1). 
  //NOTE: original contents in ics will be replaced.  
  void permute(vector<size_t>& ics);

  //Permutes data in x.
  void permute(vector<num_t>& x);

  //Generates a bootstrap sample. Samples not in the bootstrap sample will be stored in oob_ics, 
  //and the number of oob samples is stored in noob.
  //NOTE: ics.size() will depend on the number of non-NaN values the current target has
  void bootstrap(vector<size_t>& ics, vector<size_t>& oob_ics);

  //Selects target feature. 
  //NOTE: data will be sorted with respect to the target.
  void select_target(size_t targetidx);

  size_t get_target();
  
  void permute_contrasts();

  bool isfeaturenum(size_t featureidx);

  //size_t nrealvalues();
  //size_t nrealvalues(size_t featureidx);

  void remove_nans(size_t featureidx, vector<size_t>& sampleics, size_t& nreal);
  

  //Given feature, finds and returns the optimal split point wrt. sampleics. 
  //Samples branching left and right will be stored in sampleics_left (resp. right)
  void split_target(size_t featureidx,
		    const size_t min_split,
		    vector<size_t>& sampleics,
		    vector<size_t>& sampleics_left,
		    vector<size_t>& sampleics_right,
		    num_t& splitvalue,
		    set<num_t>& values_left);
  
    
  num_t split_fitness(const size_t featureidx,
		      const size_t min_split,
		      vector<size_t> const& sampleics,
		      vector<size_t> const& sampleics_left,
		      vector<size_t> const& sampleics_right);

  
  void impurity(size_t featureidx, vector<size_t> const& sampleics, num_t& impurity, size_t& nreal);
  

  num_t at(size_t featureidx, size_t sampleidx);
  num_t randf(size_t featureidx);

  inline size_t randidx(const size_t n) { return(irand_() % n); }

private:

  enum Filetype {UNKNOWN, AFM, ARFF};

  void read_filetype(string& filename, Filetype& filetype);

  void read_afm(ifstream& featurestream, vector<vector<string> >& rawmatrix);
  void read_arff(ifstream& featurestream, vector<vector<string> >& rawmatrix);

  void parse_arff_attribute(const string& str, vector<string>& fields);

  bool is_featureheader(const string& str);

  template <typename T> void transpose(vector<vector<T> >& mat);

  //Sorts data with respect to a given feature
  void sort_all_wrt_feature(size_t featureidx);

  //Sorts data with respect to target
  void sort_all_wrt_target();

  //Finds the best split for target with respect to selected feature splitter, which needs to be numerical.
  void incremental_target_split(size_t featureidx,
				const size_t min_split, 
				vector<size_t>& sampleics, 
				vector<size_t>& sampleics_left, 
				vector<size_t>& sampleics_right,
				num_t& splitvalue);

  void categorical_target_split(size_t featureidx,
				   vector<size_t>& sampleics,
				   vector<size_t>& sampleics_left,
				   vector<size_t>& sampleics_right,
				   set<num_t>& categories_left);

  //Splits a set of samples to "left" and "right", given a splitidx
  void split_samples(vector<size_t>& sampleics,
		      size_t splitidx,
		      vector<size_t>& sampleics_left,
		      vector<size_t>& sampleics_right);  
  
  struct Feature
  {
    vector<num_t> data;
    vector<num_t> contrast;
    string name;
    bool isnumeric;
  };

  size_t targetidx_;

  vector<Feature> features_;

  vector<vector<num_t> > featurematrix_;
  vector<vector<num_t> > contrastmatrix_;
  vector<bool> isfeaturenum_;
  size_t nrealsamples_;
  //vector<size_t> ncatvalues_;

  size_t nsamples_;
  size_t nfeatures_;

  vector<string> featureheaders_;
  vector<string> sampleheaders_;

  MTRand_int32 irand_;
};

#endif