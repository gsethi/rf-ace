#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include "argparse.hpp"
#include "datadefs.hpp"
#include "treedata.hpp"
#include "utils.hpp"

using namespace std;
using datadefs::num_t;

namespace options {

  enum ForestType {GBT,RF,CART,UNKNOW};

  // Default general configuration
  const bool       GENERAL_DEFAULT_PRINT_HELP = false;
  const bool       GENERAL_DEFAULT_IS_FILTER = false;
  const size_t     GENERAL_DEFAULT_RECOMBINE_PERMS = 0;
  const bool       GENERAL_DEFAULT_REPORT_CONTRASTS = false;
  const char       GENERAL_DEFAULT_DATA_DELIMITER = '\t';
  const char       GENERAL_DEFAULT_HEADER_DELIMITER = ':';
  const size_t     GENERAL_DEFAULT_MIN_SAMPLES = 5;
  const int        GENERAL_DEFAULT_SEED = -1;
  const ForestType GENERAL_DEFAULT_FOREST_TYPE = RF;

  // Statistical test default configuration
  const size_t     ST_DEFAULT_N_PERMS = 20;
  const num_t      ST_DEFAULT_P_VALUE_THRESHOLD = 0.05;
  const num_t      ST_DEFAULT_IMPORTANCE_THRESHOLD = 0;
  const bool       ST_DEFAULT_REPORT_NONEXISTENT_FEATURES = false;

  // Random Forest default configuration
  const size_t     RF_DEFAULT_N_TREES = 100;
  const size_t     RF_DEFAULT_M_TRY = 0;
  const size_t     RF_DEFAULT_N_MAX_LEAVES = 100;
  const size_t     RF_DEFAULT_NODE_SIZE = 3;
  const num_t      RF_DEFAULT_IN_BOX_FRACTION = 1.0;
  const num_t      RF_DEFAULT_SAMPLE_WITH_REPLACEMENT = true;
  const bool       RF_DEFAULT_USE_CONTRASTS = true;
  const bool       RF_DEFAULT_IS_RANDOM_SPLIT = true;
  const num_t      RF_DEFAULT_SHRINKAGE = 0.0;

  // Gradient Boosting Trees default configuration
  const size_t     GBT_DEFAULT_N_TREES = 100;
  const size_t     GBT_DEFAULT_M_TRY = 0;
  const size_t     GBT_DEFAULT_N_MAX_LEAVES = 6;
  const size_t     GBT_DEFAULT_NODE_SIZE = 3;
  const num_t      GBT_DEFAULT_IN_BOX_FRACTION = 0.5;
  const num_t      GBT_DEFAULT_SAMPLE_WITH_REPLACEMENT = false;
  const bool       GBT_DEFAULT_USE_CONTRASTS = false;
  const bool       GBT_DEFAULT_IS_RANDOM_SPLIT = false;
  const num_t      GBT_DEFAULT_SHRINKAGE = 0.1;

  // Determines the amount of indentation in help print-outs
  const size_t maxWidth = 20;
  
  struct General_options {

  private:
    ArgParse* parser_;

  public:
    // EXPERIMENTAL
    size_t recombinePerms; const string recombinePerms_s; const string recombinePerms_l;

    // I/O related and general paramters
    bool printHelp; const string printHelp_s; const string printHelp_l;
    bool isFilter; const string isFilter_s; const string isFilter_l;
    bool reportContrasts; const string reportContrasts_s; const string reportContrasts_l;
    string input; const string input_s; const string input_l;
    string output; const string output_s; const string output_l;
    string targetStr; const string targetStr_s; const string targetStr_l;
    string whiteList; const string whiteList_s; const string whiteList_l;
    string blackList; const string blackList_s; const string blackList_l;
    string predictionData; const string predictionData_s; const string predictionData_l;
    string log; const string log_s; const string log_l;
    char dataDelimiter; const string dataDelimiter_s; const string dataDelimiter_l;
    char headerDelimiter; const string headerDelimiter_s; const string headerDelimiter_l;
    size_t pruneFeatures; const string pruneFeatures_s; const string pruneFeatures_l;
    int seed; string seed_s; string seed_l;

    // Forest Type
    ForestType forestType; const string forestType_s; const string forestType_l;

