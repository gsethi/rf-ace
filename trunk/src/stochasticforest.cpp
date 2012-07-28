#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>

#include "stochasticforest.hpp"
#include "datadefs.hpp"
#include "argparse.hpp"
#include "utils.hpp"
#include "math.hpp"

StochasticForest::StochasticForest(Treedata* treeData, options::General_options* parameters):
  treeData_(treeData),
  parameters_(parameters),
  rootNodes_(parameters_->nTrees) {

  size_t targetIdx = treeData_->getFeatureIdx(parameters_->targetStr);
  targetSupport_ = treeData_->categories(targetIdx);

  // Grows the forest
  if ( parameters_->modelType == options::RF ) {
    this->learnRF();
  } else if ( parameters_->modelType == options::GBT ) {
    cerr << "GBT isn't working at the moment!" << endl;
    exit(1);
    this->learnGBT();
  } else if ( parameters_->modelType == options::CART ) {
    this->learnRF();
  } else {
    cerr << "Unknown model to be learned!" << endl;
    exit(1);
  }

  // Get features in the forest for fast look-up
  for ( size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx ) {
    set<size_t> featuresInTree = rootNodes_[treeIdx]->getFeaturesInTree();
    for ( set<size_t>::const_iterator it(featuresInTree.begin()); it != featuresInTree.end(); ++it ) {
      featuresInForest_.insert(*it);
    }
  }

}

StochasticForest::StochasticForest(options::General_options* parameters):
  parameters_(parameters) {

  ifstream forestStream( parameters_->forestInput.c_str() );
  assert(forestStream.good());
  
  string newLine("");
  getline(forestStream,newLine);

  map<string,string> forestSetup = utils::parse(newLine,',','=','"');

  if ( forestSetup["FOREST"] == "GBT" ) {    
    parameters_->modelType = options::GBT;
  } else if ( forestSetup["FOREST"] == "RF" ) {
    parameters_->modelType = options::RF;
  } else if ( forestSetup["FOREST"] == "CART" ) {
    parameters_->modelType = options::CART;
  } else {
    cerr << "Unknown forest type: " << forestSetup["FOREST"] << endl;
    exit(1);
  }

  assert( forestSetup.find("NTREES") != forestSetup.end() );
  parameters_->nTrees = static_cast<size_t>( utils::str2<int>(forestSetup["NTREES"]) );
  rootNodes_.resize(parameters_->nTrees);

  assert( forestSetup.find("TARGET") != forestSetup.end() );
  parameters_->targetStr = forestSetup["TARGET"];

  //size_t targetIdx = treeData_->getFeatureIdx(parameters_->targetStr);

  assert( forestSetup.find("CATEGORIES") != forestSetup.end() );
  targetSupport_ = utils::split(forestSetup["CATEGORIES"],',');

  bool isTargetNumerical = targetSupport_.size() == 0;

  assert( forestSetup.find("SHRINKAGE") != forestSetup.end() );
  parameters_->shrinkage = utils::str2<num_t>(forestSetup["SHRINKAGE"]);
  
  assert( forestStream.good() );
 
  int treeIdx = -1;

  vector<map<string,Node*> > forestMap( parameters_->nTrees );

  // Read the forest
  while ( getline(forestStream,newLine) ) {

    // remove trailing end-of-line characters
    newLine = utils::chomp(newLine);

    // Empty lines will be discarded
    if ( newLine == "" ) {
      continue;
    }

    if ( newLine.compare(0,5,"TREE=") == 0 ) { 
      ++treeIdx;
      //cout << treeIdx << " vs " << newLine << " => " << utils::str2int(utils::split(newLine,'=')[1]) << endl;
      assert( treeIdx == utils::str2<int>(utils::split(newLine,'=')[1]) );
      continue;
    }
        
    //cout << newLine << endl;

    map<string,string> nodeMap = utils::parse(newLine,',','=','"');

    // If the node is rootnode, we will create a new RootNode object and assign a reference to it in forestMap
    if ( nodeMap["NODE"] == "*" ) {
      size_t threadIdx = 0;
      rootNodes_[treeIdx] = new RootNode(NULL,parameters_,threadIdx);
      forestMap[treeIdx]["*"] = rootNodes_[treeIdx];
    }
      
    Node* nodep = forestMap[treeIdx][nodeMap["NODE"]];

    string rawTrainPrediction = nodeMap["PRED"];
    num_t trainPrediction = datadefs::NUM_NAN;

    if ( isTargetNumerical ) {
      trainPrediction = utils::str2<num_t>(rawTrainPrediction);
    }
    
    // Set train prediction of the node
    nodep->setTrainPrediction(trainPrediction, rawTrainPrediction);
    
    // If the node has a splitter...
    if ( nodeMap.find("SPLITTER") != nodeMap.end() ) {
      
      //size_t featureIdx = treeData_->getFeatureIdx(nodeMap["SPLITTER"]);

      // If the splitter is numerical...
      if ( nodeMap["SPLITTERTYPE"] == "NUMERICAL" ) {
       
	nodep->setSplitter(1,
			   nodeMap["SPLITTER"],
			   utils::str2<num_t>(nodeMap["LFRACTION"]),
			   utils::str2<num_t>(nodeMap["LVALUES"]));
	
      } else {

	set<string> splitLeftValues = utils::keys(nodeMap["LVALUES"],':');
	set<string> splitRightValues = utils::keys(nodeMap["RVALUES"],':');

	nodep->setSplitter(1,
			   nodeMap["SPLITTER"],
			   utils::str2<num_t>(nodeMap["LFRACTION"]),
			   splitLeftValues,
			   splitRightValues);
      
      }

      forestMap[treeIdx][nodeMap["NODE"]+"L"] = nodep->leftChild();
      forestMap[treeIdx][nodeMap["NODE"]+"R"] = nodep->rightChild();

    }
    
  }
  
}

