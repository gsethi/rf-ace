#include <cmath>
#include <ctime>
#include <iostream>
#include <iomanip>

#include "stochasticforest.hpp"
#include "datadefs.hpp"

StochasticForest::StochasticForest(Treedata* treeData, const string& targetName, const size_t nTrees):
  treeData_(treeData),
  targetName_(targetName),
  nTrees_(nTrees),
  rootNodes_(0),
  oobMatrix_(0) {

  featuresInForest_.clear();

  //treeData_ = treeData;
  //targetIdx_ = targetIdx;

  learnedModel_ = NO_MODEL;

  /* PartitionSequence stores the optimal binary split strategy for data with nMaxCategories 
   * as a Gray Code sequence. It is assumed that the two disjoint partitions are interchangeable,
   * thus, the sequence has length of
   * 
   *  treeData->nMaxCategories() - 1
   *
   */
  if( treeData->nMaxCategories() >= 2 ) {
    partitionSequence_ = new PartitionSequence( treeData->nMaxCategories() - 1 );
  } else {
    partitionSequence_ = new PartitionSequence( 1 );
  }
    
}

StochasticForest::~StochasticForest() {
  for(size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx) {
    delete rootNodes_[treeIdx];
  }

  delete partitionSequence_;

}

/* Prints the forest into a file, so that the forest can be loaded for later use (e.g. prediction).
 * This function still lacks most of the content. 
 */
void StochasticForest::printToFile(const string& fileName) {

  // Open stream for writing
  ofstream toFile( fileName.c_str() );

  // Save each tree in the forest
  for ( size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx ) {
    toFile << endl << "TREE " << treeIdx << endl;
    rootNodes_[treeIdx]->print(toFile);
  }
  
  // TODO: StcohasticForest::printToFile() must also print the forest parameters

  // Close stream
  toFile.close();
  
}

void StochasticForest::learnRF(const size_t mTry, 
			       const size_t nMaxLeaves,
			       const size_t nodeSize,
			       const bool useContrasts) {

  if(useContrasts) {
    treeData_->permuteContrasts();
  }

  //numClasses_ = treeData_->nCategories( targetName_ );
  learnedModel_ = RF_MODEL;
  featuresInForest_.clear();
  oobMatrix_.resize( nTrees_ );
  rootNodes_.resize( nTrees_ );

  //These parameters, and those specified in the Random Forest initiatialization, define the type of the forest generated (an RF) 
  bool sampleWithReplacement = true;
  num_t sampleSizeFraction = 1.0;
  size_t maxNodesToStop = 2 * nMaxLeaves - 1;
  size_t minNodeSizeToStop = nodeSize;
  bool isRandomSplit = true;
  size_t nFeaturesForSplit = mTry;
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  size_t numClasses = treeData_->nCategories( targetIdx );

  //Allocates memory for the root nodes
  for(size_t treeIdx = 0; treeIdx < nTrees_; ++treeIdx) {
    rootNodes_[treeIdx] = new RootNode(sampleWithReplacement,
                                       sampleSizeFraction,
                                       maxNodesToStop,
                                       minNodeSizeToStop,
                                       isRandomSplit,
                                       nFeaturesForSplit,
                                       useContrasts,
                                       numClasses,
				       partitionSequence_);

    size_t nNodes;

    //void (*leafPredictionFunction)(const vector<num_t>&, const size_t);
    RootNode::LeafPredictionFunctionType leafPredictionFunctionType;

    if(treeData_->isFeatureNumerical(targetIdx)) {
      leafPredictionFunctionType = RootNode::LEAF_MEAN;
    } else {
      leafPredictionFunctionType = RootNode::LEAF_MODE;
    }

    rootNodes_[treeIdx]->growTree(treeData_,
				  targetIdx,
				  leafPredictionFunctionType,
				  oobMatrix_[treeIdx],
				  featuresInForest_[treeIdx],
				  nNodes);
    
  }
}

