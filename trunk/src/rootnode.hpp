#ifndef ROOTNODE_HPP
#define ROOTNODE_HPP

#include <cstdlib>
#include <map>
#include <vector>
#include <set>
#include <utility>
#include "node.hpp"
#include "treedata.hpp"
#include "options.hpp"
#include "distributions.hpp"

class RootNode : public Node {
public:

  RootNode();

  ~RootNode();

  void reset(const size_t nNodes);

  void growTree(Treedata* trainData, const size_t targetIdx, const distributions::PMF* pmf, const ForestOptions* forestOptions, distributions::Random* random);

  Node& childRef(const size_t childIdx);
  
  size_t nNodes() const;

  num_t getTestPrediction(Treedata* treeData, const size_t sampleIdx);
  string getRawTestPrediction(Treedata* treeData, const size_t sampleIdx);

  //num_t getTrainPrediction(const size_t sampleIdx);
  //num_t getPermutedTrainPrediction(const size_t featureIdx,
  //				   const size_t sampleIdx);
  
  //vector<num_t> getTrainPrediction();

  vector<size_t> getOobIcs();

  size_t nOobSamples();

  set<size_t> getFeaturesInTree() { return( featuresInTree_ ); }

  vector<pair<size_t,size_t> > getMinDistFeatures();

  void verifyIntegrity() const;

#ifndef TEST__
private:
#endif

  size_t getTreeSizeEstimate(const size_t nSamples, const size_t nMaxLeaves, const size_t nodeSize) const;

  // Parameters that are generated only when a tree is grown
  vector<Node> children_;
  size_t nNodes_;
  vector<size_t> bootstrapIcs_;
  vector<size_t> oobIcs_;

  set<size_t> featuresInTree_;

  vector<size_t> minDistToRoot_;

};

#endif
