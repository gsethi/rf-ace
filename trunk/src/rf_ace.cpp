#include <cstdlib>
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <iomanip>
#include <algorithm>
#include <map>

#include "rf_ace.hpp"
#include "argparse.hpp"
#include "stochasticforest.hpp"
#include "treedata.hpp"
#include "datadefs.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "statistics.hpp"
#include "progress.hpp"
#include "math.hpp"

using namespace std;
using datadefs::num_t;

statistics::RF_statistics executeRandomForest(Treedata& treeData,
					      const options::General_options& gen_op,
					      vector<num_t>& pValues,
					      vector<num_t>& importanceValues,
					      vector<num_t>& contrastImportanceSample);

void rf_ace_filter(options::General_options& gen_op);

void rf_ace(options::General_options& gen_op);

void rf_ace_recombine(options::General_options& gen_op);

void printGeneralSetup(Treedata& treeData, const options::General_options& gen_op); 

void printAssociationsToFile(options::General_options& gen_op, 
			     vector<string>& featureNames,
			     vector<num_t>& pValues,
			     vector<num_t>& importanceValues,
			     vector<num_t>& contrastImportanceSample,
			     vector<num_t>& correlations,
			     vector<size_t>& sampleCounts);

void printPredictionToFile(StochasticForest& SF, Treedata& treeData, const string& targetName, const string& fileName);

int main(const int argc, char* const argv[]) {

  options::printHeader(cout);

  ArgParse parser(argc,argv);

  // Structs that store all the user-specified command-line arguments
  options::General_options gen_op;
  gen_op.loadUserParams(parser);
  
  // With no input arguments the help is printed
  if ( argc == 1 || gen_op.printHelp ) {
    //options::printFilterOverview();
    gen_op.help();
    return(EXIT_SUCCESS);

  }

  rface::validateRequiredParameters(gen_op);

  if ( gen_op.isFilter ) {

    rf_ace_filter(gen_op);

  } else if ( gen_op.isRecombiner ) {
    
    cout << " *(EXPERIMENTAL) RF-ACE RECOMBINER ACTIVATED* " << endl;
        
    if ( gen_op.recombinePerms == 0 ) {
      cerr << "Currently the number of permutations to be recombined ( -" 
	   << gen_op.recombinePerms_s << " / --" << gen_op.recombinePerms_l << endl
	   << " ) needs to be explicitly specified." << endl;
      exit(1);
    }

    rf_ace_recombine(gen_op);

  } else {

    rf_ace(gen_op);

  }
  
}