void StochasticForest::learnGBT(const size_t nMaxLeaves, 
				const num_t shrinkage, 
				const num_t subSampleSize) {

  size_t targetIdx = treeData_->getFeatureIdx(targetName_);
  size_t numClasses = treeData_->nCategories(targetIdx);
  
  shrinkage_ = shrinkage;
  //size_t numClasses = treeData_->nCategories(targetIdx_);
  if (numClasses > 0) {
    nTrees_ *= numClasses;
  }

  learnedModel_ = GBT_MODEL;
  featuresInForest_.clear();
  oobMatrix_.resize(nTrees_);
  rootNodes_.resize(nTrees_);
  
  bool sampleWithReplacement = false;
  num_t sampleSizeFraction = subSampleSize;
  size_t maxNodesToStop = 2 * nMaxLeaves - 1;
  size_t minNodeSizeToStop = 1;
  bool isRandomSplit = false;
  size_t nFeaturesForSplit = treeData_->nFeatures();
  bool useContrasts = false;
  //size_t numClasses = treeData_->nCategories(targetName_);

  //Allocates memory for the root nodes. With all these parameters, the RootNode is now able to take full control of the splitting process
  rootNodes_.resize(nTrees_);
  for(size_t treeIdx = 0; treeIdx < nTrees_; ++treeIdx) {
    rootNodes_[treeIdx] = new RootNode(sampleWithReplacement,
                                       sampleSizeFraction,
                                       maxNodesToStop,
                                       minNodeSizeToStop,
                                       isRandomSplit,
                                       nFeaturesForSplit,
                                       useContrasts,
                                       numClasses,
				       partitionSequence_);
  }
    
  if(numClasses == 0) {
    StochasticForest::growNumericalGBT();
  } else {
    StochasticForest::growCategoricalGBT();
  }
}

// Grow a GBT "forest" for a numerical target variable
void StochasticForest::growNumericalGBT() {

  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  
  //A function pointer to a function "mean()" that is used to compute the node predictions with
  RootNode::LeafPredictionFunctionType leafPredictionFunctionType = RootNode::LEAF_MEAN;

  size_t nSamples = treeData_->nSamples();
  // save a copy of the target column because it will be overwritten
  vector<num_t> trueTargetData = treeData_->getFeatureData(targetIdx);
  //treeData_->getFeatureData(targetIdx_,trueTargetData);

  // Target for GBT is different for each tree
  // reference to the target column, will overwrite it
  vector<num_t>& curTargetData = treeData_->features_[targetIdx].data; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////

  // Set the initial prediction to zero.
  vector<num_t> prediction(nSamples, 0.0);
  //vector<num_t> curPrediction(nSamples);

  size_t nNodes;


  for ( size_t treeIdx = 0; treeIdx < nTrees_; ++treeIdx ) {
    // current target is the negative gradient of the loss function
    // for square loss, it is target minus current prediction
    for (size_t i = 0; i < nSamples; i++ ) {
      curTargetData[i] = trueTargetData[i] - prediction[i];
    }

    // Grow a tree to predict the current target
    rootNodes_[treeIdx]->growTree(treeData_,
				  targetIdx,
				  leafPredictionFunctionType,
				  oobMatrix_[treeIdx],
				  featuresInForest_[treeIdx],
				  nNodes);


    // What kind of a prediction does the new tree produce?
    vector<num_t> curPrediction = StochasticForest::predictDatasetByTree(treeIdx);

    // Calculate the current total prediction adding the newly generated tree
    num_t sqErrorSum = 0.0;
    for (size_t i=0; i<nSamples; i++) {
      prediction[i] = prediction[i] + shrinkage_ * curPrediction[i];

      // diagnostics
      num_t iError = trueTargetData[i]-prediction[i];
      sqErrorSum += iError*iError;

    }

  }
  // GBT-forest is now done!

  // restore true target column
  treeData_->features_[targetIdx].data = trueTargetData; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////
}