    // Random Forest related parameters
    size_t nTrees; const string nTrees_s; const string nTrees_l;
    size_t mTry; const string mTry_s; const string mTry_l;
    size_t nMaxLeaves; const string nMaxLeaves_s; const string nMaxLeaves_l;
    size_t nodeSize; const string nodeSize_s; const string nodeSize_l;
    num_t shrinkage; const string shrinkage_s; const string shrinkage_l;

    // Parameters that cannot be controlled through the interface
    num_t inBoxFraction;
    bool sampleWithReplacement;
    bool isRandomSplit;
    bool useContrasts;

    // Statistical test related parameters
    size_t nPerms; const string nPerms_s; const string nPerms_l;
    num_t pValueThreshold; const string pValueThreshold_s; const string pValueThreshold_l;
    num_t importanceThreshold; const string importanceThreshold_s; const string importanceThreshold_l;
    bool reportAllFeatures; const string reportAllFeatures_s; const string reportAllFeatures_l;

    General_options(const int argc, char* const argv[]):
      // EXPERIMENTAL PARAMETERS
      recombinePerms(GENERAL_DEFAULT_RECOMBINE_PERMS),recombinePerms_s("R"),recombinePerms_l("recombine"),
      // I/O and general parameters
      printHelp(GENERAL_DEFAULT_PRINT_HELP),printHelp_s("h"),printHelp_l("help"),
      isFilter(GENERAL_DEFAULT_IS_FILTER),isFilter_s("L"),isFilter_l("filter"),
      reportContrasts(GENERAL_DEFAULT_REPORT_CONTRASTS),reportContrasts_s("C"),reportContrasts_l("report_contrasts"),
      input(""),input_s("I"),input_l("input"),
      output(""),output_s("O"),output_l("output"),
      targetStr(""),targetStr_s("i"),targetStr_l("target"),
      whiteList(""),whiteList_s("W"),whiteList_l("whitelist"),
      blackList(""),blackList_s("B"),blackList_l("blacklist"),
      predictionData(""),predictionData_s("T"),predictionData_l("test"),
      log(""),log_s("L"),log_l("log"),
      dataDelimiter(GENERAL_DEFAULT_DATA_DELIMITER),dataDelimiter_s("D"),dataDelimiter_l("data_delim"),
      headerDelimiter(GENERAL_DEFAULT_HEADER_DELIMITER),headerDelimiter_s("H"),headerDelimiter_l("head_delim"),
      pruneFeatures(GENERAL_DEFAULT_MIN_SAMPLES),pruneFeatures_s("X"),pruneFeatures_l("prune_features"),
      seed(GENERAL_DEFAULT_SEED),seed_s("S"),seed_l("seed"),
      // Forest Type
      forestType(GENERAL_DEFAULT_FOREST_TYPE),forestType_s("f"),forestType_l("forest_type"),
      // Random Forest related parameters
      // NOTE: Defaults will be loaded inside the constructor
      nTrees_s("n"),nTrees_l("ntrees"),
      mTry_s("m"),mTry_l("mtry"),
      nMaxLeaves_s("a"),nMaxLeaves_l("nmaxleaves"),
      nodeSize_s("s"),nodeSize_l("nodesize"),
      shrinkage_s("k"),shrinkage_l("shrinkage"),
      // Statistical test related parameters
      nPerms(ST_DEFAULT_N_PERMS),nPerms_s("p"),nPerms_l("nperms"),
      pValueThreshold(ST_DEFAULT_P_VALUE_THRESHOLD),pValueThreshold_s("t"),pValueThreshold_l("p_threshold"),
      importanceThreshold(ST_DEFAULT_IMPORTANCE_THRESHOLD),importanceThreshold_s("o"),importanceThreshold_l("i_threshold"),
      reportAllFeatures(ST_DEFAULT_REPORT_NONEXISTENT_FEATURES),reportAllFeatures_s("A"),reportAllFeatures_l("report_all_features") 
    { 
      
      parser_ = new ArgParse(argc,argv);

      if ( forestType == RF ) {
	setRFDefaults(); 
      } else if ( forestType == GBT ) {
	setGBTDefaults();
      } else if ( forestType == CART ) {
	cerr << "CART forest not yet fully specified!" << endl;
	exit(1);
      } 
    }
    