void rf_ace_filter(options::General_options& gen_op) {
  
  statistics::RF_statistics RF_stat;

  // Read train data into Treedata object
  cout << "Reading file '" << gen_op.input << "', please wait... " << flush;
  Treedata treeData(gen_op.input,gen_op.dataDelimiter,gen_op.headerDelimiter,gen_op.seed);
  cout << "DONE" << endl;

  rface::updateTargetStr(treeData,gen_op);
  rface::pruneFeatureSpace(treeData,gen_op);
  rface::updateMTry(treeData,gen_op);
      
  if(treeData.nSamples() < 2 * gen_op.nodeSize) {
    cerr << "Not enough samples (" << treeData.nSamples() << ") to perform a single split" << endl;
    exit(1);
  }
  
  printGeneralSetup(treeData,gen_op);
      
  // Store the start time (in clock cycles) just before the analysis
  clock_t clockStart( clock() );
  
  ////////////////////////////////////////////////////////////////////////
  //  STEP 1 -- MULTIVARIATE ASSOCIATIONS WITH RANDOM FOREST ENSEMBLES  //
  ////////////////////////////////////////////////////////////////////////     
  vector<num_t> pValues; 
  vector<num_t> importanceValues; 
  vector<num_t> contrastImportanceSample;
  
  cout << "===> Uncovering associations... " << flush;
  RF_stat = executeRandomForest(treeData,gen_op,pValues,importanceValues,contrastImportanceSample);
  cout << "DONE" << endl;
  
  /////////////////////////////////////////////////
  //  STEP 2 -- FEATURE FILTERING WITH P-VALUES  //
  /////////////////////////////////////////////////
  
  cout << "===> Filtering features... " << flush;

  size_t targetIdx = treeData.getFeatureIdx(gen_op.targetStr);
  
  vector<string> featureNames( treeData.nFeatures() );
  vector<num_t> correlations ( treeData.nFeatures() );
  vector<size_t> sampleCounts( treeData.nFeatures() );

  for ( size_t i = 0; i < treeData.nFeatures(); ++i ) {
    featureNames[i] = treeData.getFeatureName(i);
    correlations[i] = treeData.pearsonCorrelation(targetIdx,i);
    sampleCounts[i] = treeData.nRealSamples(targetIdx,i);
  }

  printAssociationsToFile(gen_op,
			  featureNames,
			  pValues,
			  importanceValues,
			  contrastImportanceSample,
			  correlations,
			  sampleCounts);

  if ( gen_op.log != "" ) {
    
    ofstream toLogFile(gen_op.log.c_str());
    options::printHeader(toLogFile);
    RF_stat.print(toLogFile);
    toLogFile.close();
    
    toLogFile.open("contrasts.tsv");
    RF_stat.printContrastImportance(toLogFile);
    toLogFile.close();
    
  }
  
  cout << 1.0 * ( clock() - clockStart ) / CLOCKS_PER_SEC << " seconds elapsed." << endl << endl;
  
  cout << "Association file created, format:" << endl;
  cout << "TARGET   PREDICTOR   LOG10(P-VALUE)   IMPORTANCE   CORRELATION   NSAMPLES" << endl;
  cout << endl;
  cout << "RF-ACE completed successfully." << endl;
  cout << endl;
  
  exit(0);
}