StochasticForest::~StochasticForest() {

  for ( size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx ) {
    delete rootNodes_[treeIdx];
  }
  
}

/* Prints the forest into a file, so that the forest can be loaded for later use (e.g. prediction).
 */
void StochasticForest::printToFile(const string& fileName) {

  // Open stream for writing
  ofstream toFile( fileName.c_str() );

  if ( parameters_->modelType == options::GBT ) {
    toFile << "FOREST=GBT";
  } else if ( parameters_->modelType == options::RF ) {
    toFile << "FOREST=RF"; 
  } else if ( parameters_->modelType == options::CART ) {
    toFile << "FOREST=CART";
  }

  size_t targetIdx = treeData_->getFeatureIdx( parameters_->targetStr );
  
  vector<string> categories = treeData_->categories(targetIdx);

  string categoriesStr = utils::join(categories.begin(),categories.end(),',');

  toFile << ",TARGET=" << "\"" << parameters_->targetStr << "\"";
  toFile << ",NTREES=" << parameters_->nTrees;
  toFile << ",CATEGORIES=" << "\"" << categoriesStr << "\"";
  toFile << ",SHRINKAGE=" << parameters_->shrinkage << endl;

  // Save each tree in the forest
  for ( size_t treeIdx = 0; treeIdx < rootNodes_.size(); ++treeIdx ) {
    toFile << "TREE=" << treeIdx << endl;
    rootNodes_[treeIdx]->print(toFile);
  }
  
  // Close stream
  toFile.close();
  
}

