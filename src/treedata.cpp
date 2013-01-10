#include "treedata.hpp"
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <ctime>

#include "math.hpp"
#include "utils.hpp"

using namespace std;

Feature::Feature():
  type_(Feature::Type::UNKNOWN) {  
}

Feature::Feature(Feature::Type newType, const string& newName, const size_t nSamples):
  type_(newType) {

  name = newName;

  if ( type_ == Feature::Type::NUM || type_ == Feature::Type::CAT ) {
    data.resize(nSamples);
    hashSet.clear();
  } else if ( type_ == Feature::Type::TXT ) {
    data.clear();
    hashSet.resize(nSamples);
  }
  
  mapping.clear();
  backMapping.clear();
  
}

void Feature::setNumSampleValue(const size_t sampleIdx, const num_t val) {
  assert( type_ == Feature::Type::NUM );
  data[sampleIdx] = val;
}

void Feature::setCatSampleValue(const size_t sampleIdx, const string& str) {
  assert( type_ == Feature::Type::CAT );

  if ( datadefs::isNAN_STR(str) ) {
    data[sampleIdx] = datadefs::NUM_NAN;
    return;
  }
  
  map<string,num_t>::iterator it(mapping.find(str));

  if ( it != mapping.end() ) {
    data[sampleIdx] = it->second;
  } else {
    num_t val = static_cast<num_t>(mapping.size());
    mapping[str] = val;
    backMapping[val] = str;
    data[sampleIdx] = val;
  }
}

void Feature::setTxtSampleValue(const size_t sampleIdx, const string& str) {
  assert( type_ == Feature::Type::TXT );
  if ( datadefs::isNAN_STR(str) ) {
    hashSet[sampleIdx] = utils::hashText("");
  } else {
    hashSet[sampleIdx] = utils::hashText(str);
  }
}

Feature::Feature(const vector<num_t>& newData, const string& newName):
  type_(Feature::Type::NUM) {

  data = newData;
  name = newName;
  mapping.clear();
  backMapping.clear();

}

Feature::Feature(const vector<string>& newStringData, const string& newName, bool doHash):
  type_( doHash ? Feature::Type::TXT : Feature::Type::CAT ) { 

  name = newName;

  if ( type_ == Feature::Type::CAT ) {
    
    utils::strv2catv(newStringData,data,mapping,backMapping);
    
  } else {
    
    size_t nSamples = newStringData.size();
    
    hashSet.resize(nSamples);
    
    for ( size_t i = 0; i < nSamples; ++i ) {
      
      hashSet[i] = utils::hashText(newStringData[i]);
      
    }
    
  }

}

Feature::~Feature() { }

bool Feature::isNumerical() const { 
  return( type_ == Feature::Type::NUM ? true : false ); 
}

bool Feature::isCategorical() const { 
  return( type_ == Feature::Type::CAT ? true : false ); 
}

bool Feature::isTextual() const { 
  return( type_ == Feature::Type::TXT ? true : false );
}

uint32_t Feature::getHash(const size_t sampleIdx, const size_t integer) const {

  assert( type_ == Feature::Type::TXT );
  
  size_t pos = integer % this->hashSet[sampleIdx].size();

  unordered_set<uint32_t>::const_iterator it(this->hashSet[sampleIdx].begin());
  for ( size_t i = 0; i < pos; ++i ) {
    it++;
  }
  
  return(*it);
  
}

bool Feature::hasHash(const size_t sampleIdx, const uint32_t hashIdx) const {

  return( this->hashSet[sampleIdx].find(hashIdx) != this->hashSet[sampleIdx].end() );
  
}

num_t Feature::entropy() const {

  size_t nSamples = hashSet.size();

  unordered_map<uint32_t,size_t> visited_keys;

  for ( size_t i = 0; i < nSamples; ++i ) {
    for ( unordered_set<uint32_t>::const_iterator it(hashSet[i].begin()); it != hashSet[i].end(); ++it ) {
      visited_keys[*it]++;
    }
  }
  
  unordered_map<uint32_t,size_t>::const_iterator it(visited_keys.begin());

  num_t entropy = 0.0;

  for ( ; it != visited_keys.end(); ++it ) {
    num_t f = static_cast<num_t>(it->second) / static_cast<num_t>(nSamples);
    if ( fabs(f) > 1e-5 && fabs(1-f) > 1e-5 ) {
      entropy -= f * log(f) + (1-f)*log(1-f);
    }
  }

  return(entropy);

}

uint32_t Treedata::getHash(const size_t featureIdx, const size_t sampleIdx, const size_t integer) const {
  return( features_[featureIdx].getHash(sampleIdx,integer) );
}

bool Treedata::hasHash(const size_t featureIdx, const size_t sampleIdx, const uint32_t hashIdx) const {

  return( features_[featureIdx].hasHash(sampleIdx,hashIdx) );

}

num_t Treedata::getFeatureEntropy(const size_t featureIdx) const {

  return( features_[featureIdx].entropy() );

}