// Grow a GBT "forest" for a categorical target variable
void StochasticForest::growCategoricalGBT() {
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );

  size_t numClasses = treeData_->nCategories( targetIdx );

  //A function pointer to a function "gamma()" that is used to compute the node predictions with
  //void (*leafPredictionFunction)(const vector<num_t>&, const size_t) = Node::leafGamma;
  RootNode::LeafPredictionFunctionType leafPredictionFunctionType = RootNode::LEAF_GAMMA;

  // Save a copy of the target column because it will be overwritten later.
  // We also know that it must be categorical.
  size_t nSamples = treeData_->nSamples();
  vector<num_t> trueTargetData = treeData_->getFeatureData(targetIdx);

  // Target for GBT is different for each tree.
  // We use the original target column to save each temporary target.
  // Reference to the target column, will overwrite it:
  vector<num_t>& curTargetData = treeData_->features_[targetIdx].data; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////

  // For categorical target variables, we predict each category probability separately.
  // This target is numerical, thus we need to change the target variable type to
  // numerical from categorical.
  //size_t targetIdx = treeData_->name2idx_[featureName_]; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////
  treeData_->features_[targetIdx].isNumerical = true; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////


  // Initialize class probability estimates and the predictions.
  // Note that dimensions in these two are reversed!
  vector< vector<num_t> > prediction(    nSamples, vector<num_t>( numClasses, 0.0 ) );
  vector< vector<num_t> > curPrediction( numClasses, vector<num_t>( nSamples, 0.0 ) );

  vector< vector<num_t> > curProbability( nSamples, vector<num_t>( numClasses)  );

  // Each iteration consists of numClasses_ trees,
  // each of those predicting the probability residual for each class.
  size_t numIterations = nTrees_ / numClasses;

  for(size_t m=0; m < numIterations; ++m) {
    // Multiclass logistic transform of class probabilities from current probability estimates.
    for (size_t i=0; i<nSamples; i++) {
      StochasticForest::transformLogistic( numClasses, prediction[i],  curProbability[i]);
      // each prediction[i] is a vector<num_t>(numClasses_)
    }

    // construct a tree for each class
    for (size_t k = 0; k < numClasses; ++k) {
      // target for class k is ...
      for (size_t i=0; i<nSamples; i++) {
        // ... the difference between true target and current prediction
        curTargetData[i] = (k==trueTargetData[i]) - curProbability[i][k];
      }

      // Grow a tree to predict the current target
      size_t treeIdx = m * numClasses + k; // tree index
      size_t nNodes;
      rootNodes_[treeIdx]->growTree(treeData_,
				    targetIdx,
				    leafPredictionFunctionType,
				    oobMatrix_[treeIdx],
				    featuresInForest_[treeIdx],
				    nNodes);

      // What kind of a prediction does the new tree produce
      // out of the whole training data set?
      curPrediction[k] = StochasticForest::predictDatasetByTree(treeIdx);
      // Calculate the current total prediction adding the newly generated tree
      for (size_t i = 0; i < nSamples; i++) {
        prediction[i][k] = prediction[i][k] + shrinkage_ * curPrediction[k][i];
      }
    }
  }

  // GBT-forest is now done!
  // restore the true target column and its true type
  treeData_->features_[targetIdx].data = trueTargetData; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////
  treeData_->features_[targetIdx].isNumerical = false; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////
}

void StochasticForest::transformLogistic(const size_t numClasses, vector<num_t>& prediction, vector<num_t>& probability) {
  // Multiclass logistic transform of class probabilities from current probability estimates.
  //size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  //size_t numClasses = treeData_->nCategories( targetName_ );
  assert( numClasses == prediction.size() );
  vector<num_t>& expPrediction = probability; // just using the space by a different name

  // find maximum prediction
  vector<num_t>::iterator maxPrediction = max_element( prediction.begin(), prediction.end() );
  // scale by maximum to prevent numerical errors

  num_t expSum = 0.0;
  size_t k;
  for(k=0; k < numClasses; ++k) {
    expPrediction[k] = exp( prediction[k] - *maxPrediction ); // scale by maximum
    expSum += expPrediction[k];
  }
  for(k = 0; k < numClasses; ++k) {
    probability[k] = expPrediction[k] / expSum;
  }
}

// Use a single GBT tree to produce a prediction from a single data sample of an arbitrary data set.
num_t StochasticForest::predictSampleByTree(size_t sampleIdx, size_t treeIdx) {
  
  // Root of current tree
  Node* currentNode = rootNodes_[treeIdx];
  //Node* newNode = rootNodes_[treeIdx];
  
  StochasticForest::percolateSampleIdx(sampleIdx, &currentNode);
  
  return( currentNode->getTrainPrediction() );
}  