void printGeneralSetup(Treedata& treeData, const options::General_options& gen_op) {
  
  // After masking, it's safe to refer to features as indices
  // TODO: rf_ace.cpp: this should be made obsolete; instead of indices, use the feature headers
  size_t targetIdx = treeData.getFeatureIdx(gen_op.targetStr);
  
  size_t nAllFeatures = treeData.nFeatures();
  size_t nRealSamples = treeData.nRealSamples(targetIdx);
  num_t realFraction = 1.0*nRealSamples / treeData.nSamples();
  
  //Before number crunching, print values of parameters of RF-ACE
  cout << "General configuration:" << endl;
  cout << "    nfeatures" << setw(options::maxWidth-9) << "" << "= " << nAllFeatures << endl;
  cout << "    nsamples"  << setw(options::maxWidth-8) << "" << "= " << treeData.nRealSamples(targetIdx) << " / " << treeData.nSamples() << " ( " << 100.0 * ( 1 - realFraction ) << " % missing )" << endl;
  cout << "    tree type" << setw(options::maxWidth-9) << "" << "= ";
  if(treeData.isFeatureNumerical(targetIdx)) { cout << "Regression CART" << endl; } else { cout << treeData.nCategories(targetIdx) << "-class CART" << endl; }
  cout << "  --" << gen_op.dataDelimiter_l << setw( options::maxWidth - gen_op.dataDelimiter_l.size() ) << ""
       << "= '" << gen_op.dataDelimiter << "'" << endl;
  cout << "  --" << gen_op.headerDelimiter_l << setw( options::maxWidth - gen_op.headerDelimiter_l.size() ) << ""
       << "= '" << gen_op.headerDelimiter << "'" << endl;
  cout << "  --" << gen_op.input_l << setw( options::maxWidth - gen_op.input_l.size() ) << ""
       << "= " << gen_op.input << endl;
  cout << "  --" << gen_op.targetStr_l << setw( options::maxWidth - gen_op.targetStr_l.size() ) << ""
       << "= " << gen_op.targetStr << " ( index " << targetIdx << " )" << endl;
  cout << "  --" << gen_op.output_l << setw( options::maxWidth - gen_op.output_l.size() ) << ""
       << "= "; if ( gen_op.output != "" ) { cout << gen_op.output << endl; } else { cout << "NOT SET" << endl; }
  cout << "  --" << gen_op.log_l << setw( options::maxWidth - gen_op.log_l.size() ) << ""
       << "= "; if( gen_op.log != "" ) { cout << gen_op.log << endl; } else { cout << "NOT SET" << endl; }
  cout << "  --" << gen_op.seed_l << setw( options::maxWidth - gen_op.seed_l.size() ) << ""
       << "= " << gen_op.seed << endl;
  cout << endl;

  cout << "Stochastic Forest configuration:" << endl;
  cout << "  --" << gen_op.nTrees_l << setw( options::maxWidth - gen_op.nTrees_l.size() ) << ""
       << "= "; if(gen_op.nTrees == 0) { cout << "DEFAULT" << endl; } else { cout << gen_op.nTrees << endl; }
  cout << "  --" << gen_op.mTry_l << setw( options::maxWidth - gen_op.mTry_l.size() ) << ""
       << "= " << gen_op.mTry << endl;
  cout << "  --" << gen_op.nMaxLeaves_l << setw( options::maxWidth - gen_op.nMaxLeaves_l.size() ) << ""
       << "= " << gen_op.nMaxLeaves << endl;
  cout << "  --" << gen_op.nodeSize_l << setw( options::maxWidth - gen_op.nodeSize_l.size() ) << ""
       << "= "; if(gen_op.nodeSize == 0) { cout << "DEFAULT" << endl; } else { cout << gen_op.nodeSize << endl; }
  cout << "  --" << gen_op.shrinkage_l << setw( options::maxWidth - gen_op.shrinkage_l.size() ) << ""
       << "= " << gen_op.shrinkage << endl;
  cout << endl;

  cout << "Statistical test configuration [Filter only]:" << endl;
  cout << "  --" << gen_op.nPerms_l << setw( options::maxWidth - gen_op.nPerms_l.size() ) << ""
       << "= " << gen_op.nPerms << endl;
  cout << "  --" << gen_op.pValueThreshold_l << setw( options::maxWidth - gen_op.pValueThreshold_l.size() ) << ""
       << "= " << gen_op.pValueThreshold << " (lower limit)" <<endl;
  cout << "  --" << gen_op.importanceThreshold_l << setw( options::maxWidth - gen_op.importanceThreshold_l.size() ) << ""
       << "= " << gen_op.importanceThreshold << " (upper limit)" << endl;
  cout << endl;

}