/*
  Node::GrowInstructions StochasticForest::getGrowInstructions() {
  
  Node::GrowInstructions GI;
  
  size_t targetIdx = treeData_->getFeatureIdx( parameters_->targetStr );
  
  RootNode::PredictionFunctionType predictionFunctionType;
  
  if(treeData_->isFeatureNumerical(targetIdx)) {
  predictionFunctionType = RootNode::MEAN;
  } else {
  predictionFunctionType = RootNode::MODE;
  }
  
  GI.sampleWithReplacement = parameters_->sampleWithReplacement;
  GI.sampleSizeFraction = parameters_->inBoxFraction;
  GI.maxNodesToStop = 2 * parameters_->nMaxLeaves - 1;
  GI.minNodeSizeToStop = parameters_->nodeSize;
  GI.isRandomSplit = parameters_->isRandomSplit;
  
  GI.featureIcs = utils::range( treeData_->nFeatures() );
  GI.featureIcs.erase( GI.featureIcs.begin() + targetIdx ); // Remove target from the feature index list
  
  if ( GI.isRandomSplit ) {
  GI.nFeaturesForSplit = parameters_->mTry;
  } else {
  GI.nFeaturesForSplit = GI.featureIcs.size();
  }
  
  GI.useContrasts = parameters_->useContrasts;
  GI.numClasses = targetSupport_.size();
  GI.predictionFunctionType = predictionFunctionType;
  
  GI.randIntGens = &parameters_->randIntGens;
  
  return( GI );
  
  }
*/

void growTreesPerThread(vector<RootNode*>& rootNodes) {
   
  for ( size_t i = 0; i < rootNodes.size(); ++i ) {
    rootNodes[i]->growTree();
  }

}


void StochasticForest::learnRF() {

  // We force the shrinkage to equal to one over the number of trees
  //parameters_->shrinkage = 1.0 / parameters_->nTrees;

  // If we use contrasts, we need to permute them before using
  if(parameters_->useContrasts) {
    treeData_->permuteContrasts(parameters_->randIntGens[0]);
  }

  if ( parameters_->isRandomSplit && parameters_->mTry == 0 ) {
    cerr << "StochasticForest::learnRF() -- for randomized splits mTry must be greater than 0" << endl;
    exit(1);
  }

  vector<vector<size_t> > treeIcs = utils::splitRange(parameters_->nTrees,parameters_->nThreads);
  
  assert( parameters_->nThreads > 0);
  assert( parameters_->nTrees > 0);
  assert( rootNodes_.size() == parameters_->nTrees );

  vector<thread> threads;

  for ( size_t threadIdx = 0; threadIdx < parameters_->nThreads; ++threadIdx ) {
    
    vector<size_t>& treeIcsPerThread = treeIcs[threadIdx];
    vector<RootNode*> rootNodesPerThread(treeIcsPerThread.size());
    
    for ( size_t i = 0; i < treeIcsPerThread.size(); ++i ) {

      rootNodes_[treeIcsPerThread[i]] = new RootNode(treeData_,
						     parameters_,
						     threadIdx);
      
      rootNodesPerThread[i] = rootNodes_[treeIcsPerThread[i]];
      
      //rootNodes_[treeIcsPerThread[i]]->growTree();
    }
    
    threads.push_back( thread(growTreesPerThread,rootNodesPerThread) ); //thread(this->growTrees,treeIcsPerThread[threadIdx],threadIdx);
  }
  
  for ( size_t threadIdx = 0; threadIdx < threads.size(); ++threadIdx ) {
    threads[threadIdx].join();
  }
  
}

void StochasticForest::learnGBT() {
 

  //size_t targetIdx = treeData_->getFeatureIdx( parameters_->targetStr );
  size_t nCategories = targetSupport_.size();

  if (nCategories > 0) {
    parameters_->nTrees *= nCategories;
  }

  // This is required since the number of trees became multiplied by the number of classes
  rootNodes_.resize( parameters_->nTrees );

  // No multithreading at the moment
  size_t threadIdx = 0;

  //Allocates memory for the root nodes. With all these parameters, the RootNode is now able to take full control of the splitting process
  rootNodes_.resize( parameters_->nTrees );
  for(size_t treeIdx = 0; treeIdx < parameters_->nTrees; ++treeIdx) {
    rootNodes_[treeIdx] = new RootNode(treeData_,
				       parameters_,
				       threadIdx);
  }
    
  if ( targetSupport_.size() == 0 ) {
    this->growNumericalGBT();
  } else {
    this->growCategoricalGBT();
  }

}


