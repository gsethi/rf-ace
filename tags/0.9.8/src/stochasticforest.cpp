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
  rootNodes_(nTrees),
  oobMatrix_(nTrees) {

  featuresInForest_.clear();

  learnedModel_ = NO_MODEL;

  /* PartitionSequence stores the optimal binary split strategy for data with nMaxCategories 
   * as a Gray Code sequence. It is assumed that the two disjoint partitions are interchangeable,
   * thus, the sequence has length of
   * 
   *  treeData->nMaxCategories() - 1
   *
   */
  if( treeData->nMaxCategories() >= 2 ) {
    partitionSequence_ = new PartitionSequence( treeData_->nMaxCategories() - 1 );
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

  if ( learnedModel_ == GBT_MODEL ) {
    toFile << "FOREST=GBT";
  } else if ( learnedModel_ == RF_MODEL ) {
    toFile << "FOREST=RF"; 
  }

  toFile << ",TARGET=" << targetName_ << endl;

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

  shrinkage_ = 1 / nTrees_;

  if(useContrasts) {
    treeData_->permuteContrasts();
  }

  learnedModel_ = RF_MODEL;

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

  if (numClasses > 0) {
    nTrees_ *= numClasses;
  }

  // This is required since the number of trees became multiplied by the number of classes
  oobMatrix_.resize( nTrees_ );
  rootNodes_.resize( nTrees_ );

  learnedModel_ = GBT_MODEL;
  
  bool sampleWithReplacement = false;
  num_t sampleSizeFraction = subSampleSize;
  size_t maxNodesToStop = 2 * nMaxLeaves - 1;
  size_t minNodeSizeToStop = 1;
  bool isRandomSplit = false;
  size_t nFeaturesForSplit = treeData_->nFeatures();
  bool useContrasts = false;

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

  // Target for GBT is different for each tree
  // reference to the target column, will overwrite it
  vector<num_t> curTargetData = trueTargetData; 

  // Set the initial prediction to zero.
  vector<num_t> prediction(nSamples, 0.0);

  size_t nNodes;

  for ( size_t treeIdx = 0; treeIdx < nTrees_; ++treeIdx ) {
    // current target is the negative gradient of the loss function
    // for square loss, it is target minus current prediction
    for (size_t i = 0; i < nSamples; i++ ) {
      curTargetData[i] = trueTargetData[i] - prediction[i];
    }

    treeData_->replaceFeatureData(targetIdx,curTargetData);

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
  // restore true target
  treeData_->replaceFeatureData(targetIdx,trueTargetData);
}

// Grow a GBT "forest" for a categorical target variable
void StochasticForest::growCategoricalGBT() {
  
  size_t targetIdx = treeData_->getFeatureIdx( targetName_ );
  size_t numClasses = treeData_->nCategories( targetIdx );

  //A function pointer to a function "gamma()" that is used to compute the node predictions with
  RootNode::LeafPredictionFunctionType leafPredictionFunctionType = RootNode::LEAF_GAMMA;

  // Save a copy of the target column because it will be overwritten later.
  // We also know that it must be categorical.
  size_t nSamples = treeData_->nSamples();
  vector<num_t> trueTargetData = treeData_->getFeatureData(targetIdx);
  vector<string> trueRawTargetData = treeData_->getRawFeatureData(targetIdx);
  
  // Target for GBT is different for each tree.
  // We use the original target column to save each temporary target.
  // Reference to the target column, will overwrite it:
  vector<num_t> curTargetData = trueTargetData;

  // For categorical target variables, we predict each category probability separately.
  // This target is numerical, thus we need to change the target variable type to
  // numerical from categorical.

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

      // For each tree the target data becomes the recently computed residuals
      treeData_->replaceFeatureData(targetIdx,curTargetData);

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
  // restore the true target
  treeData_->replaceFeatureData(targetIdx,trueRawTargetData);
}

void StochasticForest::transformLogistic(const size_t numClasses, vector<num_t>& prediction, vector<num_t>& probability) {

  // Multiclass logistic transform of class probabilities from current probability estimates.
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

void StochasticForest::predict(Treedata* treeData, vector<string>& categoryPrediction, vector<num_t>& confidence) {

  size_t targetIdx = treeData->getFeatureIdx( targetName_ );
  size_t nSamples = treeData->nSamples();
  size_t numClasses = treeData->nCategories( targetIdx );

  // For classification, each "tree" is actually numClasses_ trees in a row, each predicting the probability of its own class.
  size_t numIterations = nTrees_ / numClasses;

  // Vector storing the transformed probabilities for each class prediction
  vector<num_t> prediction( numClasses );

  // Vector storing true probabilities for each class prediction
  vector<num_t> probPrediction( numClasses );

  categoryPrediction.resize( nSamples );
  confidence.resize( nSamples );

  if ( learnedModel_ == GBT_MODEL ) {
    
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
	  prediction[k] = prediction[k] + shrinkage_ * rootNodes_[t]->percolateData(treeData,i)->getTrainPrediction(); //predictSampleByTree(i, t);
	  
	}
      }
      
      // ... find index of maximum prediction, this is the predicted category
      vector<num_t>::iterator maxProb = max_element( prediction.begin(), prediction.end() );
      num_t maxProbCategory = 1.0*(maxProb - prediction.begin());
      
      categoryPrediction[i] = treeData->getRawFeatureData(targetIdx,maxProbCategory); //backMapping[ maxProbCategory ]; // classes are 0,1,2,...
      
      StochasticForest::transformLogistic(numClasses, prediction, probPrediction); // predictions-to-probabilities
      
      vector<num_t>::iterator largestElementIt = max_element(probPrediction.begin(),probPrediction.end());
      confidence[i] = *largestElementIt;
    }

  } else if ( learnedModel_ == RF_MODEL ) {
    
    cerr << "StochasticForest::predict(string) -- implementation for predicting with RFs missing" << endl;
    exit(1);

  } else {

    cerr << "StochasticForest::predict(string) -- no model to predict with" << endl;
    exit(1);

  }

}