Treedata::Treedata(const vector<Feature>& features, const bool useContrasts, const vector<string>& sampleHeaders):
  useContrasts_(useContrasts),
  features_(features),
  sampleHeaders_(sampleHeaders) {

  size_t nFeatures = features_.size();

  assert( nFeatures > 0 );

  // If we have contrasts, there would be 2*nFeatures, in which case
  // 4*nFeatures results in a reasonable max load factor of 0.5
  name2idx_.rehash(4*nFeatures);

  size_t nSamples = features_[0].data.size();

  for ( size_t featureIdx = 0; featureIdx < nFeatures; ++featureIdx ) {

    assert( features_[featureIdx].data.size() == nSamples );
    name2idx_[ features_[featureIdx].name ] = featureIdx;
  }

  assert( nSamples > 0 );

  if ( sampleHeaders_.size() == 0 ) {
    sampleHeaders_.resize(nSamples,"NO_SAMPLE_ID");
  } 

  assert( sampleHeaders_.size() == nSamples );

  if ( useContrasts_ ) {
    this->createContrasts(); // Doubles matrix size
    // this->permuteContrasts(random);
  }
  
}


/**
   Reads a data file into a Treedata object. The data file can be either AFM or ARFF
   NOTE: dataDelimiter and headerDelimiter are used only when the format is AFM, for 
   ARFF default delimiter (comma) is used 
*/
Treedata::Treedata(string fileName, const char dataDelimiter, const char headerDelimiter, const bool useContrasts):
  useContrasts_(useContrasts) {

  //Initialize stream to read from file
  ifstream featurestream;
  featurestream.open(fileName.c_str());
  if ( !featurestream.good() ) {
    cerr << "Failed to open file '" << fileName << "' for reading. Make sure the file exists. Quitting..." << endl;
    exit(1);
  }

  // Interprets file type from the content of the file
  FileType fileType = UNKNOWN;
  Treedata::readFileType(fileName,fileType);

  // Reads raw data matrix from the input file
  // NOTE: should be optimized to scale for large data sets
  vector<vector<string> > rawMatrix;
  vector<string> featureHeaders;
  vector<Feature::Type> featureTypes;
  if(fileType == AFM) {
    
    // Reads from the AFM stream and inserts 
    // data to the following arguments
    Treedata::readAFM(featurestream,
		      rawMatrix,
		      featureHeaders,
		      sampleHeaders_,
		      featureTypes,
		      dataDelimiter,
		      headerDelimiter);
    
  } else if(fileType == ARFF) {
    
    // Reads from the ARFF stream and inserts 
    // data to the following arguments
    Treedata::readARFF(featurestream,
		       rawMatrix,
		       featureHeaders,
		       featureTypes);

    // ARFF doesn't contain sample headers 
    sampleHeaders_.clear();
    sampleHeaders_.resize(rawMatrix[0].size(),"NO_SAMPLE_ID");
    
  } else {
    
    // By default, reads from the AFM stream and inserts
    // data to the following arguments
    Treedata::readAFM(featurestream,
		      rawMatrix,
		      featureHeaders,
		      sampleHeaders_,
		      featureTypes,
		      dataDelimiter,
		      headerDelimiter);
    
  }      

  // Extract the number of features 
  size_t nFeatures = featureHeaders.size();

  features_.resize(nFeatures);

  // If we have contrasts, there would be 2*nFeatures, in which case
  // 4*nFeatures results in a reasonable max load factor of 0.5
  name2idx_.rehash(4*nFeatures);

  // Start reading data to the final container "features_"
  for(size_t i = 0; i < nFeatures; ++i) {
    
    // We require that no two features have identical header
    if( name2idx_.find(featureHeaders[i]) != name2idx_.end() ) {
      cerr << "Duplicate feature header '" << featureHeaders[i] << "' found!" << endl;
      exit(1);
    }

    // Map the i'th feature header to integer i
    // NOTE: could be replaced with a hash table
    name2idx_[featureHeaders[i]] = i;

    if ( featureTypes[i] == Feature::Type::NUM ) {

      vector<num_t> data;

      // If type is numerical, read the raw data as numbers
      utils::strv2numv(rawMatrix[i],data);

      features_[i] = Feature(data,featureHeaders[i]);

    } else if ( featureTypes[i] == Feature::Type::CAT ) {

      features_[i] = Feature(rawMatrix[i],featureHeaders[i]);

    } else if ( featureTypes[i] == Feature::Type::TXT ) {

      bool doHash = true;
      features_[i] = Feature(rawMatrix[i],featureHeaders[i],doHash);

      /*
	for ( size_t j = 0; j < features_[i].hashSet.size(); ++j ) {
	
	utils::write(cout,rawMatrix[i].begin(),rawMatrix[i].end());
	cout << " ==> " << flush;
	utils::write(cout,features_[i].hashSet[j].begin(),features_[i].hashSet[j].end());
	cout << endl;
	}
      */

    } else {
      cerr << "ERROR: unknown feature type for feature with header '" << featureHeaders[i] << "'" << endl;
      exit(1);
    }

  } 
 
  if ( useContrasts_ ) {
    this->createContrasts(); // Doubles matrix size
  }
  
}