statistics::RF_statistics executeRandomForest(Treedata& treeData,
					      const options::General_options& gen_op,
					      vector<num_t>& pValues,
					      vector<num_t>& importanceValues,
					      vector<num_t>& contrastImportanceSample) {
  
  vector<vector<size_t> > nodeMat(gen_op.nPerms,vector<size_t>(gen_op.nTrees));
  
  vector<vector<num_t> >         importanceMat( gen_op.nPerms, vector<num_t>(treeData.nFeatures()) );
  vector<vector<num_t> > contrastImportanceMat( gen_op.nPerms, vector<num_t>(treeData.nFeatures()) );
  
  size_t nFeatures = treeData.nFeatures();
  
  pValues.clear();
  pValues.resize(nFeatures,1.0);
  importanceValues.resize(2*nFeatures);
  
  StochasticForest::Parameters parameters;
  parameters.model = StochasticForest::RF;
  parameters.inBoxFraction = 1.0;
  parameters.sampleWithReplacement = true;
  parameters.isRandomSplit = true;
  parameters.nTrees       = gen_op.nTrees;
  parameters.mTry         = gen_op.mTry;
  parameters.nMaxLeaves   = gen_op.nMaxLeaves;
  parameters.nodeSize     = gen_op.nodeSize;
  parameters.useContrasts = true;
  parameters.shrinkage    = gen_op.shrinkage;

  if(gen_op.nPerms > 1 || gen_op.reportContrasts ) {
    parameters.useContrasts = true;
  } else {
    parameters.useContrasts = false;
  }
  
  
  Progress progress;
  clock_t clockStart( clock() );
  contrastImportanceSample.resize(gen_op.nPerms);
  
  for(int permIdx = 0; permIdx < static_cast<int>(gen_op.nPerms); ++permIdx) {
    
    progress.update(1.0*permIdx/gen_op.nPerms);
    
    // Initialize the Random Forest object
    StochasticForest SF(&treeData,gen_op.targetStr,parameters);
    
    // Get the number of nodes in each tree in the forest
    nodeMat[permIdx] = SF.nNodes();
    
    SF.getImportanceValues(importanceMat[permIdx],contrastImportanceMat[permIdx]);
    
    // Store the new percentile value in the vector contrastImportanceSample
    contrastImportanceSample[permIdx] = math::mean( utils::removeNANs( contrastImportanceMat[permIdx] ) );
    
  }

  // Remove possible NANs from the contrast sample
  contrastImportanceSample = utils::removeNANs(contrastImportanceSample);
  
  // Notify if the sample size of the null distribution is very low
  if ( gen_op.nPerms > 1 && contrastImportanceSample.size() < 5 ) {
    cerr << " Too few samples drawn ( " << contrastImportanceSample.size() << " < 5 ) from the null distribution. Consider adding more permutations. Quitting..." << endl;
    exit(0);
  }
  
  // Loop through each feature and calculate p-value for each
  for(size_t featureIdx = 0; featureIdx < treeData.nFeatures(); ++featureIdx) {
    
    vector<num_t> featureImportanceSample(gen_op.nPerms);
    
    // Extract the sample for the real feature
    for(size_t permIdx = 0; permIdx < gen_op.nPerms; ++permIdx) {
      featureImportanceSample[permIdx] = importanceMat[permIdx][featureIdx];
    }

    // Remove missing values
    featureImportanceSample = utils::removeNANs(featureImportanceSample);

    // If sample size is too small, assign p-value to 1.0
    if ( featureImportanceSample.size() < 5 ) {

      pValues[featureIdx] = 1.0;

    } else {
      
      // Perform t-test against the contrast sample
      pValues[featureIdx] = math::ttest(featureImportanceSample,contrastImportanceSample);
      
    }

    // If for some reason the t-test returns NAN, turn that into 1.0
    // NOTE: 1.0 is better number than NAN when sorting
    if ( datadefs::isNAN( pValues[featureIdx] ) ) {
      pValues[featureIdx] = 1.0;
    }
    
    // Calculate mean importace score from the sample
    importanceValues[featureIdx] = math::mean(featureImportanceSample);
    
  }
  
  // Store statistics of the run in an object
  statistics::RF_statistics RF_stat(importanceMat,contrastImportanceMat,nodeMat, 1.0 * ( clock() - clockStart ) / CLOCKS_PER_SEC );
  
  // Resize importance value container to proper dimensions
  importanceValues.resize( treeData.nFeatures() );
  
  // Return statistics
  return( RF_stat );
  
}


