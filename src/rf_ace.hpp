#ifndef RF_ACE_HPP
#define RF_ACE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <iomanip>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>
#include <cstdlib>
#include <string>
#include <vector>

#include "stochasticforest.hpp"
#include "treedata.hpp"
#include "options.hpp"
#include "utils.hpp"
#include "math.hpp"
#include "datadefs.hpp"
#include "progress.hpp"
#include "distributions.hpp"

using namespace std;
using datadefs::num_t;
using datadefs::forest_t;
using datadefs::ftable_t;

using namespace std;

class RFACE { 

public:

  RFACE(size_t nThreads = 1, int seed = -1):
    trainedModel_(NULL) {
    this->resetRandomNumberGenerators(nThreads,seed);
  }

  ~RFACE() {

    if ( trainedModel_ ) {
      delete trainedModel_;
      trainedModel_ = NULL;
    }
    
  }

  struct FilterOutput {
    size_t nAllFeatures;
    size_t nSignificantFeatures;
    string targetName;
    vector<string> featureNames;
    vector<num_t> pValues;
    vector<num_t> importances;
    vector<num_t> correlations;
    vector<num_t> sampleCounts;
  };

  struct TestOutput {
    string targetName;
    bool isTargetNumerical;
    vector<num_t> numPredictions;
    vector<num_t> numTrueData;
    vector<string> catPredictions;
    vector<string> catTrueData;
    vector<num_t> confidence;
    vector<string> sampleNames;
  };

  struct QuantilePredictionOutput {
    string targetName;
    vector<num_t> quantiles;
    vector<vector<num_t> > predictions;
    vector<num_t> trueData;
    vector<string> sampleNames;
  };
  
  void train(Treedata* trainData, 
	     const size_t targetIdx, 
	     const vector<num_t>& featureWeights, 
	     ForestOptions* forestOptions) {
    
    forestOptions->useContrasts = false;

    forestOptions->validate();

    assert( !forestOptions->useContrasts );

    if ( trainedModel_ ) {
      delete trainedModel_;
      trainedModel_ = NULL;
    }
    
    trainedModel_ = new StochasticForest();

    if ( forestOptions->forestType == forest_t::RF || forestOptions->forestType == forest_t::QRF ) {
      trainedModel_->learnRF(trainData,targetIdx,forestOptions,featureWeights,randoms_);
    } else if ( forestOptions->forestType == forest_t::GBT ) {
      trainedModel_->learnGBT(trainData,targetIdx,forestOptions,featureWeights,randoms_);
    } else {
      cerr << "Unknown forest type!" << endl;
      exit(1);
    }
  }

  FilterOutput filter(Treedata* filterData, 
		      const size_t targetIdx, 
		      const vector<num_t>& featureWeights, 
		      ForestOptions* forestOptions,
		      FilterOptions* filterOptions,
		      const string& forestFile = "" ) {

    forestOptions->useContrasts = true;

    forestOptions->validate();
    filterOptions->validate();

    assert( forestOptions->useContrasts );

    if ( filterData->nSamples() < 2 * forestOptions->nodeSize ) {
      cerr << "Not enough samples (" << filterData->nSamples() << ") to perform a single split" << endl;
      exit(1);
    }

    FilterOutput filterOutput;

    vector<num_t> contrastImportanceSample;
    set<size_t> featuresInAllForests;

    cout << endl << "Uncovering associations... " << flush;
    executeRandomForest(filterData,targetIdx,featureWeights,forestOptions,filterOptions,filterOutput,forestFile);
    cout << "DONE" << endl;

    return( filterOutput );

  }