Treedata::~Treedata() {
  /* Empty destructor */
}

void Treedata::createContrasts() {

  // Resize the feature data container to fit the
  // original AND contrast features ( so 2*nFeatures )
  size_t nFeatures = features_.size();
  features_.resize(2*nFeatures);

  // Generate contrast features
  for(size_t i = nFeatures; i < 2*nFeatures; ++i) {
    features_[i] = features_[ i - nFeatures ];
    features_[i].name.append("_CONTRAST");
    name2idx_[ features_[i].name ] = i;
  }

}

void Treedata::readFileType(string& fileName, FileType& fileType) {

  stringstream ss(fileName);
  string suffix = "";
  while(getline(ss,suffix,'.')) {}
  //datadefs::toupper(suffix);

  if(suffix == "AFM" || suffix == "afm") {
    fileType = AFM;
  } else if(suffix == "ARFF" || suffix == "arff") {
    fileType = ARFF;
  } else {
    fileType = UNKNOWN;
  }

}

void Treedata::readAFM(ifstream& featurestream, 
		       vector<vector<string> >& rawMatrix, 
		       vector<string>& featureHeaders, 
		       vector<string>& sampleHeaders,
		       vector<Feature::Type>& featureTypes,
		       const char dataDelimiter,
		       const char headerDelimiter) {

  string field;
  string row;

  assert( headerDelimiter != ' ' );

  rawMatrix.clear();
  featureHeaders.clear();
  sampleHeaders.clear();
  featureTypes.clear();

  //Remove upper left element from the matrix as useless
  getline(featurestream,field,dataDelimiter);

  //Next read the first row, which should contain the column headers
  getline(featurestream,row);
  stringstream ss( utils::chomp(row) );
  bool isFeaturesAsRows = true;
  vector<string> columnHeaders;
  while ( getline(ss,field,dataDelimiter) ) {

    // If at least one of the column headers is a valid feature header, we assume features are stored as columns
    if ( isFeaturesAsRows && isValidFeatureHeader(field,headerDelimiter) ) {
      isFeaturesAsRows = false;
    }
    columnHeaders.push_back(field);
  }

  // We should have reached the end of file. NOTE: failbit is set since the last element read did not end at '\t'
  assert( ss.eof() );
  //assert( !ss.fail() );

  size_t nColumns = columnHeaders.size();

  vector<string> rowHeaders;
  //vector<string> sampleHeaders; // THIS WILL BE DEFINED AS ONE OF THE INPUT ARGUMENTS

  //Go through the rest of the rows
  size_t iter = 0;
  while ( getline(featurestream,row) ) {

    row = utils::chomp(row);

    //Read row from the stream
    ss.clear();
    ss.str("");

    //Read the string back to a stream
    ss << row;

    //Read the next row header from the stream
    getline(ss,field,dataDelimiter);
    rowHeaders.push_back(field);

    vector<string> rawVector(nColumns);
    for(size_t i = 0; i < nColumns; ++i) {
      getline(ss,rawVector[i],dataDelimiter);
      rawVector[i] = utils::trim(rawVector[i]);
    }
    if ( ss.fail() || !ss.eof()) {
      cerr << "ERROR: incorrectly formatted line " << iter << ". Make sure the line contains expected number of fields (" << nColumns << ")" << endl;
      exit(1);
    }
    rawMatrix.push_back(rawVector);
    ++iter;
  }

  //If the data is row-formatted...
  if(isFeaturesAsRows) {
    //cout << "AFM orientation: features as rows" << endl;

    //... and feature headers are row headers
    featureHeaders = rowHeaders;
    sampleHeaders = columnHeaders;

  } else {

    //cout << "AFM orientation: features as columns" << endl;
      
    Treedata::transpose<string>(rawMatrix);
      
    //... and feature headers are row headers
    featureHeaders = columnHeaders;
    sampleHeaders = rowHeaders;
      
  }

  size_t nFeatures = featureHeaders.size();
  featureTypes.resize(nFeatures);
  for(size_t i = 0; i < nFeatures; ++i) {
    if(Treedata::isValidNumericalHeader(featureHeaders[i],headerDelimiter)) {
      featureTypes[i] = Feature::Type::NUM;
    } else if ( this->isValidCategoricalHeader(featureHeaders[i],headerDelimiter) ) {
      featureTypes[i] = Feature::Type::CAT;
    } else if ( this->isValidTextHeader(featureHeaders[i],headerDelimiter) ) {
      featureTypes[i] = Feature::Type::TXT;
    } else {
      cerr << "ERROR: unknown feature type with feature header '" << featureHeaders[i] << "'" << endl;
      exit(1);
    }
  }
}