// Grow a GBT "forest" for a numerical target variable
void StochasticForest::growNumericalGBT() {

  size_t targetIdx = treeData_->getFeatureIdx( parameters_->targetStr );
  
  //A function pointer to a function "mean()" that is used to compute the node predictions with
  //RootNode::PredictionFunctionType predictionFunctionType = RootNode::MEAN;

  size_t nSamples = treeData_->nSamples();
  // save a copy of the target column because it will be overwritten
  vector<num_t> trueTargetData = treeData_->getFeatureData(targetIdx);

  // Target for GBT is different for each tree
  // reference to the target column, will overwrite it
  vector<num_t> curTargetData = trueTargetData; 

  // Set the initial prediction to zero.
  vector<num_t> prediction(nSamples, 0.0);

  //size_t nNodes;

  for ( size_t treeIdx = 0; treeIdx < parameters_->nTrees; ++treeIdx ) {
    // current target is the negative gradient of the loss function
    // for square loss, it is target minus current prediction
    for (size_t i = 0; i < nSamples; i++ ) {
      curTargetData[i] = trueTargetData[i] - prediction[i];
    }

    treeData_->replaceFeatureData(targetIdx,curTargetData);

    // Grow a tree to predict the current target
    rootNodes_[treeIdx]->growTree();

    // What kind of a prediction does the new tree produce?
    vector<num_t> curPrediction = rootNodes_[treeIdx]->getTrainPrediction(); //StochasticForest::predictDatasetByTree(treeIdx);

    // Calculate the current total prediction adding the newly generated tree
    num_t sqErrorSum = 0.0;
    for (size_t i=0; i<nSamples; i++) {
      prediction[i] = prediction[i] + parameters_->shrinkage * curPrediction[i];

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
  
  size_t targetIdx = treeData_->getFeatureIdx( parameters_->targetStr );
  size_t nCategories = treeData_->nCategories( targetIdx );

  //A function pointer to a function "gamma()" that is used to compute the node predictions with
  //RootNode::PredictionFunctionType predictionFunctionType = RootNode::GAMMA;

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
  vector< vector<num_t> > prediction(    nSamples, vector<num_t>( nCategories, 0.0 ) );
  vector< vector<num_t> > curPrediction( nCategories, vector<num_t>( nSamples, 0.0 ) );

  vector< vector<num_t> > curProbability( nSamples, vector<num_t>( nCategories ) );

  // Each iteration consists of numClasses_ trees,
  // each of those predicting the probability residual for each class.
  size_t numIterations = parameters_->nTrees / nCategories;

  for(size_t m=0; m < numIterations; ++m) {
    // Multiclass logistic transform of class probabilities from current probability estimates.
    for (size_t i=0; i<nSamples; i++) {
      StochasticForest::transformLogistic( prediction[i],  curProbability[i]);
      // each prediction[i] is a vector<num_t>(numClasses_)
    }

    // construct a tree for each class
    for (size_t k = 0; k < nCategories; ++k) {
      // target for class k is ...
      for (size_t i=0; i<nSamples; i++) {
        // ... the difference between true target and current prediction
        curTargetData[i] = (k==trueTargetData[i]) - curProbability[i][k];
      }

      // For each tree the target data becomes the recently computed residuals
      treeData_->replaceFeatureData(targetIdx,curTargetData);

      // Grow a tree to predict the current target
      size_t treeIdx = m * nCategories + k; // tree index
      //size_t nNodes;
      rootNodes_[treeIdx]->growTree();

      //cout << "Tree " << treeIdx << " ready, predicting OOB samples..." << endl;

      // What kind of a prediction does the new tree produce
      // out of the whole training data set?
      curPrediction[k] = rootNodes_[treeIdx]->getTrainPrediction(); //StochasticForest::predictDatasetByTree(treeIdx);
      // Calculate the current total prediction adding the newly generated tree
      for (size_t i = 0; i < nSamples; i++) {
        prediction[i][k] = prediction[i][k] + parameters_->shrinkage * curPrediction[k][i];
      }
    }
  }
  
  // GBT-forest is now done!
  // restore the true target
  treeData_->replaceFeatureData(targetIdx,trueRawTargetData);
}

void StochasticForest::transformLogistic(vector<num_t>& prediction, vector<num_t>& probability) {

  size_t nCategories = targetSupport_.size();

  // Multiclass logistic transform of class probabilities from current probability estimates.
  assert( nCategories == prediction.size() );
  vector<num_t>& expPrediction = probability; // just using the space by a different name

  // find maximum prediction
  vector<num_t>::iterator maxPrediction = max_element( prediction.begin(), prediction.end() );
  // scale by maximum to prevent numerical errors

  num_t expSum = 0.0;
  size_t k;
  for(k=0; k < nCategories; ++k) {
    expPrediction[k] = exp( prediction[k] - *maxPrediction ); // scale by maximum
    expSum += expPrediction[k];
  }
  for(k = 0; k < nCategories; ++k) {
    probability[k] = expPrediction[k] / expSum;
  }
}

num_t StochasticForest::error(const vector<num_t>& data1, 
			      const vector<num_t>& data2) {

  size_t nSamples = data1.size();
  assert( nSamples == data2.size() );

  num_t predictionError = 0.0;
  size_t nRealSamples = 0;

  if ( this->isTargetNumerical() ) {
    for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
      if ( !datadefs::isNAN(data1[sampleIdx]) && !datadefs::isNAN(data2[sampleIdx]) ) {
        ++nRealSamples;
        predictionError += pow( data1[sampleIdx] - data2[sampleIdx] ,2);
      }
    }
  } else {
    for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
      if ( !datadefs::isNAN(data1[sampleIdx]) && !datadefs::isNAN(data2[sampleIdx]) ) {
        ++nRealSamples;
        predictionError += 1.0 * ( data1[sampleIdx] != data2[sampleIdx] );
      }
    }
  }

  if ( nRealSamples > 0 ) {
    predictionError /= nRealSamples;
  } else {
    predictionError = datadefs::NUM_NAN;
  }

  return( predictionError );

}