  void executeRandomForest(Treedata* filterData,
			   const size_t targetIdx,
			   const vector<num_t>& featureWeights,
			   ForestOptions* forestOptions,
			   FilterOptions* filterOptions,
			   FilterOutput& filterOutput,
			   const string& forestFile) {
    
    vector<vector<num_t> >         importanceMat( filterOptions->nPerms, vector<num_t>(filterData->nFeatures()) );
    vector<vector<num_t> > contrastImportanceMat( filterOptions->nPerms, vector<num_t>(filterData->nFeatures()) );

    size_t nFeatures = filterData->nFeatures();

    filterOutput.targetName = filterData->feature(targetIdx)->name();
    filterOutput.pValues.resize(nFeatures,1.0);
    filterOutput.importances.resize(nFeatures);
    filterOutput.sampleCounts.resize(nFeatures);
    filterOutput.correlations.resize(nFeatures);
    filterOutput.featureNames.resize(nFeatures);

    Progress progress;
    vector<num_t> contrastImportanceSample(filterOptions->nPerms);

    ftable_t frequency;

    ofstream toFile;
    if ( forestFile != "" ) {
      toFile.open(forestFile.c_str());
      toFile.close();
    }

    for(size_t permIdx = 0; permIdx < filterOptions->nPerms; ++permIdx) {

      filterData->permuteContrasts(&randoms_[0]);

      progress.update(1.0*permIdx/filterOptions->nPerms);

      StochasticForest SF;

      SF.learnRF(filterData,targetIdx,forestOptions,featureWeights,randoms_);

      if ( forestFile != "" ) {
	//ofstream toFile;
	toFile.open(forestFile.c_str(),ios::app);
	SF.writeForest(toFile);
	toFile.close();
      }

      SF.getMeanMinimalDepthValues(filterData,importanceMat[permIdx],contrastImportanceMat[permIdx]);

      // Store the new percentile value in the vector contrastImportanceSample
      contrastImportanceSample[permIdx] = math::mean( utils::removeNANs( contrastImportanceMat[permIdx] ) );

    }

    contrastImportanceSample = utils::removeNANs( contrastImportanceSample );

    //assert( !datadefs::containsNAN(contrastImportanceSample) );

    // Notify if the sample size of the null distribution is very low
    if ( filterOptions->nPerms > 1 && contrastImportanceSample.size() < 5 ) {
      cerr << " WARNING: Too few samples drawn ( " << contrastImportanceSample.size()
	   << " < 5 ) from the null distribution. Consider adding more permutations. Quitting..."
	   << endl;
      exit(1);
    }
    
    // Loop through each feature and calculate p-value for each
    for ( size_t featureIdx = 0; featureIdx < filterData->nFeatures(); ++featureIdx ) {

      vector<num_t> featureImportanceSample(filterOptions->nPerms);
      
      // Extract the sample for the real feature
      for ( size_t permIdx = 0; permIdx < filterOptions->nPerms; ++permIdx ) {
	featureImportanceSample[permIdx] = importanceMat[permIdx][featureIdx];
      }
      
      featureImportanceSample = utils::removeNANs(featureImportanceSample);
      
      assert( !datadefs::containsNAN(featureImportanceSample) );
      
      // If sample size is too small, assign p-value to 1.0
      if ( featureImportanceSample.size() < 5 ) {
	
	filterOutput.pValues[featureIdx] = 1.0;
	
      } else {
	
	// Perform WS-approximated t-test against the contrast sample
	bool WS = true;
	filterOutput.pValues[featureIdx] = math::ttest(contrastImportanceSample,featureImportanceSample,WS);
	
	// If for some reason the t-test returns NAN, turn that into 1.0
	// NOTE: 1.0 is better number than NAN when sorting
	if ( datadefs::isNAN( filterOutput.pValues[featureIdx] ) ) {
	  filterOutput.pValues[featureIdx] = 1.0;
	}
      }
      
      // Calculate mean importace score from the sample
      filterOutput.importances[featureIdx] = math::mean(featureImportanceSample);
      filterOutput.correlations[featureIdx] = filterData->pearsonCorrelation(targetIdx,featureIdx);
      filterOutput.sampleCounts[featureIdx] = filterData->nRealSamples(targetIdx,featureIdx);
      filterOutput.featureNames[featureIdx] = filterData->feature(featureIdx)->name();
    }
    
    sortFilterOutput(&filterOutput);
    
    size_t nSelectedFeatures = 0;

    for ( size_t i = 0; i < nFeatures; ++i ) {

      if ( filterOutput.pValues[i] > filterOptions->pValueThreshold ) {
	continue;
      }

      filterOutput.pValues[nSelectedFeatures] = filterOutput.pValues[i];
      filterOutput.importances[nSelectedFeatures] = filterOutput.importances[i];
      filterOutput.correlations[nSelectedFeatures] = filterOutput.correlations[i];
      filterOutput.sampleCounts[nSelectedFeatures] = filterOutput.sampleCounts[i];
      filterOutput.featureNames[nSelectedFeatures] = filterOutput.featureNames[i];
      ++nSelectedFeatures;
    }

    filterOutput.nAllFeatures = nFeatures - 1;
    filterOutput.nSignificantFeatures = nSelectedFeatures;
    filterOutput.pValues.resize(nSelectedFeatures);
    filterOutput.importances.resize(nSelectedFeatures);
    filterOutput.correlations.resize(nSelectedFeatures);
    filterOutput.sampleCounts.resize(nSelectedFeatures);
    filterOutput.featureNames.resize(nSelectedFeatures);

  }

