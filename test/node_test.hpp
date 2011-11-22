#ifndef NODETEST_HPP
#define NODETEST_HPP

#include <cppunit/extensions/HelperMacros.h>
#include "datadefs.hpp"
#include "node.hpp"
#include "errno.hpp"

class NodeTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE( NodeTest );
  CPPUNIT_TEST( test_setSplitter );
  CPPUNIT_TEST( test_setSplitter );
  CPPUNIT_TEST( test_getSplitter );
  CPPUNIT_TEST( test_percolateData );
  CPPUNIT_TEST( test_getLeafTrainPrediction );
  CPPUNIT_TEST( test_hasChildren );
  CPPUNIT_TEST( test_leafMean );
  CPPUNIT_TEST( test_leafMode );
  CPPUNIT_TEST( test_leafGamma );
  CPPUNIT_TEST( test_recursiveNodeSplit );
  CPPUNIT_TEST( test_cleanPairVectorFromNANs );
  CPPUNIT_TEST( test_numericalFeatureSplit );
  CPPUNIT_TEST( test_categoricalFeatureSplit );
  CPPUNIT_TEST( test_splitFitness );
  CPPUNIT_TEST( test_recursiveNDescendantNodes );
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp();
  void tearDown();

  void test_setSplitter();
  //void test_setSplitter();
  void test_getSplitter();
  void test_percolateData();
  void test_getLeafTrainPrediction();
  void test_hasChildren();
  void test_leafMean();
  void test_leafMode();
  void test_leafGamma();
  void test_recursiveNodeSplit();
  void test_cleanPairVectorFromNANs();
  void test_numericalFeatureSplit();
  void test_categoricalFeatureSplit();
  void test_splitFitness();
  void test_recursiveNDescendantNodes();

};

void NodeTest::setUp() {}
void NodeTest::tearDown() {}

void NodeTest::test_setSplitter() {
  
  size_t splitterIdx = 3;
  datadefs::num_t splitLeftLeqValue = 0.5; 

  //Splitter::Splitter splitter(0.5);

  Node::Node node;

  CPPUNIT_ASSERT( !node.splitter_ );

  node.setSplitter(splitterIdx,splitLeftLeqValue);

  CPPUNIT_ASSERT( node.splitterIdx_ == splitterIdx );
  CPPUNIT_ASSERT( node.splitter_->splitterType_ == Splitter::NUMERICAL_SPLITTER );
  CPPUNIT_ASSERT( fabs(node.splitter_->splitLeftLeqValue_ - splitLeftLeqValue) < datadefs::EPS );
  
  
}

void NodeTest::test_getSplitter() {
  //Node::getSplitter(...);
}

void NodeTest::test_percolateData() {
  //Node::percolateData(...);
}

void NodeTest::test_getLeafTrainPrediction() {
}

void NodeTest::test_hasChildren() {
}

void NodeTest::test_leafMean() {
  /*
  vector<datadefs::num_t> data;
  datadefs::num_t mu;
  size_t nRealValues;

  for (int i = 0; i < 50; ++i) {
    data.push_back(static_cast<datadefs::num_t>(i));
  }

  // Spuriously assign the values of mu and nRealValues, since they'll be
  //  flattened during function invocation
  mu = -1.0;
  nRealValues = static_cast<size_t>(-1);

  Node::leafMean(data, mu, nRealValues);
  CPPUNIT_ASSERT(mu == 24.5);
  CPPUNIT_ASSERT(nRealValues == 50);

  // Our data vector is defined as const in our signature. Since we're not
  //  testing edge cases of non-trivial memory corruption, we ignore it here.

  // Interleave the original input with NaNs; verify we get the same results
  for (int i = 0; i < 50; ++i) {
    data.insert(data.begin() + (i*2), datadefs::NUM_NAN);
  }

  mu = -1.0;
  nRealValues = static_cast<size_t>(-1);

  Node::leafMean(data, mu, nRealValues);
  CPPUNIT_ASSERT(mu == 24.5);
  CPPUNIT_ASSERT(nRealValues == 50);

  // Ensure a vector containing only NaNs is handled properly
  data.clear();
  for (int i = 0; i < 50; ++i) {
    data.push_back(datadefs::NUM_NAN);
  }

  mu = -1.0;
  nRealValues = static_cast<size_t>(-1);

  Node::leafMean(data, mu, nRealValues);
  CPPUNIT_ASSERT(mu == 0.0);
  CPPUNIT_ASSERT(nRealValues == 0);
  */
}