void rf_ace(options::General_options& gen_op) {
  
  // Read train data into Treedata object
  cout << "Reading file '" << gen_op.input << "', please wait... " << flush;
  Treedata treeData(gen_op.input,gen_op.dataDelimiter,gen_op.headerDelimiter,gen_op.seed);
  cout << "DONE" << endl;
  
  rface::updateTargetStr(treeData,gen_op);

  // Initialize parameters struct for the stochastic forest and load defaults
  StochasticForest::Parameters parameters;
  //if ( PB_op.isGBT ) {
  // parameters.model = StochasticForest::GBT;
  // parameters.inBoxFraction = 0.5;
  // parameters.sampleWithReplacement = false;
  // parameters.isRandomSplit = false;
  // gen_op.setGBTDefaults();
  // else if ( PB_op.isRF ) {
  parameters.model = StochasticForest::RF;
  parameters.inBoxFraction = 1.0;
  parameters.sampleWithReplacement = true;
  parameters.isRandomSplit = true;
  // gen_op.setRFDefaults();
  //} else {
  //   cerr << "Model needs to be specified explicitly" << endl;
  //   exit(1);
  // }

  rface::pruneFeatureSpace(treeData,gen_op);
  rface::updateMTry(treeData,gen_op);

  // Copy command line parameters to parameters struct for the stochastic forest
  parameters.nTrees       = gen_op.nTrees;
  parameters.mTry         = gen_op.mTry;
  parameters.nMaxLeaves   = gen_op.nMaxLeaves;
  parameters.nodeSize     = gen_op.nodeSize;
  parameters.useContrasts = false;
  parameters.shrinkage    = gen_op.shrinkage;
    
  printGeneralSetup(treeData,gen_op);

  // Store the start time (in clock cycles) just before the analysis
  clock_t clockStart( clock() );
      
  //if ( PB_op.isGBT ) {
  //   cout << "===> Growing GBT predictor... " << flush;
  //} else {
  cout << "===> Growing RF predictor... " << flush;
  // }
  
  StochasticForest SF(&treeData,gen_op.targetStr,parameters);
  cout << "DONE" << endl << endl;

  size_t targetIdx = treeData.getFeatureIdx(gen_op.targetStr);
  vector<num_t> data = utils::removeNANs(treeData.getFeatureData(targetIdx));

  num_t oobError = SF.getOobError();
  num_t ibOobError =  SF.getError();
  num_t nullError;
  if ( treeData.isFeatureNumerical(targetIdx) ) {
    nullError = math::squaredError(data) / data.size();
  } else {
    map<num_t,size_t> freq = math::frequency(data);
    nullError = 1.0 * ( data.size() - freq[ math::mode<num_t>(data) ] ) / data.size();
  }

  cout << "RF training error measures:" << endl;
  cout << "  data variance = " << nullError << endl;
  cout << "      OOB error = " << oobError << endl;
  cout << "   IB+OOB error = " << ibOobError << endl;
  cout << "    1 - OOB/var = " << 1 - oobError / nullError << endl;
  cout << endl;

  if ( gen_op.predictionData == "" ) {

    cout << "===> Writing predictor to file... " << flush;
    SF.printToFile( gen_op.output );
    cout << "DONE" << endl << endl;
    
    cout << endl;
    cout << 1.0 * ( clock() - clockStart ) / CLOCKS_PER_SEC << " seconds elapsed." << endl << endl;
    
    cout << "RF-ACE predictor built and saved to a file." << endl;
    cout << endl;

  } else {

    cout << "===> Making predictions with test data... " << flush;

    printPredictionToFile(SF,treeData,gen_op.targetStr,gen_op.output);

    cout << "DONE" << endl;

    cout << endl;

    cout << 1.0 * ( clock() - clockStart ) / CLOCKS_PER_SEC << " seconds elapsed." << endl << endl;


    cout << "Prediction file '" << gen_op.output << "' created. Format:" << endl;
    cout << "TARGET   SAMPLE_ID     PREDICTION    CONFIDENCE" << endl;
    cout << endl;


    cout << "RF-ACE completed successfully." << endl;
    cout << endl;

  }
  
  exit(0);
}

vector<string> readFeatureMask(const string& fileName);