// Use a single GBT tree to produce predictions for an arbitrary data set.
vector<num_t> StochasticForest::predictDatasetByTree(size_t treeIdx) {
  
  size_t nSamples = treeData_->nSamples();

  vector<num_t> prediction(nSamples);

  // predict for all samples
  for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx) {
    prediction[sampleIdx] = StochasticForest::predictSampleByTree(sampleIdx, treeIdx);
    // cout << "Sample " << i << ", prediction " << curPrediction[i]  << endl;
  }

  return( prediction );
}

/*
  void StochasticForest::predict(vector<string>& prediction, vector<num_t>& confidence) {
  assert(numClasses_ != 0);
  StochasticForest::predict(prediction, confidence);
  }
  
  void StochasticForest::predict(vector<num_t>& prediction, vector<num_t>& confidence) {
  assert(numClasses_ == 0);
  StochasticForest::predict(prediction, confidence);
  }
*/

// Predict with the trained model and using an arbitrary data set. StochasticForest::numClasses_ determines
// whether the prediction is for a categorical or a numerical variable.
void StochasticForest::predict(vector<string>& prediction, vector<num_t>& confidence) {
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  size_t numClasses = treeData_->nCategories( targetIdx );

  assert( numClasses != 0 );
  
  //cerr << "StochasticForest::predict() -- prediction with novel data not yet working" << endl;
  //exit(1);

  switch ( learnedModel_ ) {
  case GBT_MODEL:
    StochasticForest::predictWithCategoricalGBT(prediction, confidence);
    break;
  case RF_MODEL:
    cerr << "Implementation of categorical prediction with RFs isn't yet working" << endl;
    assert(false);
    StochasticForest::predictWithCategoricalRF(prediction);
    break;
  case NO_MODEL:
    cerr << "Cannot predict -- no model trained" << endl;
    assert(false);
  }

}

// Predict with the trained model and using an arbitrary data set. StochasticForest::numClasses_ determines
// whether the prediction is for a categorical or a numerical variable.
void StochasticForest::predict(vector<num_t>& prediction, vector<num_t>& confidence) {
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  size_t numClasses = treeData_->nCategories( targetIdx );

  assert( numClasses == 0 );

  //cerr << "StochasticForest::predict() -- prediction with novel data not yet working" << endl;
  //exit(1);
  
  switch ( learnedModel_ ) {
  case GBT_MODEL:
    StochasticForest::predictWithNumericalGBT(prediction, confidence);
    break;
  case RF_MODEL:
    StochasticForest::predictWithNumericalRF(prediction);
    break;
  case NO_MODEL:
    cerr << "Cannot predict -- no model trained" << endl;
    assert(false);
  }

}

void StochasticForest::predictWithCategoricalRF(vector<string>& categoryPrediction) {

  cerr << "Prediction with RF isn't yet working" << endl;
  assert(false);

  categoryPrediction.resize( treeData_->nSamples() );
  

}

void StochasticForest::predictWithNumericalRF(vector<num_t>& prediction) {

  cerr << "Prediction with RF isn't yet working" << endl;
  assert(false);

  prediction.resize( treeData_->nSamples() );

  /*
    for(size_t sampleIdx = 0; sampleIdx < treeData_->nSamples(); ++sampleIdx) {
    
    prediction[sampleIdx] = 0.0;
    
    for(size_t treeIdx = 0; treeIdx < nTrees_; ++treeIdx) {
    
    Node* nodep(rootNodes_[treeIdx]);
    StochasticForest::percolateSampleIdx(sampleIdx, &nodep);
    prediction[sampleIdx] += nodep->getTrainPrediction();
    
    }
    
    prediction[sampleIdx] /= nTrees_;
    
    }
  */
  
}