num_t StochasticForest::getOobError() {

  vector<num_t> oobPredictions = this->getOobPredictions();
  
  size_t targetIdx = treeData_->getFeatureIdx(parameters_->targetStr);
  vector<num_t> trueData = treeData_->getFeatureData(targetIdx);

  return( this->error(oobPredictions,trueData) );
  
}

num_t StochasticForest::getError() {

  vector<num_t> predictions = this->getPredictions();
  //this->predictWithTestData(treeData_,predictions,confidence)


  size_t targetIdx = treeData_->getFeatureIdx(parameters_->targetStr);
  vector<num_t> trueData = treeData_->getFeatureData(targetIdx);

  return( this->error(predictions,trueData) );
  
}

void StochasticForest::getImportanceValues(vector<num_t>& importanceValues, vector<num_t>& contrastImportanceValues) {
  
  if ( featuresInForest_.size() == 0 ) {
    cout << "NOTE: forest is empty!" << endl;
  }
  
  vector<num_t> oobPredictions = this->getOobPredictions();
  
  size_t nRealFeatures = treeData_->nFeatures();
  size_t nAllFeatures = 2*nRealFeatures;

  size_t targetIdx = treeData_->getFeatureIdx(parameters_->targetStr);

  vector<num_t> trueData = treeData_->getFeatureData(targetIdx);

  importanceValues.clear();
  importanceValues.resize(nAllFeatures,0.0);

  num_t oobError = this->error(oobPredictions,trueData);

  for ( set<size_t>::const_iterator it(featuresInForest_.begin()); it != featuresInForest_.end(); ++it ) {

    size_t featureIdx = *it;

    vector<num_t> permutedOobPredictions = this->getPermutedOobPredictions(featureIdx);

    num_t permutedOobError = this->error(permutedOobPredictions,trueData);

    importanceValues[featureIdx] = permutedOobError - oobError;

    if ( parameters_->normalizeImportanceValues ) {
      if ( oobError < datadefs::EPS ) {
	importanceValues[featureIdx] = datadefs::NUM_INF;
      } else {
	importanceValues[featureIdx] /= oobError;
      }
    }

  }

  assert( !datadefs::containsNAN(importanceValues) );


  contrastImportanceValues.resize(nRealFeatures);

  copy(importanceValues.begin() + nRealFeatures,
       importanceValues.end(),
       contrastImportanceValues.begin());

  importanceValues.resize(nRealFeatures);

}

