#ifndef STOCHASTICFOREST_HPP
#define STOCHASTICFOREST_HPP

#include<cstdlib>
#include "rootnode.hpp"
#include "treedata.hpp"

using namespace std;

class StochasticForest {
public:

  StochasticForest(Treedata* treeData, const size_t targetIdx);  
  ~StochasticForest();

  void learnRF(const size_t nTrees,
	       const size_t mTry,
	       const size_t nodeSize,
	       const bool useContrasts,
	       const bool isOptimizedNodeSplit);

  void learnGBT(const size_t nTrees,
		const size_t nMaxLeaves,
		const num_t shrinkage,
		const num_t subSampleSize);
  
  //Calculates the feature importance scores for real and contrast features
  vector<size_t> featureFrequency();
  
  vector<num_t> featureImportance();
  
  //Counts the number of nodes in the forest
  size_t nNodes();


private:

  void growNumericalGBT(const size_t nTrees,
                        const num_t shrinkage);

  void growCategoricalGBT(const size_t nTrees,
                          const size_t numClasses,
                          const num_t shrinkage);


  //Will be moved elsewhere!
  void transformLogistic(const size_t numClasses, vector<num_t>& prediction, vector<num_t>& probability);

  num_t predictSampleByTree(size_t sampleIdx, size_t treeIdx);
  void predictDatasetByTree(size_t treeIdx, vector<num_t>& curPrediction);

  //Percolates samples along the trees, starting from the rootNode. Spits out a map<> that ties the percolated train samples to the leaf nodes
  //NOTE: currently, since there's no implementation for a RootNode class, there's no good way to store the percolated samples in the tree, but a map<> is generated instead
  void percolateSampleIcs(Node* rootNode, vector<size_t>& sampleIcs, map<Node*,vector<size_t> >& trainIcs);
  void percolateSampleIcsAtRandom(size_t featureIdx, Node* rootNode, vector<size_t>& sampleIcs, map<Node*,vector<size_t> >& trainIcs);
  
  void percolateSampleIdx(size_t sampleIdx, Node** nodep);
  void percolateSampleIdxAtRandom(size_t featureIdx, size_t sampleIdx, Node** nodep);

  //Given the map<>, generated with the percolation functions, a tree impurity score is outputted
  void treeImpurity(map<Node*,vector<size_t> >& trainIcs, num_t& impurity);

  //Pointer to treeData_ object, stores all the feature data
  Treedata* treeData_;

  //Chosen target to regress on
  size_t targetIdx_;
  
  //Size of the forests
  //size_t nTrees_;

  //Root nodes for every tree
  vector<RootNode*> rootNodes_;

  //A helper variable that stores all splitter features. This will make impurity calculation faster
  map<size_t, set<size_t> > featuresInForest_;

  //Out-of-box samples for each tree
  vector<vector<size_t> > oobMatrix_;
  
};

#endif