// Predict categorical target using a GBT "forest" from an arbitrary data set. The function also outputs a confidence score for 
// the predictions. In this case the confidence score is the probability for the prediction.
void StochasticForest::predictWithCategoricalGBT(vector<string>& categoryPrediction, vector<num_t>& confidence) {
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );

  size_t nSamples = treeData_->nSamples();

  size_t numClasses = treeData_->nCategories( targetIdx );
  
  // For classification, each "tree" is actually numClasses_ trees in a row, each predicting the probability of its own class.
  size_t numIterations = nTrees_ / numClasses;

  // Vector storing the transformed probabilities for each class prediction
  vector<num_t> prediction( numClasses );

  // Vector storing true probabilities for each class prediction
  vector<num_t> probPrediction( numClasses );

  categoryPrediction.resize( nSamples );
  confidence.resize( nSamples );

  // For each sample we need to produce predictions for each class.
  for ( size_t i = 0; i < nSamples; i++ ) { 
    
    for (size_t k = 0; k < numClasses; ++k) { 
      
      // Initialize the prediction
      prediction[k] = 0.0;
      
      // We go through 
      for(size_t m = 0; m < numIterations; ++m) {
      
	// Tree index
	size_t t =  m * numClasses + k;

	// Shrinked shift towards the new prediction
        prediction[k] = prediction[k] + shrinkage_ * predictSampleByTree(i, t);
      
      }
    }

    // ... find index of maximum prediction, this is the predicted category
    vector<num_t>::iterator maxProb = max_element( prediction.begin(), prediction.end() );
    num_t maxProbCategory = 1.0*(maxProb - prediction.begin()); 
    
    //map<num_t,string> backMapping = treeData_->features_[targetIdx_].backMapping;
    categoryPrediction[i] = treeData_->getRawFeatureData(targetIdx,maxProbCategory); //backMapping[ maxProbCategory ]; // classes are 0,1,2,...
    
    StochasticForest::transformLogistic(numClasses, prediction, probPrediction); // predictions-to-probabilities
    
    vector<num_t>::iterator largestElementIt = max_element(probPrediction.begin(),probPrediction.end());
    confidence[i] = *largestElementIt;
  }
}

// Predict numerical target using a GBT "forest" from an arbitrary data set
void StochasticForest::predictWithNumericalGBT(vector<num_t>& prediction, vector<num_t>& confidence) {

  size_t nSamples = treeData_->nSamples();
  prediction.resize(nSamples);
  confidence.resize(nSamples);

  //cout << "Predicting "<<nSamples<<" samples. Target="<<targetIdx_<<endl;
  for (size_t i=0; i<nSamples; i++) {
    prediction[i] = 0;
    for(size_t t = 0; t < nTrees_; ++t) {
      prediction[i] = prediction[i] + shrinkage_ * predictSampleByTree(i, t);
    }
    // diagnostic print out the true and the prediction
    //cout << i << "\t" << treeData_->features_[targetIdx_].data[i] << "\t" << prediction[i] <<endl; //// THIS WILL BECOME INVALID UPON REMOVAL OF FRIENDSHIP ASSIGNMENT IN TREEDATA ////
	}
}


void StochasticForest::percolateSampleIcs(Node* rootNode, const vector<size_t>& sampleIcs, map<Node*,vector<size_t> >& trainIcs) {
  
  trainIcs.clear();
  //map<Node*,vector<size_t> > trainics;
  
  for(size_t i = 0; i < sampleIcs.size(); ++i) {
    //cout << " " << i << " / " << sampleIcs.size() << endl; 
    Node* nodep(rootNode);
    size_t sampleIdx = sampleIcs[i];
    StochasticForest::percolateSampleIdx(sampleIdx,&nodep);
    map<Node*,vector<size_t> >::iterator it(trainIcs.find(nodep));
    if(it == trainIcs.end()) {
      Node* foop(nodep);
      vector<size_t> foo(1);
      foo[0] = sampleIdx;
      trainIcs.insert(pair<Node*,vector<size_t> >(foop,foo));
    } else {
      trainIcs[it->first].push_back(sampleIdx);
    }
      
  }
  
  
  if(false) {
    cout << "Train samples percolated accordingly:" << endl;
    size_t iter = 0;
    for(map<Node*,vector<size_t> >::const_iterator it(trainIcs.begin()); it != trainIcs.end(); ++it, ++iter) {
      cout << "leaf node " << iter << ":"; 
      for(size_t i = 0; i < it->second.size(); ++i) {
        cout << " " << it->second[i];
      }
      cout << endl;
    }
  }
}