void Treedata::readARFF(ifstream& featurestream, vector<vector<string> >& rawMatrix, vector<string>& featureHeaders, vector<Feature::Type>& featureTypes) {

  string row;

  bool hasRelation = false;
  bool hasData = false;

  size_t nFeatures = 0;
  //TODO: add Treedata::clearData(...)
  rawMatrix.clear();
  featureHeaders.clear();
  featureTypes.clear();
  
  //Read one line from the ARFF file
  while ( getline(featurestream,row) ) {

    row = utils::chomp(row);

    //Comment lines and empty lines are omitted
    if(row[0] == '%' || row == "") {
      continue;
    }
    
    string rowU = datadefs::toUpperCase(row);
    
    //Read relation
    if(!hasRelation && rowU.compare(0,9,"@RELATION") == 0) {
      hasRelation = true;
      //cout << "found relation header: " << row << endl;
    } else if ( rowU.compare(0,10,"@ATTRIBUTE") == 0) {    //Read attribute
      string attributeName = "";
      bool isNumerical;
      ++nFeatures;
      //cout << "found attribute header: " << row << endl;
      Treedata::parseARFFattribute(row,attributeName,isNumerical);
      featureHeaders.push_back(attributeName);
      if ( isNumerical ) {
	featureTypes.push_back(Feature::Type::NUM);
      } else {
	featureTypes.push_back(Feature::Type::CAT);
      }
      
    } else if(!hasData && rowU.compare(0,5,"@DATA") == 0) {    //Read data header
      
      hasData = true;
      break;
      //cout << "found data header:" << row << endl;
    } else {      //If none of the earlier branches matched, we have a problem
      cerr << "incorrectly formatted ARFF row '" << row << "'" << endl;
      assert(false);
    }
    
  }

  if ( !hasData ) {
    cerr << "Treedata::readARFF() -- could not find @data/@DATA identifier" << endl;
    exit(1);
  }

  if ( !hasRelation ) {
    cerr << "Treedata::readARFF() -- could not find @relation/@RELATION identifier" << endl;
    exit(1);
  }
    
  //Read data row-by-row
  while ( getline(featurestream,row) ) {
    
    row = utils::chomp(row);

    //Comment lines and empty lines are omitted
    if ( row == "" ) {
      continue;
    }
    
    // One sample is stored as row in the matrix
    rawMatrix.push_back( utils::split(row,',') );
    
    if ( rawMatrix.back().size() != nFeatures ) {
      cerr << "Treedata::readARFF() -- sample contains incorrect number of features" << endl;
      exit(1);
    }
    
  }
  
  this->transpose<string>(rawMatrix);
  
}

void Treedata::parseARFFattribute(const string& str, string& attributeName, bool& isFeatureNumerical) {

  stringstream ss(str);
  string attributeHeader = "";
  attributeName = "";
  string attributeType = "";

  getline(ss,attributeHeader,' ');
  getline(ss,attributeName,' ');
  getline(ss,attributeType);

  //string prefix;
  if(datadefs::toUpperCase(attributeType) == "NUMERIC" ||
     datadefs::toUpperCase(attributeType) == "REAL" ) {
    isFeatureNumerical = true;
  } else {
    isFeatureNumerical = false;
  }
  //prefix.append(attributeName);
  //attributeName = prefix;
}

bool Treedata::isValidNumericalHeader(const string& str, const char headerDelimiter) {
  if ( str.size() > 1 ) {
    return( str[0] == 'N' && str[1] == headerDelimiter );
  } else {
    return( false );
  }
}

bool Treedata::isValidCategoricalHeader(const string& str, const char headerDelimiter) {
  if ( str.size() > 1 ) {
    return( ( str[0] == 'C' || str[0] == 'B' ) && str[1] == headerDelimiter );
  } else {
    return(false);
  }
}

bool Treedata::isValidTextHeader(const string& str, const char headerDelimiter) {
  if ( str.size() > 1 ) {
    return( str[0] == 'T' && str[1] == headerDelimiter );
  } else {
    return(false);
  }
}

bool Treedata::isValidFeatureHeader(const string& str, const char headerDelimiter) {
  return( isValidNumericalHeader(str,headerDelimiter) || isValidCategoricalHeader(str,headerDelimiter) || isValidTextHeader(str,headerDelimiter) );
}

size_t Treedata::nFeatures() const {
  return( useContrasts_ ? features_.size() / 2 : features_.size() );
}

size_t Treedata::nSamples() const {
  return( sampleHeaders_.size() );
}

// WILL BECOME DEPRECATED
num_t Treedata::pearsonCorrelation(size_t featureIdx1, size_t featureIdx2) {
  
  vector<size_t> sampleIcs = utils::range( this->nSamples() );

  vector<size_t> missingIcs;

  this->separateMissingSamples(featureIdx1,sampleIcs,missingIcs);
  this->separateMissingSamples(featureIdx2,sampleIcs,missingIcs);

  vector<num_t> featureData1 = this->getFeatureData(featureIdx1,sampleIcs);
  vector<num_t> featureData2 = this->getFeatureData(featureIdx2,sampleIcs);

  return( math::pearsonCorrelation(featureData1,featureData2) );

}

