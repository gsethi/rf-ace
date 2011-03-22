//treedata.hpp
//
//

#ifndef TREEDATA_HPP
#define TREEDATA_HPP

#include<cstdlib>
#include<map>
#include "datadefs.hpp"

using namespace std;
//using datadefs::cat_t;
using datadefs::num_t;

class Treedata 
{
public:
  //Initializes the object and reads in a data matrix
  Treedata(string fname, bool is_featurerows);
  ~Treedata();

  //Returns the number of features
  size_t nfeatures();

  //Returns the number of samples
  size_t nsamples();

  //Prints the treedata matrix in its internal form
  void print();

protected: 

  friend class Randomforest;

  //Permutes integers in range 0,1,...,(ics.size()-1). 
  //NOTE: original contents in ics will be replaced.  
  void permute(vector<size_t>& ics);

  //Permutes data in x.
  void permute(vector<num_t>& x);

  //Generates a bootstrap sample. Samples not in the bootstrap sample will be stored in oob_ics, 
  //and the number of oob samples is stored in noob.
  //NOTE: ics.size() will depend on the number of non-NaN values the current target has
  void bootstrap(vector<size_t>& ics, vector<size_t>& oob_ics, size_t& noob);

  //Selects target feature. 
  //NOTE: data will be sorted with respect to the target.
  void select_target(size_t targetidx);

  size_t get_target();
  
  size_t nrealvalues();
  size_t nrealvalues(size_t featureidx);

  //Sorts data with respect to a given feature.
  void sort_all_wrt_feature(size_t featureidx);
  
  //Sorts data with respect to target.
  void sort_all_wrt_target();

  //Given feature, finds and returns the optimal split point wrt. sampleics. 
  //Samples branching left and right will be stored in sampleics_left (resp. right)
  void split_target_wrt_feature(size_t featureidx,
				const size_t min_split,
				vector<size_t>& sampleics,
				vector<size_t>& sampleics_left,
				vector<size_t>& sampleics_right);
  
  void split_target(const size_t min_split,
		    vector<size_t>& sampleics,
		    vector<size_t>& sampleics_left,
		    vector<size_t>& sampleics_right);
  
  void range(vector<size_t>& ics);

  void impurity(size_t featureidx, vector<size_t>& sampleics, num_t& impurity);

private:

  void incremental_num_target_split(const size_t min_split, 
				    vector<size_t>& sampleics, 
				    vector<size_t>& sampleics_left, 
				    vector<size_t>& sampleics_right);

  void categorical_cat_target_split(vector<size_t>& sampleics, vector<size_t>& sampleics_left, vector<size_t>& sampleics_right);

  //Splits a set of samples to "left" and "right", given a splitidx
  void split_samples(vector<size_t>& sampleics,
		      size_t splitidx,
		      vector<size_t>& sampleics_left,
		      vector<size_t>& sampleics_right);

  //Splits a set of samples to "left" and "right", given a set of categories
  void split_samples(size_t featureidx,
		     vector<size_t>& sampleics,
		     set<num_t>& categories_left,
		     vector<size_t>& sampleics_left,
		     vector<size_t>& sampleics_right);
    

    
  void count_real_values(size_t featureidx, size_t& nreal);

  //These could be moved to datadefs::
  template <typename T1,typename T2> void make_pairedv(vector<T1> const& v1, 
						       vector<T2> const& v2, 
						       vector<pair<T1,T2> >& p);

  template <typename T1,typename T2> void separate_pairedv(vector<pair<T1,T2> > const& p, 
							   vector<T1>& v1, 
							   vector<T2>& v2);

  template <typename T> void sort_and_make_ref(vector<T>& v, vector<size_t>& ref_ics);

  //Sorts a given input data vector of type T based on a given reference ordering of type vector<int>
  template <typename T> void sort_from_ref(vector<T>& in, vector<size_t> const& ref_ics);

  size_t targetidx_;

  vector<vector<num_t> > featurematrix_;
  vector<bool> isfeaturenum_;
  vector<size_t> nrealvalues_;
  vector<size_t> ncatvalues_;

  size_t nsamples_;
  size_t nfeatures_;

  vector<string> featureheaders_;
  vector<string> sampleheaders_;

};

#endif