void StochasticForest::predict(Treedata* treeData, vector<num_t>& prediction, vector<num_t>& confidence) {
 
  size_t nSamples = treeData->nSamples();
  prediction.resize(nSamples);
  confidence.resize(nSamples);

  for ( size_t i = 0; i < nSamples; i++ ) {
    prediction[i] = 0;
    for ( size_t t = 0; t < nTrees_; ++t) {
      prediction[i] = prediction[i] + shrinkage_ * rootNodes_[t]->percolateData(treeData,i)->getTrainPrediction();
    }
    
  }
 
}

void StochasticForest::percolateSampleIcs(Node* rootNode, const vector<size_t>& sampleIcs, map<Node*,vector<size_t> >& trainIcs) {
  
  trainIcs.clear();
  
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

    //string featureNameNew = (*nodep)->splitterName();
    //size_t featureIdxNew = treeData_->getFeatureIdx(featureNameNew);
    size_t featureIdxNew = (*nodep)->splitterIdx();

    num_t value = treeData_->getFeatureData(featureIdxNew,sampleIdx);
    
    while ( datadefs::isNAN( value ) ) {
      treeData_->getRandomData(featureIdxNew,value);
    }
    
    Node* childNode;

    if ( treeData_->isFeatureNumerical(featureIdxNew) ) {
      childNode = (*nodep)->percolateData(value);
    } else {
      childNode = (*nodep)->percolateData(treeData_->getRawFeatureData(featureIdxNew,value));
    }
   
    if ( childNode == *nodep ) {
      break;
    }

    *nodep = childNode;

  }

}

void StochasticForest::percolateSampleIdxAtRandom(const size_t featureIdx, const size_t sampleIdx, Node** nodep) {
  
  while ( (*nodep)->hasChildren() ) {

    //string featureNameNew = (*nodep)->splitterName();
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
      childNode = (*nodep)->percolateData(treeData_->getRawFeatureData(featureIdxNew,value));
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
  vector<bool> hasImportance( nAllFeatures, false );
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

      importance[featureIdx] += nNewOobSamples * (permutedTreeImpurity - treeImpurity);
      hasImportance[featureIdx] = true;
    }
      
  }

  //cout << endl;

  //cout << nContrastsInForest << " contrasts in forest. Mean tree impurity " << meanTreeImpurity << endl;
  
  for ( size_t featureIdx = 0; featureIdx < nAllFeatures; ++featureIdx ) {
    
    if ( !hasImportance[featureIdx] || fabs(meanTreeImpurity) < datadefs::EPS ) {
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
