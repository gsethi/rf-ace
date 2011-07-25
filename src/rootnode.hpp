#ifndef ROOTNODE_HPP
#define ROOTNODE_HPP

#include <cstdlib>
#include "node.hpp"
#include "treedata.hpp"

class RootNode : public Node 
{
public:

  
  RootNode(bool sampleWithReplacement,
	   num_t sampleSizeFraction,
	   size_t maxNodesToStop,
	   size_t minNodeSizeToStop,
	   bool isRandomSplit,
	   size_t nFeaturesForSplit,
	   bool useContrasts,
	   bool isOptimizedNodeSplit,
	   size_t numClasses);
  
  //~RootNode();
  

  void growTree(Treedata* treeData,
		const size_t targetIdx,
		void (*leafPrediction)(const vector<num_t>&,num_t&,const size_t),
		vector<size_t>& oobIcs,
		set<size_t>& featuresInTree,
		size_t& nNodes);

  /*
    size_t targetIdx();
    bool isTargetNumerical();
    bool sampleWithReplacement();
    size_t maxNodesToStop();
    size_t minNodeSizeToStop();
    bool isRandomSplit();
    size_t featureSampleSize();
    bool useContrasts();
  */
  
private:

  GrowInstructions GI_;
  
  /*
    bool sampleWithReplacement_;
    num_t sampleSizeFraction_;
    size_t maxNodesToStop_;
    size_t minNodeSizeToStop_;
    bool isRandomSplit_;
    size_t nFeaturesForSplit_;
    bool useContrasts_;
  */

};

#endif