size_t Treedata::getFeatureIdx(const string& featureName) const {
  
  unordered_map<string,size_t>::const_iterator it( name2idx_.find(featureName) );
  
  // If the feature does not exist, return "end", which is a value that 
  // points to outside the range of valid indices
  if ( it == name2idx_.end() ) {
    return( this->end() );
  }
  return( it->second );
}

string Treedata::getFeatureName(const size_t featureIdx) const {
  return( features_.at(featureIdx).name );
}

string Treedata::getSampleName(const size_t sampleIdx) {
  return( sampleHeaders_.at(sampleIdx) );
}

vector<num_t> Treedata::getFeatureWeights() const {

  vector<num_t> weights(this->nFeatures(),1.0);

  for ( size_t i = 0; i < this->nFeatures(); ++i ) {
    if ( this->isFeatureTextual(i) ) {
      num_t entropy = this->getFeatureEntropy(i);
      cout << "Feature '" << this->getFeatureName(i) << "' is textual and has sqrt(entropy) " << sqrtf(entropy) << endl;
      weights[i] = sqrtf(entropy);
    }
  }

  return(weights);

}

void Treedata::print() {
  cout << "Printing feature matrix (missing values encoded to " << datadefs::NUM_NAN << "):" << endl;
  for(size_t j = 0; j < Treedata::nSamples(); ++j) {
    cout << '\t' << "foo";
  }
  cout << endl;
  for(size_t i = 0; i < Treedata::nFeatures(); ++i) {
    cout << i << ':' << features_[i].name << ':';
    for(size_t j = 0; j < Treedata::nSamples(); ++j) {
      cout << '\t' << features_[i].data[j];
    }
    cout << endl;
  }
}


void Treedata::print(const size_t featureIdx) {
  cout << "Print " << features_[featureIdx].name << ":";
  for(size_t i = 0; i < Treedata::nSamples(); ++i) {
    cout << " " << features_[featureIdx].data[i];
  }
  cout << endl;
}


void Treedata::permuteContrasts(distributions::Random* random) {

  size_t nFeatures = this->nFeatures();
  size_t nSamples = this->nSamples();

  for ( size_t i = nFeatures; i < 2*nFeatures; ++i ) {
    
    if ( this->isFeatureTextual(i) ) { continue; }

    vector<size_t> sampleIcs = utils::range( nSamples );
    vector<size_t> missingIcs;
    
    this->separateMissingSamples(i,sampleIcs,missingIcs);

    vector<num_t> filteredData = this->getFeatureData(i,sampleIcs);
    
    utils::permute(filteredData,random);

    for ( size_t j = 0; j < sampleIcs.size(); ++j ) {
      features_[i].data[ sampleIcs[j] ] = filteredData[j];
    }

  }

}

bool Treedata::isFeatureNumerical(const size_t featureIdx) const {
  return( features_[featureIdx].isNumerical() );
}

bool Treedata::isFeatureCategorical(const size_t featureIdx) const {
  return( features_[featureIdx].isCategorical() );
}

bool Treedata::isFeatureTextual(const size_t featureIdx) const {
  return( features_[featureIdx].isTextual() );
}

size_t Treedata::nRealSamples(const size_t featureIdx) { 
  
  size_t nRealSamples;
  datadefs::countRealValues( features_[featureIdx].data, nRealSamples );
  return( nRealSamples );

}

size_t Treedata::nRealSamples(const size_t featureIdx1, const size_t featureIdx2) {

  size_t nRealSamples = 0;
  for( size_t i = 0; i < Treedata::nSamples(); ++i ) {
    if( !datadefs::isNAN( features_[featureIdx1].data[i] ) && !datadefs::isNAN( features_[featureIdx2].data[i] ) ) {
      ++nRealSamples;
    }
  }
  return( nRealSamples );
}

size_t Treedata::nCategories(const size_t featureIdx) {
  return( features_[featureIdx].mapping.size() );
}

size_t Treedata::nMaxCategories() {

  size_t ret = 0;
  for( size_t i = 0; i < Treedata::nFeatures(); ++i ) {
    if( ret < features_[i].mapping.size() ) {
      ret = features_[i].mapping.size();
    }
  }
  
  return( ret ); 

}

vector<string> Treedata::categories(const size_t featureIdx) {
  
  vector<string> categories;

  if( this->isFeatureNumerical(featureIdx) ) {
    return( categories );
  }
 
  for ( map<num_t,string>::const_iterator it( features_[featureIdx].backMapping.begin() ) ; it != features_[featureIdx].backMapping.end(); ++it ) {
    categories.push_back(it->second);
  }

  return( categories );

}