void printPredictionToFile(StochasticForest& SF, Treedata& treeData, const string& targetName, const string& fileName) {

  ofstream toPredictionFile(fileName.c_str());
  
  //size_t targetIdx = treeData.getFeatureIdx(targetName);
  
  if ( SF.isTargetNumerical() ) {
    
    vector<num_t> prediction;
    vector<num_t> confidence;
    SF.predict(prediction,confidence);
    
    for(size_t i = 0; i < prediction.size(); ++i) {
      toPredictionFile << targetName << "\t" << treeData.getSampleName(i) << "\t" << prediction[i] << "\t" << setprecision(3) << confidence[i] << endl;
    }
    
  } else {

    vector<string> prediction;
    vector<num_t> confidence;
    SF.predict(prediction,confidence);
    
    for(size_t i = 0; i < prediction.size(); ++i) {
      toPredictionFile << targetName << "\t" << treeData.getSampleName(i) << "\t" << prediction[i] << "\t" << setprecision(3) << confidence[i] << endl;
    }
    
  }

  toPredictionFile.close();
   
}

void printAssociationsToFile(options::General_options& gen_op, 
			     vector<string>& featureNames, 
			     vector<num_t>& pValues,
			     vector<num_t>& importanceValues,
			     vector<num_t>& contrastImportanceSample,
			     vector<num_t>& correlations,
			     vector<size_t>& sampleCounts) {

  assert( featureNames.size() == pValues.size() );
  assert( featureNames.size() == importanceValues.size() );
  assert( featureNames.size() == correlations.size() );
  assert( featureNames.size() == sampleCounts.size() );

  ofstream toAssociationFile(gen_op.output.c_str());
  //toAssociationFile.precision(8);

  // Initialize mapping vector to identity mapping: range 0,1,...,N-1
  // NOTE: when not sorting we use the identity map, otherwise the sorted map
  vector<size_t> featureIcs = utils::range( featureNames.size() );

  // If we prefer sorting the outputs wrt. significance (either p-value or importance)
  //if ( !gen_op.noSort ) {
  // If there are more than one permutation, we can compute the p-values and thus sort wrt. them
  if ( gen_op.nPerms > 1 ) {
    bool isIncreasingOrder = true;
    datadefs::sortDataAndMakeRef(isIncreasingOrder,pValues,featureIcs);
    datadefs::sortFromRef<num_t>(importanceValues,featureIcs);
  } else { // ... otherwise we can sort wrt. importance scores
    bool isIncreasingOrder = false;
    datadefs::sortDataAndMakeRef(isIncreasingOrder,importanceValues,featureIcs);
    datadefs::sortFromRef<num_t>(pValues,featureIcs);
  }
  datadefs::sortFromRef<string>(featureNames,featureIcs);
  datadefs::sortFromRef<num_t>(correlations,featureIcs);
  datadefs::sortFromRef<size_t>(sampleCounts,featureIcs);
  //}

  //assert( gen_op.targetStr == treeData.getFeatureName(targetIdx) );

  size_t nSignificantFeatures = 0;

  size_t nFeatures = featureIcs.size();

  for ( size_t i = 0; i < featureIcs.size(); ++i ) {
    //size_t featureIdx = featureIcs[i];

    // With more than 1 permutation we look at p-value threshold
    if ( gen_op.nPerms > 1 && pValues[i] > gen_op.pValueThreshold ) {
      continue;
    } 

    if ( importanceValues[i] < gen_op.importanceThreshold ) {
      continue;
    }

    if ( featureNames[i] == gen_op.targetStr ) {
      --nFeatures;
      continue;
    }

    ++nSignificantFeatures;

    // cout << " " << i << ":" << featureIdx;

    toAssociationFile << gen_op.targetStr.c_str() << "\t" << featureNames[i]
                      << "\t" << scientific << log10(pValues[i]) << "\t" << scientific << importanceValues[i] << "\t"
		      << scientific << correlations[i] << "\t" << sampleCounts[i] << endl;
  }

  if ( gen_op.reportContrasts ) {
    for ( size_t i = 0; i < contrastImportanceSample.size(); ++i ) {
      toAssociationFile << gen_op.targetStr.c_str() << "\t" << datadefs::CONTRAST
			<< "\t" << scientific << 0.0 << "\t" << scientific << contrastImportanceSample[i] << "\t" 
			<< scientific << 0.0 << "\t" << 0 << endl;
    }
  }

  toAssociationFile.close();


  // Print some statistics
  // NOTE: we're subtracting the target from the total head count, that's why we need to subtract by 1
  cout << "DONE, " << nSignificantFeatures << " / " << nFeatures << " features ( "
       << 100.0 * nSignificantFeatures / nFeatures << " % ) left " << endl;


}