void predictCatPerThread(Treedata* testData, vector<RootNode*>& rootNodes, vector<size_t>& sampleIcs, vector<string>* predictions, vector<num_t>* confidence) {
  size_t nTrees = rootNodes.size();
  for ( size_t i = 0; i < sampleIcs.size(); ++i ) {
    size_t sampleIdx = sampleIcs[i];
    vector<string> predictionVec(nTrees);
    for ( size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx ) {
      predictionVec[treeIdx] = rootNodes[treeIdx]->getRawTestPrediction(testData,sampleIdx);
    }
    (*predictions)[sampleIdx] = math::mode(predictionVec);
    (*confidence)[sampleIdx] = 1.0 * math::nMismatches(predictionVec,(*predictions)[sampleIdx]) / nTrees;
  }
}


void predictNumPerThread(Treedata* testData, vector<RootNode*>& rootNodes, vector<size_t>& sampleIcs, vector<num_t>* predictions, vector<num_t>* confidence) {
  size_t nTrees = rootNodes.size();
  for ( size_t i = 0; i < sampleIcs.size(); ++i ) {
    size_t sampleIdx = sampleIcs[i];
    vector<num_t> predictionVec(nTrees);
    for ( size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx ) {
      predictionVec[treeIdx] = rootNodes[treeIdx]->getTestPrediction(testData,sampleIdx);
    }
    (*predictions)[sampleIdx] = math::mean(predictionVec);
    (*confidence)[sampleIdx] = sqrt(math::var(predictionVec,(*predictions)[sampleIdx]));
  }
}


void StochasticForest::predict(Treedata* testData, vector<string>& predictions, vector<num_t>& confidence) {

  assert( parameters_->modelType == options::RF );
  assert( !this->isTargetNumerical() );

  size_t nSamples = testData->nSamples();

  predictions.resize(nSamples);
  confidence.resize(nSamples);

  vector<vector<size_t> > sampleIcs = utils::splitRange(nSamples,parameters_->nThreads);

  vector<thread> threads;

  for ( size_t threadIdx = 0; threadIdx < parameters_->nThreads; ++threadIdx ) {
    threads.push_back( thread(predictCatPerThread,testData,rootNodes_,sampleIcs[threadIdx],&predictions,&confidence) );
  }

  for ( size_t threadIdx = 0; threadIdx < threads.size(); ++threadIdx ) {
    threads[threadIdx].join();
  }

}


void StochasticForest::predict(Treedata* testData, vector<num_t>& predictions, vector<num_t>& confidence) {

  assert( parameters_->modelType == options::RF );
  assert( this->isTargetNumerical() );

  size_t nSamples = testData->nSamples();

  predictions.resize(nSamples);
  confidence.resize(nSamples);

  vector<vector<size_t> > sampleIcs = utils::splitRange(nSamples,parameters_->nThreads);

  vector<thread> threads;

  for ( size_t threadIdx = 0; threadIdx < parameters_->nThreads; ++threadIdx ) {
    threads.push_back( thread(predictNumPerThread,testData,rootNodes_,sampleIcs[threadIdx],&predictions,&confidence) );
  }

  for ( size_t threadIdx = 0; threadIdx < threads.size(); ++threadIdx ) {
    threads[threadIdx].join();
  }

}



vector<num_t> StochasticForest::getPredictions() {
  
  size_t nSamples = treeData_->nSamples();
  size_t nTrees = this->nTrees();
  vector<num_t> predictions(nSamples);
  for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
    vector<num_t> predictionVec(nTrees);
    for ( size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx ) {
      predictionVec[treeIdx] = rootNodes_[treeIdx]->getTrainPrediction(sampleIdx);
    }
    predictions[sampleIdx] = this->isTargetNumerical() ? math::mean(predictionVec) : math::mode(predictionVec);
  }
  
  return( predictions );

}