template <typename T> void Treedata::transpose(vector<vector<T> >& mat) {

  vector<vector<T> > foo = mat;

  size_t ncols = mat.size();
  size_t nrows = mat[0].size();

  mat.resize(nrows);
  for(size_t i = 0; i < nrows; ++i) {
    mat[i].resize(ncols);
  }

  for(size_t i = 0; i < nrows; ++i) {
    for(size_t j = 0; j < ncols; ++j) {
      mat[i][j] = foo[j][i];
    }
  }
}

void Treedata::bootstrapFromRealSamples(distributions::Random* random,
					const bool withReplacement, 
                                        const num_t sampleSize, 
                                        const size_t featureIdx, 
                                        vector<size_t>& ics, 
                                        vector<size_t>& oobIcs) {
    
  //Check that the sampling parameters are appropriate
  assert(sampleSize > 0.0);
  if(!withReplacement && sampleSize > 1.0) {
    cerr << "Treedata: when sampling without replacement, sample size must be less or equal to 100% (sampleSize <= 1.0)" << endl;
    exit(1);
  }

  //First we collect all indices that correspond to real samples
  vector<size_t> allIcs;
  for(size_t i = 0; i < this->nSamples(); ++i) {
    if(!datadefs::isNAN(features_[featureIdx].data[i])) {
      allIcs.push_back(i);
    }
  }
  
  //Extract the number of real samples, and see how many samples do we have to collect
  size_t nRealSamples = allIcs.size();
  size_t nSamples = static_cast<size_t>( floor( sampleSize * nRealSamples ) );
  ics.resize(nSamples);
  
  //If sampled with replacement...
  if(withReplacement) {
    //Draw nSamples random integers from range of allIcs
    for(size_t sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx) {
      ics[sampleIdx] = allIcs[ random->integer() % nRealSamples ];
    }
  } else {  //If sampled without replacement...
    vector<size_t> foo = utils::range(nRealSamples);
    utils::permute(foo,random);
    for(size_t i = 0; i < nSamples; ++i) {
      ics[i] = allIcs[foo[i]];
    }
  }

  sort(ics.begin(),ics.end());

  if(nSamples < nRealSamples) {
    oobIcs.resize(nRealSamples);
  } else {
    oobIcs.resize(nSamples);
  }

  //Then, as we now have the sample stored in ics, we'll check which of the samples, from allIcs, are not contained in ics and store them in oobIcs instead
  vector<size_t>::iterator it = set_difference(allIcs.begin(),allIcs.end(),ics.begin(),ics.end(),oobIcs.begin());
  size_t nOob = distance(oobIcs.begin(),it);
  oobIcs.resize(nOob);
  //cout << "nOob=" << nOob << endl;
}


vector<num_t> Treedata::getFeatureData(size_t featureIdx) {
  
  vector<num_t> data( features_[featureIdx].data.size() );

  for(size_t i = 0; i < Treedata::nSamples(); ++i) {
    data[i] = features_[featureIdx].data[i];
  }

  return( data );
}


num_t Treedata::getFeatureData(size_t featureIdx, const size_t sampleIdx) {

  num_t data = features_[featureIdx].data[sampleIdx];

  return( data ); 
}

vector<num_t> Treedata::getFeatureData(size_t featureIdx, const vector<size_t>& sampleIcs) {
  
  vector<num_t> data(sampleIcs.size());
  
  for(size_t i = 0; i < sampleIcs.size(); ++i) {
    data[i] = features_[featureIdx].data[sampleIcs[i]];
  }

  return( data );

}
  
void Treedata::separateMissingSamples(const size_t featureIdx,
				      vector<size_t>& sampleIcs,
				      vector<size_t>& missingIcs) {
  
  size_t nReal = 0;
  size_t nMissing = 0;

  missingIcs.resize(sampleIcs.size());

  vector<size_t>::const_iterator it(sampleIcs.begin());

  if ( this->isFeatureTextual(featureIdx) ) {
    for ( ; it != sampleIcs.end(); ++it ) {
      if ( features_[featureIdx].hashSet[*it].size() > 0 ) {
	sampleIcs[nReal++] = *it;
      } else {
	missingIcs[nMissing++] = *it;
      }
    } 
  } else {
    for ( ; it != sampleIcs.end(); ++it ) {
      if ( ! datadefs::isNAN( features_[featureIdx].data[*it] ) ) {
	sampleIcs[nReal++] = *it;
      } else {
	missingIcs[nMissing++] = *it;
      }
    }
  }

  sampleIcs.resize(nReal);
  missingIcs.resize(nMissing);

}