void NodeTest::test_leafMode() {
  /*
  vector<datadefs::num_t> data;
  datadefs::num_t leafMode = -1.0;
  size_t numClasses = static_cast<size_t>(-1);

  data.push_back(static_cast<datadefs::num_t>(10));
  for (int i = 0; i < 50; ++i) {
    data.push_back(static_cast<datadefs::num_t>(i));
  }

  Node::leafMode(data, leafMode, numClasses);
  CPPUNIT_ASSERT(leafMode == 10.0);

  // numClasses is defined as const in our signature. Since we're not testing
  //  edge cases of non-trivial memory corruption, we ignore it here.

  // Interleave the original input with NaNs; verify we get the same results
  for (int i = 0; i < 50; ++i) {
    data.insert(data.begin() + (i*2), datadefs::NUM_NAN);
  }

  leafMode = -1.0;
  numClasses = static_cast<size_t>(-1);

  Node::leafMode(data, leafMode, numClasses);
  CPPUNIT_ASSERT(leafMode == 10.0);

  // Ensure a vector containing only NaNs is handled as expected
  data.clear();
  for (int i = 0; i < 50; ++i) {
    data.push_back(datadefs::NUM_NAN);
  }

  leafMode = -1.0;
  numClasses = static_cast<size_t>(-1);

  Node::leafMode(data, leafMode, numClasses);
  CPPUNIT_ASSERT(leafMode == 0.0);
  */
}

void NodeTest::test_leafGamma() {
  /*
  vector<datadefs::num_t> data;
  datadefs::num_t leafGamma = -1.0;
  size_t numClasses = 5;

  for (int i = 0; i < 50; ++i) {
    data.push_back(static_cast<datadefs::num_t>(i));
  }

  Node::leafGamma(data, leafGamma, numClasses);
  CPPUNIT_ASSERT(leafGamma == -0.025);

  // numClasses is defined as const in our signature. Since we're not testing
  //  edge cases of non-trivial memory corruption, we ignore it here.

  // Interleave the original input with NaNs; verify we get the same results
  for (int i = 0; i < 50; ++i) {
    data.insert(data.begin() + (i*2), datadefs::NUM_NAN);
  }

  leafGamma = -1.0;
  numClasses = 5;

  Node::leafGamma(data, leafGamma, numClasses);
  CPPUNIT_ASSERT(leafGamma == -0.025);

  // Ensure a vector containing only NaNs is handled as expected
  data.clear();
  for (int i = 0; i < 50; ++i) {
    data.push_back(datadefs::NUM_NAN);
  }

  leafGamma = -1.0;
  numClasses = 5;

  Node::leafGamma(data, leafGamma, numClasses);
  CPPUNIT_ASSERT(leafGamma == 0.0);
  */
}

void NodeTest::test_recursiveNodeSplit() { 
}

void NodeTest::test_cleanPairVectorFromNANs() { 

}

void NodeTest::test_numericalFeatureSplit() { 

  vector<datadefs::num_t> featureData(8);
  vector<datadefs::num_t> targetData(8);

  featureData[0] = 3;
  featureData[1] = 2;
  featureData[2] = datadefs::NUM_NAN;
  featureData[3] = 2.4;
  featureData[4] = 5;
  featureData[5] = 4;
  featureData[6] = 2.9;
  featureData[7] = 3.1;

  targetData[0] = 1;
  targetData[1] = 3;
  targetData[2] = 2;
  targetData[3] = datadefs::NUM_NAN;
  targetData[4] = 4;
  targetData[5] = 5;
  targetData[6] = 3.6;
  targetData[7] = 2.8;

  vector<size_t> sampleIcs_left,sampleIcs_right;
  bool isTargetNumerical = true;
  
  datadefs::num_t splitValue;
  datadefs::num_t splitFitness;

  Node node;

  Node::GrowInstructions GI;
  GI.minNodeSizeToStop = 2;

  node.numericalFeatureSplit(targetData,
			     isTargetNumerical,
			     featureData,
			     GI,
			     sampleIcs_left,
			     sampleIcs_right,
			     splitValue,
			     splitFitness);

  Splitter::Splitter splitter(splitValue);

  CPPUNIT_ASSERT( splitter.splitsLeft(3.1 - 0.00001) );
  CPPUNIT_ASSERT( splitter.splitsRight(3.1) );

  CPPUNIT_ASSERT( fabs(splitFitness - 0.530492285084497) < 1e-10 );

  CPPUNIT_ASSERT(sampleIcs_left.size() == 4);
  for(size_t i = 0; i < sampleIcs_left.size(); ++i) {
    size_t idx = sampleIcs_left[i];
    CPPUNIT_ASSERT(idx == 1 || idx == 6 || idx == 0 || idx == 7);
  }

  CPPUNIT_ASSERT(sampleIcs_right.size() == 2);
  for(size_t i = 0; i < sampleIcs_right.size(); ++i) {
    size_t idx = sampleIcs_right[i];
    CPPUNIT_ASSERT(idx == 5 || idx == 4);
  }

}