vector<num_t> StochasticForest::getOobPredictions() {
  
  size_t nSamples = treeData_->nSamples();
  size_t nTrees = this->nTrees();
  vector<vector<num_t> > predictionMatrix(nSamples);
  vector<num_t> predictions(nSamples);
  for ( size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx ) {
    vector<size_t> oobIcs = rootNodes_[treeIdx]->getOobIcs();
    for ( vector<size_t>::const_iterator it( oobIcs.begin() ); it != oobIcs.end(); ++it ) {
      size_t sampleIdx = *it;
      predictionMatrix[sampleIdx].push_back( rootNodes_[treeIdx]->getTrainPrediction(sampleIdx) );
    }
  }
  
  for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
    predictions[sampleIdx] = this->isTargetNumerical() ? math::mean(predictionMatrix[sampleIdx]) : math::mode(predictionMatrix[sampleIdx]);
  }

  return( predictions );

}

vector<num_t> StochasticForest::getPermutedOobPredictions(const size_t featureIdx) {

  size_t nSamples = treeData_->nSamples();
  size_t nTrees = this->nTrees();
  vector<vector<num_t> > predictionMatrix(nSamples);
  vector<num_t> predictions(nSamples);
  for ( size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx ) {
    vector<size_t> oobIcs = rootNodes_[treeIdx]->getOobIcs();
    for ( vector<size_t>::const_iterator it( oobIcs.begin() ); it != oobIcs.end(); ++it ) {
      size_t sampleIdx = *it;
      predictionMatrix[sampleIdx].push_back( rootNodes_[treeIdx]->getPermutedTrainPrediction(featureIdx,sampleIdx) );
    }
  }

  for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
    predictions[sampleIdx] = this->isTargetNumerical() ? math::mean(predictionMatrix[sampleIdx]) : math::mode(predictionMatrix[sampleIdx]);
  }

  return( predictions );
  
}

/**
   Returns a vector of node counts in the trees of the forest
*/
vector<size_t> StochasticForest::nNodes() {
  
  // Get the number of trees
  size_t nTrees = this->nTrees();

  // Initialize the node count vector
  vector<size_t> nNodes(nTrees) ;

  // Loop through all trees
  for(size_t treeIdx = 0; treeIdx < nTrees; ++treeIdx) {

    // Get the node count for the tree
    nNodes[treeIdx] = this->nNodes(treeIdx);

  }
  
  return( nNodes );
}

/**
   Returns the number of nodes in tree treeIdx
*/
size_t StochasticForest::nNodes(const size_t treeIdx) {
  return( rootNodes_[treeIdx]->nNodes() );
}

/*
  map<size_t,map<size_t,size_t> > StochasticForest::featureFrequency() {

  // The output variable that stores all the frequencies 
  // of the features in the trees
  map<size_t,map<size_t,size_t> > frequency;

  // We loop through all the trees
  for ( size_t treeIdx = 0; treeIdx < parameters_->nTrees; ++treeIdx ) {
  
  set<size_t> featuresInTree = rootNodes_[treeIdx]->getFeaturesInTree();
  
    for ( set<size_t>::const_iterator it1(featuresInTree.begin() ); it1 != featuresInTree.end(); ++it1 ) {
      set<size_t>::const_iterator it2( it1 );
      ++it2;
      while ( it2 != featuresInTree.end() ) {
	
	size_t lokey,hikey;
	if ( *it1 <= *it2 ) { 
	  lokey = *it1;
	  hikey = *it2;
	} else {
	  lokey = *it2;
	  hikey = *it1;
	}

	if ( frequency.find(lokey) == frequency.end() ) {
	  frequency[lokey][hikey] = 1;
	} else {
	  ++frequency[lokey][hikey];
	}

	++it2;

      }
      }

  }
  

  return(frequency);
  }
*/

/**
   Returns the number of trees in the forest
*/
size_t StochasticForest::nTrees() {
  return( rootNodes_.size() );
}

