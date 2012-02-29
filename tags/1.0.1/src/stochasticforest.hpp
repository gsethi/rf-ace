#ifndef STOCHASTICFOREST_HPP
#define STOCHASTICFOREST_HPP

#include<cstdlib>
#include "rootnode.hpp"
#include "treedata.hpp"
#include "partitionsequence.hpp"

using namespace std;

/*
  TODO: inherit RF and GBT from this class, to simplify the structure
*/
class StochasticForest {
public:

  // Grow a forest based on data
  StochasticForest(Treedata* treeData, const string& targetName, const size_t nTrees);  
  
  // Load an existing forest
  StochasticForest(Treedata* treeData, const string& forestFile);

  ~StochasticForest();


  // !! Documentation: ... then, on the public side, learning would become collapsed into a single overloaded function learn(...)
  void learnRF(const num_t  mTryFraction,
	       const size_t nMaxLeaves,
	       const size_t nodeSize,
	       const bool   useContrasts);

  void learnGBT(const size_t nMaxLeaves,
		const num_t  shrinkage,
		const num_t  subSampleSize);
  
  vector<size_t> featureFrequency();
  
  //Calculates the feature importance scores for real and contrast features
  vector<num_t> featureImportance();
    
  void predict(vector<string>& categoryPrediction, vector<num_t>& confidence);
  void predict(vector<num_t>& prediction, vector<num_t>& confidence);

  //Counts the number of nodes in the forest
  vector<size_t> nNodes();
  size_t nNodes(const size_t treeIdx);

  size_t nTrees();

  inline string getTargetName() { return( targetName_ ); }
  inline bool isTargetNumerical() { return( targetSupport_.size() == 0 ? true : false ); }
  
  string type();

  void printToFile(const string& fileName);

#ifndef TEST__
private:
#endif

  void growNumericalGBT();
  void growCategoricalGBT();

  // TODO: StochasticForest::transformLogistic() should be moved elsewhere
  void transformLogistic(vector<num_t>& prediction, vector<num_t>& probability);

  // TODO: predictSampleByTree() and percolateSampleIdx families in StochasticForest need to be fused together 
  num_t predictSampleByTree(size_t sampleIdx, size_t treeIdx);
  vector<num_t> predictDatasetByTree(size_t treeIdx);

  //Percolates samples along the trees, starting from the rootNode. Spits out a map<> that ties the percolated train samples to the leaf nodes
  map<Node*,vector<size_t> > percolateSampleIcs(Node* rootNode, const vector<size_t>& sampleIcs);
  map<Node*,vector<size_t> > percolateSampleIcsAtRandom(const size_t featureIdx, Node* rootNode, const vector<size_t>& sampleIcs);
  
  void percolateSampleIdx(const size_t sampleIdx, Node** nodep);
  void percolateSampleIdxAtRandom(const size_t featureIdx, const size_t sampleIdx, Node** nodep);

  // Calculates prediction error across the nodes provided in the input map<> 
  num_t predictionError(const map<Node*,vector<size_t> >& trainIcs);

  //Pointer to treeData_ object, stores all the feature data with which the trees are grown (i.e. training data)
  Treedata* treeData_;

  //Chosen target to regress on
  string targetName_;
  vector<string> targetSupport_;

  size_t nTrees_;

  //Root nodes for every tree
  vector<RootNode*> rootNodes_;

  //A helper variable that stores all splitter features. This will make impurity calculation faster
  map<size_t, set<size_t> > featuresInForest_;

  //Out-of-box samples for each tree
  vector<vector<size_t> > oobMatrix_;

  enum LearnedModel {NO_MODEL, RF_MODEL, GBT_MODEL};
  
  LearnedModel learnedModel_;

  num_t shrinkage_;

};

#endif