    General_options() {
      delete parser_;
    }

    void loadUserParams() {

      // If forest type is explicitly specified, update it
      if ( this->isSet(forestType_s,forestType_l) ) {
        string forestTypeAsStr("");
        parser_->getArgument<string>(forestType_s, forestType_l, forestTypeAsStr);
        if ( forestTypeAsStr == "RF" ) {
          forestType = RF;
        } else if ( forestTypeAsStr == "GBT" ) {
          forestType = GBT;
        } else if ( forestTypeAsStr == "CART" ) {
          forestType = CART;
        } else {
          cerr << "Invalid Forest Type!" << endl;
          exit(1);
        }
      }

      if ( forestType == RF ) {
        setRFDefaults();
      } else if ( forestType == GBT ) {
        setGBTDefaults();
      } else if ( forestType == CART ) {
        cerr << "CART forest not yet fully specified!" << endl;
        exit(1);
      }
            
      // EXPERIMENTAL PARAMETERS
      //parser.getFlag(recombinePerms_s, recombinePerms_l, isRecombiner);
      //if ( isRecombiner ) {
      parser_->getArgument<size_t>(recombinePerms_s,recombinePerms_l,recombinePerms);
      //}

      // I/O related and general parameters
      parser_->getFlag(printHelp_s, printHelp_l, printHelp);
      parser_->getFlag(isFilter_s,isFilter_l,isFilter);
      parser_->getArgument<size_t>(recombinePerms_s,recombinePerms_l,recombinePerms);
      parser_->getFlag(reportContrasts_s,reportContrasts_l,reportContrasts);
      parser_->getArgument<string>(input_s, input_l, input);
      parser_->getArgument<string>(targetStr_s, targetStr_l, targetStr);
      parser_->getArgument<string>(output_s, output_l, output);
      parser_->getArgument<string>(whiteList_s, whiteList_l, whiteList);
      parser_->getArgument<string>(blackList_s, blackList_l, blackList);
      parser_->getArgument<string>(predictionData_s, predictionData_l, predictionData);
      parser_->getArgument<string>(log_s,log_l,log);
      string dataDelimiter,headerDelimiter;
      parser_->getArgument<string>(dataDelimiter_s, dataDelimiter_l, dataDelimiter);
      parser_->getArgument<string>(headerDelimiter_s, headerDelimiter_l, headerDelimiter);
      parser_->getArgument<size_t>(pruneFeatures_s, pruneFeatures_l, pruneFeatures);
      stringstream ss(dataDelimiter);
      ss >> dataDelimiter;

      ss.clear();
      ss.str("");
      ss << headerDelimiter;
      ss >> headerDelimiter;

      parser_->getArgument<int>(seed_s, seed_l, seed);
      
      // If no seed was provided, generate one
      if ( seed == GENERAL_DEFAULT_SEED ) {
	seed = utils::generateSeed();
      }
    
      // Random Forest related parameters
      parser_->getArgument<size_t>( nTrees_s, nTrees_l, nTrees );
      parser_->getArgument<size_t>( mTry_s, mTry_l, mTry );
      parser_->getArgument<size_t>( nMaxLeaves_s, nMaxLeaves_l, nMaxLeaves );
      parser_->getArgument<size_t>( nodeSize_s, nodeSize_l, nodeSize );
      parser_->getArgument<num_t>(  shrinkage_s, shrinkage_l, shrinkage );

      // Statistical test parameters
      parser_->getArgument<size_t>(nPerms_s, nPerms_l, nPerms);
      parser_->getArgument<num_t>(pValueThreshold_s,  pValueThreshold_l,  pValueThreshold);
      parser_->getArgument<num_t>(importanceThreshold_s, importanceThreshold_l, importanceThreshold);
      parser_->getFlag(reportAllFeatures_s, reportAllFeatures_l, reportAllFeatures);
      //parser.getArgument<string>(contrastOutput_s, contrastOutput_l, contrastOutput);

    }