  void sortFilterOutput(FilterOutput* filterOutput) {

    vector<size_t> sortIcs = utils::range(filterOutput->pValues.size());

    bool isIncreasingOrder = true;

    utils::sortDataAndMakeRef(isIncreasingOrder,filterOutput->pValues,sortIcs);
    utils::sortFromRef(filterOutput->importances,sortIcs);
    utils::sortFromRef(filterOutput->correlations,sortIcs);
    utils::sortFromRef(filterOutput->sampleCounts,sortIcs);
    utils::sortFromRef(filterOutput->featureNames,sortIcs);

  }

  TestOutput test(Treedata* testData) {

    assert(trainedModel_);

    TestOutput testOutput;

    size_t targetIdx = testData->getFeatureIdx(trainedModel_->getTargetName());

    size_t nThreads = randoms_.size();

    testOutput.targetName = trainedModel_->getTargetName();
    vector<num_t> confidence;
    if ( trainedModel_->isTargetNumerical() ) {
      testOutput.isTargetNumerical = true;
      if ( targetIdx != testData->end() ) {
	testOutput.numTrueData = testData->getFeatureData(targetIdx);
      } else {
	testOutput.numTrueData = vector<num_t>(testData->nSamples(),datadefs::NUM_NAN);
      }
      vector<num_t> predictions;
      trainedModel_->predict(testData,predictions,confidence,nThreads);
      testOutput.numPredictions = predictions;
    } else {
      testOutput.isTargetNumerical = false;
      if ( targetIdx != testData->end() ) {
	testOutput.catTrueData = testData->getRawFeatureData(targetIdx);
      } else {
	testOutput.catTrueData = vector<string>(testData->nSamples(),datadefs::STR_NAN);
      }
      vector<string> predictions;
      trainedModel_->predict(testData,predictions,confidence,nThreads);
      testOutput.catPredictions = predictions;
    }
    
    testOutput.sampleNames.resize( testData->nSamples() );
    for ( size_t i = 0; i < testData->nSamples(); ++i ) {
      testOutput.sampleNames[i] = testData->getSampleName(i);
    }
    testOutput.confidence = confidence;
    return( testOutput );
  }

  QuantilePredictionOutput predictQuantiles(Treedata* testData, const vector<num_t>& quantiles, const size_t nSamplesPerTree) {
    
    //assert(forestOptions->useQuantiles());
    assert(trainedModel_);

    QuantilePredictionOutput qPredOut;

    qPredOut.targetName = trainedModel_->getTargetName();
    qPredOut.quantiles = quantiles;

    assert(qPredOut.quantiles.size() > 0);

    size_t targetIdx = testData->getFeatureIdx(qPredOut.targetName);

    if ( targetIdx != testData->end() ) {
      qPredOut.trueData = testData->getFeatureData(targetIdx);
    } else {
      qPredOut.trueData = vector<num_t>(testData->nSamples(),datadefs::NUM_NAN);
    }

    trainedModel_->predictQuantiles(testData,qPredOut.predictions,quantiles,&randoms_[0],nSamplesPerTree);

    qPredOut.sampleNames.resize( testData->nSamples() );
    for ( size_t i = 0; i < testData->nSamples(); ++i ) {
      qPredOut.sampleNames[i] = testData->getSampleName(i);
    }

    return( qPredOut );

  }
  
  void load(const string& fileName) {
    
    if ( trainedModel_ ) {
      delete trainedModel_;
      trainedModel_ = NULL;
    }
    
    trainedModel_ = new StochasticForest();
    trainedModel_->loadForest(fileName);
  }