// !! Correctness, Inadequate Abstraction: kill this method with fire. Refactor, REFACTOR, _*REFACTOR*_.
num_t Treedata::numericalFeatureSplit(const size_t targetIdx,
				      const size_t featureIdx,
				      const size_t minSamples,
				      vector<size_t>& sampleIcs_left,
				      vector<size_t>& sampleIcs_right,
				      vector<size_t>& sampleIcs_missing,
				      num_t& splitValue) {

  num_t DI_best = 0.0;

  sampleIcs_left.clear();

  this->separateMissingSamples(featureIdx,sampleIcs_right,sampleIcs_missing);

  vector<num_t> fv = this->getFeatureData(featureIdx,sampleIcs_right);
  vector<num_t> tv = this->getFeatureData(targetIdx,sampleIcs_right);

  vector<size_t> sortIcs = utils::range(sampleIcs_right.size());
  utils::sortDataAndMakeRef(true,fv,sortIcs);
  utils::sortFromRef(tv,sortIcs);
  utils::sortFromRef(sampleIcs_right,sortIcs);

  size_t n_tot = fv.size();
  size_t n_right = n_tot;
  size_t n_left = 0;

  if(n_tot < 2 * minSamples) {
    DI_best = 0.0;
    return( DI_best );
  }

  size_t bestSplitIdx = datadefs::MAX_IDX;

  //If the target is numerical, we use the incremental squared error formula
  if ( this->isFeatureNumerical(targetIdx) ) {

    DI_best = utils::numericalFeatureSplitsNumericalTarget(tv,fv,minSamples,bestSplitIdx);

  } else { // Otherwise we use the iterative gini index formula to update impurity scores while we traverse "right"

    DI_best = utils::numericalFeatureSplitsCategoricalTarget(tv,fv,minSamples,bestSplitIdx);

  }


  if ( bestSplitIdx == datadefs::MAX_IDX ) {
    DI_best = 0.0;
    return( DI_best );
  }

  splitValue = fv[bestSplitIdx];
  n_left = bestSplitIdx + 1;
  sampleIcs_left.resize(n_left);

  for(size_t i = 0; i < n_left; ++i) {
    sampleIcs_left[i] = sampleIcs_right[i];
  }
  sampleIcs_right.erase(sampleIcs_right.begin(),sampleIcs_right.begin() + n_left);
  n_right = sampleIcs_right.size();

  assert(n_left + n_right == n_tot);

  //cout << "N : " << n_left << " <-> " << n_right << " : fitness " << splitFitness << endl;

  return( DI_best );
  
}

// !! Inadequate Abstraction: Refactor me.
num_t Treedata::categoricalFeatureSplit(const size_t targetIdx,
					const size_t featureIdx,
					const size_t minSamples,
					vector<size_t>& sampleIcs_left,
					vector<size_t>& sampleIcs_right,
					vector<size_t>& sampleIcs_missing,
					set<num_t>& splitValues_left,
					set<num_t>& splitValues_right) {

  num_t DI_best = 0.0;

  sampleIcs_left.clear();

  this->separateMissingSamples(featureIdx,sampleIcs_right,sampleIcs_missing);

  vector<num_t> fv = this->getFeatureData(featureIdx,sampleIcs_right);
  vector<num_t> tv = this->getFeatureData(targetIdx,sampleIcs_right);

  size_t n_tot = fv.size();

  if(n_tot < 2 * minSamples) {
    DI_best = 0.0;
    return( DI_best );
  }
  
  map<num_t,vector<size_t> > fmap_right;
  map<num_t,vector<size_t> > fmap_left;

  if ( this->isFeatureNumerical(targetIdx) ) {

    DI_best = utils::categoricalFeatureSplitsNumericalTarget(tv,fv,minSamples,fmap_left,fmap_right);

  } else {

    DI_best = utils::categoricalFeatureSplitsCategoricalTarget(tv,fv,minSamples,fmap_left,fmap_right);

  }

  if ( fabs(DI_best) < datadefs::EPS ) {
    return(DI_best);
  }

  // Assign samples and categories on the left. First store the original sample indices
  vector<size_t> sampleIcs = sampleIcs_right;

  // Then populate the left side (sample indices and split values)
  sampleIcs_left.resize(n_tot);
  splitValues_left.clear();
  size_t iter = 0;
  for ( map<num_t,vector<size_t> >::const_iterator it(fmap_left.begin()); it != fmap_left.end(); ++it ) {
    for ( size_t i = 0; i < it->second.size(); ++i ) {
      sampleIcs_left[iter] = sampleIcs[it->second[i]];
      ++iter;
    }
    splitValues_left.insert( it->first );
  }
  sampleIcs_left.resize(iter);
  //assert( iter == n_left);
  assert( splitValues_left.size() == fmap_left.size() );

  // Last populate the right side (sample indices and split values)
  sampleIcs_right.resize(n_tot);
  splitValues_right.clear();
  iter = 0;
  for ( map<num_t,vector<size_t> >::const_iterator it(fmap_right.begin()); it != fmap_right.end(); ++it ) {
    for ( size_t i = 0; i < it->second.size(); ++i ) {
      sampleIcs_right[iter] = sampleIcs[it->second[i]];
      ++iter;
    }
    splitValues_right.insert( it->first );
  }
  sampleIcs_right.resize(iter);
  //assert( iter == n_right );
  assert( splitValues_right.size() == fmap_right.size() );

  return( DI_best );

}