    void validate() {
      
      if ( nPerms > 1 && nPerms < 6 ) {
        cerr << "Use more than 5 permutations in statistical test!" << endl;
        exit(1);
      }
      
      if ( pValueThreshold < 0.0 || pValueThreshold > 1.0 ) {
        cerr << "P-value threshold in statistical test must be within 0...1" << endl;
        exit(1);
      }

      // Print help and exit if input file is not specified
      if ( input == "" ) {
	cerr << "Input file not specified" << endl;
	this->helpHint();
	exit(1);
      }
      
      // Print help and exit if target index is not specified
      if ( !this->isSet(recombinePerms_s,recombinePerms_l) && targetStr == "" ) {
	cerr << "target not specified" << endl;
	this->helpHint();
	exit(1);
      }
      
      if ( output == "" ) {
	cerr << "You forgot to specify an output file!" << endl;
	this->helpHint();
	exit(1);
      }
        
    }

    bool isSet(const string& shortOpt, const string& longOpt) const {

      bool ret = false;

      parser_->getFlag(shortOpt,longOpt,ret);
      return( ret );

    }

    void helpHint() {
      cout << endl;
      cout << "To get started, type -" << printHelp_s << " / --" << printHelp_l << endl;
    }


    void help() {

      cout << "GENERAL ARGUMENTS:" << endl;
      cout << " -" << input_s << " / --" << input_l << setw( maxWidth - input_l.size() )
           << " " << "Input data file (.afm or .arff) or predictor forest file (.sf)" << endl;
      cout << " -" << targetStr_s << " / --" << targetStr_l << setw( maxWidth - targetStr_l.size() )
           << " " << "Target, specified as integer or string that is to be matched with the content of input" << endl;
      cout << " -" << output_s << " / --" << output_l << setw( maxWidth - output_l.size() )
           << " " << "Output file" << endl;
      cout << " -" << predictionData_s << " / --" << predictionData_l << setw( maxWidth - predictionData_l.size() )
	   << " " << "[Prediction only] Test data file (.afm or .arff) for prediction" << endl;
      cout << " -" << log_s << " / --" << log_l << setw( maxWidth - log_l.size() )
           << " " << "Log output file" << endl;
      cout << " -" << whiteList_s << " / --" << whiteList_l << setw( maxWidth - whiteList_l.size() )
           << " " << "White list of features to be included in the input file(s)." << endl;
      cout << " -" << blackList_s << " / --" << blackList_l << setw( maxWidth - blackList_l.size() )
           << " " << "Black list of features to be excluded from the input file(s)." << endl;
      cout << " -" << dataDelimiter_s << " / --" << dataDelimiter_l << setw( maxWidth - dataDelimiter_l.size() )
           << " " << "[AFM only] Data delimiter (default \\t)" << endl;
      cout << " -" << headerDelimiter_s << " / --" << headerDelimiter_l << setw( maxWidth - headerDelimiter_l.size() )
           << " " << "[AFM only] Header delimiter that separates the N and C symbols in feature headers from the rest (default " << GENERAL_DEFAULT_HEADER_DELIMITER << ")" << endl;
      cout << " -" << pruneFeatures_s << " / --" << pruneFeatures_l << setw( maxWidth - pruneFeatures_l.size() )
           << " " << "Features with less than n ( default " << GENERAL_DEFAULT_MIN_SAMPLES << " ) samples will be removed" << endl;
      cout << " -" << seed_s << " / --" << seed_l << setw( maxWidth - seed_l.size() ) 
	   << " " << "Seed (positive integer) for the Mersenne Twister random number generator" << endl;
      cout << " -" << forestType_s << " / --" << forestType_l << setw( maxWidth - forestType_l.size() )
	   << " " << "Forest Type: RF (default), GBT, or CART" << endl;
      cout << endl;

      cout << "STOCHASTIC FOREST ARGUMENTS:" << endl;
      cout << " -" << nTrees_s << " / --" << nTrees_l << setw( maxWidth - nTrees_l.size() )
	   << " " << "Number of trees in the forest" << endl;
      cout << " -" << mTry_s << " / --" << mTry_l << setw( maxWidth - mTry_l.size() )
	   << " " << "Fraction of randomly drawn features per node split" << endl;
      cout << " -" << nMaxLeaves_s << " / --" << nMaxLeaves_l << setw( maxWidth - nMaxLeaves_l.size() )
	   << " " << "Maximum number of leaves per tree" << endl;
      cout << " -" << nodeSize_s << " / --" << nodeSize_l << setw( maxWidth - nodeSize_l.size() )
	   << " " << "Minimum number of train samples per node, affects tree depth" << endl;
      cout << " -" << shrinkage_s << " / --" << shrinkage_l << setw( maxWidth - shrinkage_l.size() )
	   << " " << "[GBT only] Shrinkage applied to evolving the residual" << endl;
      cout << endl;

      cout << "FILTER ARGUMENTS:" << endl;
      cout << " -" << isFilter_s << " / --" << isFilter_l << setw( maxWidth - isFilter_l.size() )
           << " " << "Set this flag if you want to perform feature selection (default OFF)" << endl;
      cout << " -" << nPerms_s << " / --" << nPerms_l << setw( maxWidth - nPerms_l.size() )
           << " " << "[Filter only] Number of permutations in statistical test (default " << ST_DEFAULT_N_PERMS << ")" << endl;
      cout << " -" << pValueThreshold_s << " / --" << pValueThreshold_l << setw( maxWidth - pValueThreshold_l.size() )
           << " " << "[Filter only] P-value threshold in statistical test (default " << ST_DEFAULT_P_VALUE_THRESHOLD << ")" << endl;
      cout << " -" << importanceThreshold_s << " / --" << importanceThreshold_l << setw( maxWidth - importanceThreshold_l.size() )
	   << " " << "[Filter only] Importance threshold in statistical test (default " << ST_DEFAULT_IMPORTANCE_THRESHOLD << ")" << endl;
      cout << " -" << reportAllFeatures_s << " / --" << reportAllFeatures_l << setw( maxWidth - reportAllFeatures_l.size() )
           << " " << "[Filter only] Set this flag if you want the association file to list all features regardless of statistical significance (default OFF)" << endl; 
      cout << " -" << reportContrasts_s << " / --" << reportContrasts_l << setw( maxWidth - reportContrasts_l.size() )
	   << " " << "[Filter only] Set this flag if you want to report contrasts in the output association file (default OFF)" << endl;
      cout << endl;

      cout << "EXAMPLES:" << endl << endl;
      
      cout << "Performing feature selection using 'target' as the target variable:" << endl
	   << "bin/rf-ace --filter -I data.arff -i target -O associations.tsv" << endl << endl;

      cout << "Performing feature selection with 50 permutations and p-value threshold of 0.001:" << endl
	   << "bin/rf-ace --filter -I data.arff -i 5 -p 50 -t 0.001 -O associations.tsv" << endl << endl;
      
      cout << "Building Random Forest with 1000 trees and mTry of 10 and predicting with test data:" << endl
	   << "bin/rf-ace -I data.arff -i target -T testdata.arff -n 1000 -m 10 -O predictions.tsv" << endl << endl;

      cout << "Building Random Forest predictor and saving it to a file for later use:" << endl
	   << "bin/rf-ace -I data.arff -i target -O rf_predictor.sf" << endl << endl;

      cout << "Loading Random Forest predictor from file and predicting with test data:" << endl
	   << "bin/rf-ace -I predictor.sf -T testdata.arff -O predictions.tsv" << endl << endl;
      
    }
    
    void setRFDefaults() {

      inBoxFraction         = 1.0;
      sampleWithReplacement = true;
      isRandomSplit         = true;
      useContrasts          = true;
      nTrees                = RF_DEFAULT_N_TREES;
      mTry                  = RF_DEFAULT_M_TRY;
      nMaxLeaves            = RF_DEFAULT_N_MAX_LEAVES;
      nodeSize              = RF_DEFAULT_NODE_SIZE;
      shrinkage             = RF_DEFAULT_SHRINKAGE;

    }

    void setGBTDefaults() {

      inBoxFraction         = 0.5;
      sampleWithReplacement = false;
      isRandomSplit         = false;
      useContrasts          = false;
      nTrees                = GBT_DEFAULT_N_TREES;
      mTry                  = GBT_DEFAULT_M_TRY;
      nMaxLeaves            = GBT_DEFAULT_N_MAX_LEAVES;
      nodeSize              = GBT_DEFAULT_NODE_SIZE;
      shrinkage             = GBT_DEFAULT_SHRINKAGE;

    }
    
  };
  
}

#endif