  QuantilePredictionOutput loadForestAndPredictQuantiles(const string& fileName, Treedata* testData, const vector<num_t>& quantiles, const size_t nSamplesPerTree) {
    
    QuantilePredictionOutput qPredOut;

    ifstream forestStream(fileName.c_str());
    assert(forestStream.good());

    size_t nQuantiles = quantiles.size();
    size_t nSamples = testData->nSamples();

    vector<vector<num_t> > finalData(nSamples);

    qPredOut.predictions.resize(nSamples,vector<num_t>(nQuantiles,datadefs::NUM_NAN));

    size_t iter = 0;
    while ( forestStream.good() ) {

      RootNode rootNode(forestStream);

      qPredOut.targetName = rootNode.getTargetName();

      cout << "Tree " << iter << " loaded" << endl;
      iter++;

      for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {

        vector<num_t> treeData = rootNode.getChildLeafTrainData(testData,sampleIdx);
        size_t nSamplesInTreeData = treeData.size();

        for ( size_t i = 0; i < nSamplesPerTree; ++i ) {
          finalData[sampleIdx].push_back( treeData[ randoms_[0].integer() % nSamplesInTreeData ] );
        }

      }
    }

    for ( size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx ) {
      //cout << "Test sample " << sampleIdx << " yielded " << finalData[sampleIdx].size() << " samples from the conditional distribution" << endl;
      sort(finalData[sampleIdx].begin(),finalData[sampleIdx].end());
      for ( size_t q = 0; q < nQuantiles; ++q ) {
        qPredOut.predictions[sampleIdx][q] = math::percentile(finalData[sampleIdx],quantiles[q]);
      }
    }

    cout << nQuantiles << " quantiles collected" << endl;

    //qPredOut.targetName = trainedModel_->getTargetName();
    qPredOut.quantiles = quantiles;

    assert(qPredOut.quantiles.size() > 0);

    size_t targetIdx = testData->getFeatureIdx(qPredOut.targetName);

    if ( targetIdx != testData->end() ) {
      qPredOut.trueData = testData->getFeatureData(targetIdx);
    } else {
      qPredOut.trueData = vector<num_t>(testData->nSamples(),datadefs::NUM_NAN);
    }

    qPredOut.sampleNames.resize( testData->nSamples() );
    for ( size_t i = 0; i < testData->nSamples(); ++i ) {
      qPredOut.sampleNames[i] = testData->getSampleName(i);
    }
    
    return(qPredOut);

  }

  void save(const string& fileName) {

    assert(trainedModel_);
    
    ofstream toFile(fileName);

    trainedModel_->writeForest( toFile );

  }

  StochasticForest* forestRef() { return( trainedModel_ ); }
  
  /*
    void updateFeatureFrequency(ftable_t& frequency, StochasticForest* SF) {
    
    // We loop through all the trees
    for ( size_t treeIdx = 0; treeIdx < SF->nTrees(); ++treeIdx ) {
    
    set<size_t> featuresInTree = SF->tree(treeIdx)->getFeaturesInTree();
    
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
    
    if ( frequency.find(lokey) == frequency.end() || frequency[lokey].find(hikey) == frequency[lokey].end() ) {
    frequency[lokey][hikey] = 1;
    } else {
    ++frequency[lokey][hikey];
    }
    
    ++it2;
    
    }
    }
    
    }
    
    }
  */


  /*
    template<typename T, size_t pos, bool isAscending>
    struct SortBy {
    
    bool operator()(const vector<T>& a, const vector<T>& b) {
    return( isAscending ? a[pos] < b[pos] : a[pos] > b[pos] );
    }
    
    };
    
    void printPairInteractionsToFile(Treedata* trainData, ftable_t& frequency, const string& fileName) {
    
    ofstream toFile(fileName.c_str());
    
    vector<vector<size_t> > fTuples;
    
    for ( ftable_t::const_iterator it1( frequency.begin() ); it1 != frequency.end(); ++it1 ) {
    for ( unordered_map<size_t,size_t>::const_iterator it2( it1->second.begin()); it2 != it1->second.end(); ++it2 ) {
    fTuples.push_back( {it1->first,it2->first,it2->second} );
    //toFile << trainData->getFeatureName( it1->first ) << "\t" << trainData->getFeatureName( it2->first ) << "\t" << it2->second << endl;
    }
    }
    
    SortBy<size_t,2,false> sorter;
    
    //cout << sorter(fTuples[0],fTuples[1]) << endl;
    
    sort(fTuples.begin(),fTuples.end(),sorter);
    
    for ( size_t i = 0; i < fTuples.size(); ++i ) {
    toFile << trainData->getFeatureName( fTuples[i][0] ) << "\t" << trainData->getFeatureName( fTuples[i][1] ) << "\t" << 100.0 * fTuples[i][2] / ( filterOptions->nPerms * forestOptions->nTrees ) << endl;
    }
    
    toFile.close();
    
    }
  */

  void resetRandomNumberGenerators(const size_t nThreads, int seed) {

    assert( nThreads >= 1 );

    if ( seed < 0 ) {
      //cerr << "Invalid random seed (" << seed << ")" << endl;
      //exit(1);
      seed = distributions::generateSeed();
    }

    // We have as many random number generators as there are threads
    randoms_.resize(nThreads);

    // Set the seeds for each RNG to be seed + threadIdx
    for ( size_t threadIdx = 0; threadIdx < nThreads; ++threadIdx ) {
      randoms_[threadIdx].seed(seed + threadIdx);
    }

  }


private:

  vector<distributions::Random> randoms_;

  StochasticForest* trainedModel_;
  

};

#endif
