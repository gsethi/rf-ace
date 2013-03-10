#ifndef NODE_NEWTEST_HPP
#define NODE_NEWTEST_HPP

#include <cstdlib>

#include "newtest.hpp"
#include "node.hpp"
#include "datadefs.hpp"

using namespace std;
using datadefs::num_t;

void node_newtest_getChildLeaves();
void node_newtest_setSplitter();
void node_newtest_percolateData();
void node_newtest_getLeafTrainPrediction();
void node_newtest_hasChildren();
void node_newtest_recursiveNodeSplit();
void node_newtest_cleanPairVectorFromNANs();
void node_newtest_recursiveNDescendantNodes();
void node_newtest_regularSplitterSeek();

void node_newtest() {

  newtest( "getChildLeaves(x)", &node_newtest_getChildLeaves );
  newtest( "setSplitter(x)", &node_newtest_setSplitter );
  newtest( "percolateData(x)", &node_newtest_percolateData );
  newtest( "getLeafTrainPrediction(x)", &node_newtest_getLeafTrainPrediction );
  newtest( "hasChildren(x)", &node_newtest_hasChildren );
  newtest( "recursiveNodeSplit(x)", &node_newtest_recursiveNodeSplit );
  newtest( "cleanPairVectorFromNANs(x)", &node_newtest_cleanPairVectorFromNANs );
  newtest( "recursiveNDescendantNodes(x)", &node_newtest_recursiveNDescendantNodes );
  newtest( "regularSplitterSeek(x)", &node_newtest_regularSplitterSeek );


}

void node_newtest_getChildLeaves() {

  Node node,nodeL,nodeR,nodeM,nodeLL,nodeLR;

  node.setSplitter("foo",5.0,nodeL,nodeR);
  nodeL.setSplitter("bar",6.0,nodeLL,nodeLR);
  node.missingChild_ = &nodeM;

  nodeLL.setTrainData({1,2,3});
  nodeLR.setTrainData({4,5});
  nodeR.setTrainData({6});
  nodeM.setTrainData({7});

  vector<Node*> childLeaves = node.getChildLeaves();

  set<Node*> childLeavesSet(childLeaves.begin(),childLeaves.end());

  newassert( childLeaves.size() == 4 );
  newassert( childLeavesSet.size() == 4 );
  newassert( childLeavesSet.find(&nodeLL) != childLeavesSet.end() );
  newassert( childLeavesSet.find(&nodeLR) != childLeavesSet.end() );
  newassert( childLeavesSet.find(&nodeR) != childLeavesSet.end() );
  newassert( childLeavesSet.find(&nodeM) != childLeavesSet.end() );

  childLeaves = nodeR.getChildLeaves();
  newassert( childLeaves.size() == 1 );
  newassert( childLeaves[0] == &nodeR );

  childLeaves = nodeM.getChildLeaves();
  newassert( childLeaves.size() == 1 );
  newassert( childLeaves[0] == &nodeM );

  childLeaves = nodeL.getChildLeaves();
  newassert( childLeaves.size() == 2 );
  childLeavesSet = set<Node*>(childLeaves.begin(),childLeaves.end());
  newassert( childLeavesSet.find(&nodeLL) != childLeavesSet.end() );
  newassert( childLeavesSet.find(&nodeLR) != childLeavesSet.end() );
  
  vector<num_t> trainData = nodeLL.getTrainData();
  set<num_t> trainDataSet(trainData.begin(),trainData.end());

  newassert( trainDataSet.find(1) != trainDataSet.end() );
  newassert( trainDataSet.find(2) != trainDataSet.end() );
  newassert( trainDataSet.find(3) != trainDataSet.end() );
  
}


void node_newtest_setSplitter() {
  
  size_t splitterIdx = 3;
  datadefs::num_t splitLeftLeqValue = 0.5; 
  datadefs::num_t leftFraction = 0.5;
  
  //Splitter::Splitter splitter(0.5);
  
  Node node,leftChild,rightChild;

  node.setSplitter("foo",splitLeftLeqValue,leftChild,rightChild);
  
  //newassert( node.splitterIdx() == splitterIdx );
  newassert( node.splitter_.type == Feature::Type::NUM );
  newassert( fabs(node.splitter_.leftLeqValue - splitLeftLeqValue) < datadefs::EPS );
  //newassert( fabs(node.splitter_.leftFraction - leftFraction) < datadefs::EPS );
  
}

void node_newtest_percolateData() {
  
  Treedata treeData("test_2by10_text_matrix.afm",'\t',':');

  uint32_t h;

  MurmurHash3_x86_32("c",1,0,&h);

  Node node,leftChild,rightChild;

  node.setSplitter("T:in",h,leftChild,rightChild);
  
  newassert( &leftChild == node.leftChild() );
  newassert( &rightChild == node.rightChild() );

  newassert( NULL == node.missingChild() );

  newassert( node.percolate(&treeData,0,1) == &rightChild );
  newassert( node.percolate(&treeData,1,1) == &rightChild );
  newassert( node.percolate(&treeData,2,1) == &rightChild );
  newassert( node.percolate(&treeData,3,1) == &rightChild );
  newassert( node.percolate(&treeData,4,1) == &rightChild );
  newassert( node.percolate(&treeData,5,1) == &leftChild );
  newassert( node.percolate(&treeData,6,1) == &leftChild );
  newassert( node.percolate(&treeData,7,1) == &leftChild );
  newassert( node.percolate(&treeData,8,1) == &leftChild );
  newassert( node.percolate(&treeData,9,1) == &leftChild );
  newassert( node.percolate(&treeData,10,1) == &leftChild );
  newassert( node.percolate(&treeData,11,1) == &leftChild );
  newassert( node.percolate(&treeData,12,1) == &leftChild );
  newassert( node.percolate(&treeData,13,1) == &leftChild );
  newassert( node.percolate(&treeData,14,1) == &leftChild );
  newassert( node.percolate(&treeData,15,1) == &rightChild );
  newassert( node.percolate(&treeData,16,1) == &rightChild );
  newassert( node.percolate(&treeData,17,1) == &rightChild );
  newassert( node.percolate(&treeData,18,1) == &rightChild );
  newassert( node.percolate(&treeData,19,1) == &rightChild );


}

void node_newtest_regularSplitterSeek() {

}

void node_newtest_getLeafTrainPrediction() {
}

void node_newtest_hasChildren() {
}

void node_newtest_recursiveNodeSplit() { 
}

void node_newtest_cleanPairVectorFromNANs() { 

}

void node_newtest_recursiveNDescendantNodes() {
  
}

#endif