void NodeTest::test_categoricalFeatureSplit() { 

  size_t n = 10;

  vector<datadefs::num_t> featureData(n);
  vector<datadefs::num_t> targetData(n,0);

  for ( size_t i = 0; i < n; ++i ) {
    featureData[i] = static_cast<datadefs::num_t>(i);
  }

  for ( size_t i = 0; i < n / 2; ++i ) {
    targetData[i] = 1;
  }

  vector<size_t> sampleIcs_left,sampleIcs_right;
  //bool isTargetNumerical = false;
  set<datadefs::num_t> splitValues_left,splitValues_right;
  datadefs::num_t splitFitness;

  Node node;
  PartitionSequence PS(n);

  Node::GrowInstructions GI;
  GI.minNodeSizeToStop = 2;
  GI.partitionSequence = &PS;

  size_t iter = 0;
  bool isTargetNumerical;
  while ( iter < 2 ) {

    if ( iter == 0 ) {
      isTargetNumerical = false;
    } else {
      isTargetNumerical = true;
    }
    ++iter;
 
    node.categoricalFeatureSplit(targetData,
				 isTargetNumerical,
				 featureData,
				 GI,
				 sampleIcs_left,
				 sampleIcs_right,
				 splitValues_left,
				 splitValues_right,
				 splitFitness);
    
    Splitter::Splitter splitter(splitValues_left,splitValues_right);

    CPPUNIT_ASSERT( sampleIcs_left.size() == sampleIcs_right.size() );
    
    CPPUNIT_ASSERT( splitter.splitsLeft(0) );
    CPPUNIT_ASSERT( splitter.splitsLeft(1) );
    CPPUNIT_ASSERT( splitter.splitsLeft(2) );
    CPPUNIT_ASSERT( splitter.splitsLeft(3) );
    CPPUNIT_ASSERT( splitter.splitsLeft(4) );

    
    for(size_t i = 0; i < sampleIcs_left.size(); ++i ) {
      CPPUNIT_ASSERT( targetData[sampleIcs_left[i]] == 1 );
      CPPUNIT_ASSERT( targetData[sampleIcs_right[i]] == 0 );
    }
    
    CPPUNIT_ASSERT( fabs(splitFitness - 1) < datadefs::EPS );
  }

  for ( size_t i = 0; i < targetData.size() / 2 ; ++i ) {
    featureData[ 2 * i ] = static_cast<num_t>( i );
    featureData[ 2 * i + 1 ] = static_cast<num_t>( i );

    if ( i == 0 || i == 1 ) {
      targetData[ 2 * i ] = 1;
      targetData[ 2 * i + 1 ] = 2;
    } else {
      targetData[ 2 * i ] = 0;
      targetData[ 2 * i + 1 ] = 0;
    }
  }

  iter = 0;
  while ( iter < 2 ) {
    if ( iter == 0 ) {
      isTargetNumerical = false;
    } else {
      isTargetNumerical = true;
    }
 
    node.categoricalFeatureSplit(targetData,
				 isTargetNumerical,
				 featureData,
				 GI,
				 sampleIcs_left,
				 sampleIcs_right,
				 splitValues_left,
				 splitValues_right,
				 splitFitness);
    
    Splitter::Splitter splitter;
    CPPUNIT_ASSERT( splitter.splitterType_ == Splitter::NO_SPLITTER );

    splitter.reset(splitValues_left,splitValues_right);

    CPPUNIT_ASSERT( splitter.splitterType_ == Splitter::CATEGORICAL_SPLITTER );

    CPPUNIT_ASSERT( sampleIcs_left.size() == 4 );
    CPPUNIT_ASSERT( sampleIcs_right.size() == 6 );
    CPPUNIT_ASSERT( splitter.splitsLeft(0) );
    CPPUNIT_ASSERT( splitter.splitsLeft(1) );
    
    if ( iter == 0 ) {
      //datadefs::print<size_t>(sampleIcs_left);
      //datadefs::print<size_t>(sampleIcs_right);
      //cout << splitFitness << endl;
      CPPUNIT_ASSERT( fabs( splitFitness - 0.642857142857143 ) < 1e-10 );
    } else {
      CPPUNIT_ASSERT( fabs(splitFitness - 0.843750000000000 ) < 1e-10 );
    }
     
    splitter.reset();
    CPPUNIT_ASSERT( splitter.splitterType_ == Splitter::NO_SPLITTER );

    ++iter;
  }
}

void NodeTest::test_splitFitness() { 
}

void NodeTest::test_recursiveNDescendantNodes() {
  
}
// Registers the fixture into the test 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( NodeTest ); 

#endif // NODETEST_HPP