num_t Treedata::textualFeatureSplit(const size_t targetIdx,
				    const size_t featureIdx,
				    const uint32_t hashIdx,
				    const size_t minSamples,
				    vector<size_t>& sampleIcs_left,
				    vector<size_t>& sampleIcs_right,
				    vector<size_t>& sampleIcs_missing) {


  assert(features_[featureIdx].isTextual());

  size_t n_left = 0;
  size_t n_right = 0;
  size_t n_tot = sampleIcs_right.size();

  sampleIcs_missing.clear();

  sampleIcs_left.resize(n_tot);

  num_t DI_best = 0.0;

  if ( this->isFeatureNumerical(targetIdx) ) {
  
    num_t mu_left = 0.0;
    num_t mu_right = 0.0;
    num_t mu_tot = 0.0;

    for ( size_t i = 0; i < n_tot; ++i ) {
      unordered_set<uint32_t>& hs = features_[featureIdx].hashSet[sampleIcs_right[i]];
      num_t x = features_[targetIdx].data[sampleIcs_right[i]];
      if ( hs.find(hashIdx) != hs.end() ) {
	sampleIcs_left[n_left++] = sampleIcs_right[i];
	mu_left += ( x - mu_left ) / n_left;
      } else {
	sampleIcs_right[n_right++] = sampleIcs_right[i];
	mu_right += ( x - mu_right ) / n_right;
      }
      mu_tot += x / n_tot;
    }
    
    DI_best = math::deltaImpurity_regr(mu_tot,n_tot,mu_left,n_left,mu_right,n_right);

  } else {

    map<num_t,size_t> freq_left,freq_right,freq_tot;

    size_t sf_left = 0;
    size_t sf_right = 0;
    size_t sf_tot = 0;

    for ( size_t i = 0; i < sampleIcs_right.size(); ++i ) {
      unordered_set<uint32_t>& hs = features_[featureIdx].hashSet[sampleIcs_right[i]];
      num_t x = features_[targetIdx].data[sampleIcs_right[i]];
      if ( hs.find(hashIdx) != hs.end() ) {
        sampleIcs_left[n_left++] = sampleIcs_right[i];
	math::incrementSquaredFrequency(x,freq_left,sf_left);
      } else {
        sampleIcs_right[n_right++] = sampleIcs_right[i];
	math::incrementSquaredFrequency(x,freq_right,sf_right);
      }
      math::incrementSquaredFrequency(x,freq_tot,sf_tot);
    }

    DI_best = math::deltaImpurity_class(sf_tot,n_tot,sf_left,n_left,sf_right,n_right);

  }

  assert(n_tot == n_left + n_right);

  if ( n_left < minSamples || n_right < minSamples ) {
    return(0.0);
  }

  sampleIcs_left.resize(n_left);
  sampleIcs_right.resize(n_right);
  
  return(DI_best);
  
}


string Treedata::getRawFeatureData(const size_t featureIdx, const size_t sampleIdx) {

  num_t data = features_[featureIdx].data[sampleIdx];

  return( this->getRawFeatureData(featureIdx,data) );
    
}

string Treedata::getRawFeatureData(const size_t featureIdx, const num_t data) {
  
  // If the input data is NaN, we return NaN as string 
  if ( datadefs::isNAN(data) ) {
    return( datadefs::STR_NAN );
  }
  
  // If input feature is numerical, we just represent the numeric value as string
  if ( features_[featureIdx].isNumerical() ) {
    return( utils::num2str(data) );
  } else {
    
    if ( features_[featureIdx].backMapping.find(data) == features_[featureIdx].backMapping.end() ) {
      cerr << "Treedata::getRawFeatureData() -- unknown value to get" << endl;
      exit(1);
    }
    
    return( features_[featureIdx].backMapping[data] );
  }
  
}

vector<string> Treedata::getRawFeatureData(const size_t featureIdx) {
  
  vector<string> rawData( sampleHeaders_.size() );

  for ( size_t i = 0; i < rawData.size(); ++i ) {
    rawData[i] = this->getRawFeatureData(featureIdx,i);
  }

  return( rawData );

}

void Treedata::replaceFeatureData(const size_t featureIdx, const vector<num_t>& featureData) {

  if(featureData.size() != features_[featureIdx].data.size() ) {
    cerr << "Treedata::replaceFeatureData(num_t) -- data dimension mismatch" << endl;
    exit(1);
  }

  features_[featureIdx] = Feature(featureData,features_[featureIdx].name);

}

void Treedata::replaceFeatureData(const size_t featureIdx, const vector<string>& rawFeatureData) {

  if(rawFeatureData.size() != features_[featureIdx].data.size() ) {
    cerr << "Treedata::replaceFeatureData(string) -- data dimension mismatch" << endl;
    exit(1);
  }

  features_[featureIdx] = Feature(rawFeatureData,features_[featureIdx].name);

}