void rf_ace_recombine(options::General_options& gen_op) {

  // Read all lines from file
  vector<string> associations = utils::readListFromFile(gen_op.input,'\n');

  // Initialize containers for storing the maps from associated features to 
  // variables
  map<string,vector<num_t> > associationMap;
  map<string,num_t> correlationMap;
  map<string,size_t> sampleCountMap;

  // For reference extract the first association ...
  vector<string> association = utils::split(associations[0],'\t');

  // ... and from the first association extract the target feature
  gen_op.targetStr = association[0];

  // Go through all associations in the list and update the map containers
  for ( size_t i = 0; i < associations.size(); ++i ) {
    
    // Extract the association from line "i"
    association = utils::split(associations[i],'\t');

    // Extract the feature name from line "i"
    string featureName = association[1];

    // Make sure the target feature is listed as first in each entry in the file
    // that is to be recombined (thus, we know that the entries are related to 
    // the same variable)
    assert( gen_op.targetStr == association[0] );
    
    // Extract the importance value ...
    num_t importanceValue = utils::str2<num_t>(association[3]);
    
    // ... and push it back to the container 
    associationMap[featureName].push_back(importanceValue);

    // 
    correlationMap[featureName] = utils::str2<num_t>(association[4]);
    sampleCountMap[featureName] = utils::str2<size_t>(association[5]);
  }

  assert( associationMap.find( datadefs::CONTRAST ) != associationMap.end() );

  // Subtract one because contrast is included
  size_t nRealFeatures = associationMap.size() - 1;

  vector<string> featureNames(nRealFeatures);
  vector<num_t>  pValues(nRealFeatures);
  vector<num_t>  importanceValues(nRealFeatures);
  vector<num_t>  correlations(nRealFeatures);
  vector<size_t> sampleCounts(nRealFeatures);

  vector<num_t> contrastImportanceSample = associationMap[datadefs::CONTRAST];

  // Notify if the sample size of the null distribution is very low
  if ( contrastImportanceSample.size() < 5 ) {
    cerr << " Too few samples drawn ( " << contrastImportanceSample.size() << " < 5 ) from the null distribution. Consider adding more permutations. Quitting..." << endl;
    exit(0);
  }

  // Keep count of the total number of features for which there are 
  // enough ( >= 5 ) importance values
  size_t i = 0;

  // Go through all features in the container
  for ( map<string,vector<num_t> >::const_iterator it(associationMap.begin() ); it != associationMap.end(); ++it ) {
    
    // For clarity map the iterator into more representative variable names
    string featureName = it->first;
    vector<num_t> importanceSample = it->second;

    // If there are enough ( >= 5 ) importance values, we can compute the t-test 
    if ( importanceSample.size() >= 5 && featureName != datadefs::CONTRAST ) {
      featureNames[i] = featureName;
      importanceValues[i] = math::mean(importanceSample);
      pValues[i] = math::ttest(importanceSample,contrastImportanceSample);
      correlations[i] = correlationMap[featureName];
      sampleCounts[i] = sampleCountMap[featureName];
     
      ++i;
    }

  }
  
  featureNames.resize(i);
  pValues.resize(i);
  importanceValues.resize(i);
  correlations.resize(i);
  sampleCounts.resize(i);

  gen_op.reportContrasts = false;

  printAssociationsToFile(gen_op,
			  featureNames,
			  pValues,
			  importanceValues,
			  contrastImportanceSample,
			  correlations,
			  sampleCounts);

}