void StochasticForest::percolateSampleIcsAtRandom(const size_t featureIdx, Node* rootNode, const vector<size_t>& sampleIcs, map<Node*,vector<size_t> >& trainIcs) {

  trainIcs.clear();

  for(size_t i = 0; i < sampleIcs.size(); ++i) {
    Node* nodep(rootNode);
    size_t sampleIdx = sampleIcs[i];
    StochasticForest::percolateSampleIdxAtRandom(featureIdx,sampleIdx,&nodep);
    map<Node*,vector<size_t> >::iterator it(trainIcs.find(nodep));
    if(it == trainIcs.end()) {
      Node* foop(nodep);
      vector<size_t> foo(1);
      foo[0] = sampleIdx;
      trainIcs.insert(pair<Node*,vector<size_t> >(foop,foo));
    } else {
      trainIcs[it->first].push_back(sampleIdx);
    }

  }
}

void StochasticForest::percolateSampleIdx(const size_t sampleIdx, Node** nodep) {

  while ( (*nodep)->hasChildren() ) {
    
    size_t featureIdxNew = (*nodep)->splitterIdx();

    num_t value = treeData_->getFeatureData(featureIdxNew,sampleIdx);
    
    while ( datadefs::isNAN( value ) ) {
      treeData_->getRandomData(featureIdxNew,value);
    }
    
    Node* childNode;

    if ( treeData_->isFeatureNumerical(featureIdxNew) ) {
      childNode = (*nodep)->percolateData(value);
    } else {
      childNode = (*nodep)->percolateData(treeData_->dataToRaw(featureIdxNew,value));
    }
   
    if ( childNode == *nodep ) {
      break;
    }

    *nodep = childNode;

  }

}

void StochasticForest::percolateSampleIdxAtRandom(const size_t featureIdx, const size_t sampleIdx, Node** nodep) {
  
  while ( (*nodep)->hasChildren() ) {

    size_t featureIdxNew = (*nodep)->splitterIdx();

    num_t value = datadefs::NUM_NAN;

    if(featureIdx == featureIdxNew) {
      while(datadefs::isNAN(value)) {
        treeData_->getRandomData(featureIdxNew,value);
      }
    } else {
      value = treeData_->getFeatureData(featureIdxNew,sampleIdx);
      while ( datadefs::isNAN(value) ) {
	treeData_->getRandomData(featureIdxNew,value);
      }
    }

    Node* childNode;

    if ( treeData_->isFeatureNumerical(featureIdxNew) ) {
      childNode = (*nodep)->percolateData(value);
    } else {
      childNode = (*nodep)->percolateData(treeData_->dataToRaw(featureIdxNew,value));
    }

    if ( childNode == *nodep ) {
      break;
    }

    *nodep = childNode;

  }

}

// In growForest a bootstrapper was utilized to generate in-box (IB) and out-of-box (OOB) samples.
// IB samples were used to grow the forest, excluding OOB samples. In this function, these 
// previously excluded OOB samples are used for testing which features in the trained trees seem
// to contribute the most to the quality of the predictions. This is a three-fold process:
// 
// 0. for feature_i in features:
// 1. Take the original forest, percolate OOB samples across the trees all the way to the leaf nodes
//    and check how concordant the OOB and IB samples in the leafs, on average, are.
// 2. Same as with #1, but if feature_i is to make the split, sample a random value for feature_i
//    to make the split with.
// 3. Quantitate relative increase of disagreement between OOB and IB data on the leaves, with and 
//    without random sampling. Rationale: if feature_i is important, random sampling will have a 
//    big impact, thus, relative increase of disagreement will be high.  
vector<num_t> StochasticForest::featureImportance() {

  //cout << "StochasticForest::featureImportance()..." << endl;

  // The number of real features in the data matrix...
  size_t nRealFeatures = treeData_->nFeatures();

  // But as there is an equal amount of contrast features, the total feature count is double that.
  size_t nAllFeatures = 2 * nRealFeatures;

  //Each feature, either real or contrast, will have a slot into which the importance value will be put.
  vector<num_t> importance( nAllFeatures, 0.0 );
  size_t nOobSamples = 0; // !! Potentially Unintentional Humor: "Noob". That is actually intentional. :)
  
  size_t nContrastsInForest = 0;

  num_t meanTreeImpurity = 0.0;

  // The random forest object stores the mapping from trees to features it contains, which makes
  // the subsequent computations void of unnecessary looping
  for ( map<size_t, set<size_t> >::const_iterator tit(featuresInForest_.begin()); tit != featuresInForest_.end(); ++tit ) {
    size_t treeIdx = tit->first;
      
    //cout << "looping through tree " << treeIdx << " / " << featuresInForest_.size() << endl;

    size_t nNewOobSamples = oobMatrix_[treeIdx].size(); 
    nOobSamples += nNewOobSamples;

    map<Node*,vector<size_t> > trainIcs;
    StochasticForest::percolateSampleIcs(rootNodes_[treeIdx],oobMatrix_[treeIdx],trainIcs);
    
    //cout << "sample ics percolated" << endl;

    num_t treeImpurity;
    StochasticForest::treeImpurity(trainIcs,treeImpurity);

    // Accumulate tree impurity
    meanTreeImpurity += treeImpurity / nTrees_;

    //cout << " " << meanTreeImpurity;

    // Loop through all features in the tree
    for ( set<size_t>::const_iterator fit( tit->second.begin()); fit != tit->second.end(); ++fit ) {
      size_t featureIdx = *fit;
    
      if ( featureIdx >= nRealFeatures ) {
        ++nContrastsInForest;
      }

      StochasticForest::percolateSampleIcsAtRandom(featureIdx,rootNodes_[treeIdx],oobMatrix_[treeIdx],trainIcs);
      num_t permutedTreeImpurity;
      StochasticForest::treeImpurity(trainIcs,permutedTreeImpurity);
      //if ( fabs( treeImpurity ) > datadefs::EPS) {
      importance[featureIdx] += nNewOobSamples * (permutedTreeImpurity - treeImpurity);
      //}
      //cout << treeData_->getFeatureName(featureIdx) << " += " << importance[featureIdx] << endl;
    }
      
  }

  //cout << endl;

  //cout << nContrastsInForest << " contrasts in forest. Mean tree impurity " << meanTreeImpurity << endl;
  
  for ( size_t featureIdx = 0; featureIdx < nAllFeatures; ++featureIdx ) {
    
    if ( fabs( importance[featureIdx] ) < datadefs::EPS ) {
      importance[featureIdx] = datadefs::NUM_NAN;
    } else {
      importance[featureIdx] /= ( nOobSamples * meanTreeImpurity );
    }
    //cout << "I(" << treeData_->getFeatureName(featureIdx) << ") = " << importance[featureIdx] << endl;
  }

  return(importance);

}

size_t StochasticForest::nNodes() {
  size_t nNodes = 0;
  for(size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx) {
    nNodes += rootNodes_[treeIdx]->nNodes();
  }
  
  return(nNodes);
}

vector<size_t> StochasticForest::featureFrequency() {
  assert(false);
  vector<size_t> frequency(0);
  return(frequency);
}


void StochasticForest::treeImpurity(map<Node*,vector<size_t> >& trainIcs, 
				    num_t& impurity) {

  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );

  impurity = 0.0;
  size_t n_tot = 0;

  bool isTargetNumerical = treeData_->isFeatureNumerical(targetIdx);

  for(map<Node*,vector<size_t> >::iterator it(trainIcs.begin()); it != trainIcs.end(); ++it) {

    vector<num_t> targetData = treeData_->getFeatureData(targetIdx,it->second);
    num_t nodePrediction = it->first->getTrainPrediction();
    num_t nodeImpurity = 0;
    size_t nSamplesInNode = targetData.size();

    if(isTargetNumerical) {
      for(size_t i = 0; i < nSamplesInNode; ++i) {
        nodeImpurity += pow(nodePrediction - targetData[i],2);
      }

    } else {
      for(size_t i = 0; i < nSamplesInNode; ++i) {
        nodeImpurity += nodePrediction != targetData[i]; 
      }
    }

    n_tot += nSamplesInNode;
    impurity += nSamplesInNode * nodeImpurity;
  }

  if(n_tot > 0) {
    impurity /= n_tot;
  }
}
