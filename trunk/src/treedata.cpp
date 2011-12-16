#include "treedata.hpp"
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <ctime>

using namespace std;

Treedata::Treedata(string fileName, char dataDelimiter, char headerDelimiter):
  dataDelimiter_(dataDelimiter),
  headerDelimiter_(headerDelimiter),
  features_(0),
  sampleHeaders_(0) {

  //Initialize stream to read from file
  ifstream featurestream;
  featurestream.open(fileName.c_str());
  if ( !featurestream.good() ) {
    cerr << "Failed to open file '" << fileName << "' for reading" << endl;
    assert(false);
  }

  FileType fileType = UNKNOWN;
  Treedata::readFileType(fileName,fileType);

  vector<vector<string> > rawMatrix;
  vector<string> featureHeaders;
  vector<bool> isFeatureNumerical;
  if(fileType == AFM) {
    
    Treedata::readAFM(featurestream,rawMatrix,featureHeaders,sampleHeaders_,isFeatureNumerical);
    
  } else if(fileType == ARFF) {
    
    sampleHeaders_.clear();
    sampleHeaders_.resize(rawMatrix[0].size(),"NO_SAMPLE_ID");
    Treedata::readARFF(featurestream,rawMatrix,featureHeaders,isFeatureNumerical);
    
  } else {
    
    Treedata::readAFM(featurestream,rawMatrix,featureHeaders,sampleHeaders_,isFeatureNumerical);
    
  }      
  
  if ( !datadefs::is_unique(featureHeaders) ) {
    cerr << "Feature headers are not unique!" << endl;
    assert(false);
  }

  size_t nFeatures = featureHeaders.size();
  features_.resize(2*nFeatures);

  for(size_t i = 0; i < nFeatures; ++i) {
    
    if( name2idx_.find(featureHeaders[i]) != name2idx_.end() ) {
      cerr << "Duplicate feature header '" << featureHeaders[i] << "' found!" << endl;
      exit(1);
    }

    name2idx_[featureHeaders[i]] = i;

    vector<num_t> featureData( sampleHeaders_.size() );
    features_[i].name = featureHeaders[i];
    features_[i].isNumerical = isFeatureNumerical[i];
    if(features_[i].isNumerical) {
      datadefs::strv2numv(rawMatrix[i],featureData);
      features_[i].nCategories = 0;
    } else {
      map<string,num_t> mapping;
      map<num_t,string> backMapping;
      datadefs::strv2catv(rawMatrix[i], featureData, mapping, backMapping);
      features_[i].mapping = mapping;
      features_[i].backMapping = backMapping;
      map<num_t,size_t> freq;
      size_t nReal;
      datadefs::count_freq(featureData, freq, nReal);
      features_[i].nCategories = freq.size();
    }
    features_[i].data = featureData;
  } 
  
  for(size_t i = nFeatures; i < 2*nFeatures; ++i) {
    features_[i] = features_[ i - nFeatures ];
    string contrastName = features_[ i - nFeatures ].name;
    features_[i].name = contrastName.append("_CONTRAST");
    name2idx_[contrastName] = i;
  }

  // Initialize the Mersenne Twister RNG with the CPU cycle count as the seed
  time_t now;
  time(&now);
  unsigned int seed = clock() + now;
  randomInteger_.seed(seed);

  //cout << "permuting contrasts..." << endl;

  Treedata::permuteContrasts();

  //cout << "done permuting" << endl;

}



Treedata::Treedata(Treedata& treedata):
  features_(0),
  sampleHeaders_(0) {

  time_t now;
  time(&now);
  unsigned int seed = clock() + now;
  randomInteger_.seed(seed);

  dataDelimiter_ = treedata.dataDelimiter_;
  headerDelimiter_ = treedata.headerDelimiter_;
  features_ = treedata.features_;
  sampleHeaders_ = treedata.sampleHeaders_;
  name2idx_ = treedata.name2idx_;

}


Treedata::Treedata(Treedata& treedata, const vector<size_t>& featureIcs):
  features_(0),
  sampleHeaders_(0) {

  time_t now;
  time(&now);
  unsigned int seed = clock() + now;
  randomInteger_.seed(seed);

  size_t nFeaturesNew = featureIcs.size();

  // We'll leave room for the contrasts
  features_.resize(2*nFeaturesNew);
  for(size_t i = 0; i < nFeaturesNew; ++i) {
    features_[i] = treedata.features_[ featureIcs[i] ];
    features_[i + nFeaturesNew] = treedata.features_[ featureIcs[i] + treedata.nFeatures() ];
  }
  
  dataDelimiter_ = treedata.dataDelimiter_;
  headerDelimiter_ = treedata.headerDelimiter_;

  sampleHeaders_ = treedata.sampleHeaders_;
  name2idx_ = treedata.name2idx_;
  
}

Treedata::~Treedata() {
}

void Treedata::keepFeatures(const vector<size_t>& featureIcs) {
  
  size_t nFeaturesOld = Treedata::nFeatures();
  size_t nFeaturesNew = featureIcs.size();
  
  vector<Feature> featureCopy = features_;
  features_.resize(2*nFeaturesNew);
  name2idx_.clear();
  for(size_t i = 0; i < nFeaturesNew; ++i) {
    name2idx_[ featureCopy[featureIcs[i]].name ] = i;
    name2idx_[ featureCopy[ nFeaturesOld + featureIcs[i] ].name ] = nFeaturesNew + i;
    features_[i] = featureCopy[featureIcs[i]];
    features_[ nFeaturesNew + i ] = featureCopy[ nFeaturesOld + featureIcs[i] ];
  }
  //nFeatures_ = nFeaturesNew;
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
		       vector<bool>& isFeatureNumerical) {

  string field;
  string row;

  rawMatrix.clear();
  featureHeaders.clear();
  sampleHeaders.clear();
  isFeatureNumerical.clear();

  //Remove upper left element from the matrix as useless
  getline(featurestream,field,dataDelimiter_);

  //Next read the first row, which should contain the column headers
  getline(featurestream,row);
  stringstream ss( datadefs::chomp(row) );
  bool isFeaturesAsRows = true;
  vector<string> columnHeaders;
  while ( getline(ss,field,dataDelimiter_) ) {

    // If at least one of the column headers is a valid feature header, we assume features are stored as columns
    if ( isFeaturesAsRows && isValidFeatureHeader(field) ) {
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
  while ( getline(featurestream,row) ) {

    row = datadefs::chomp(row);

    //Read row from the stream
    ss.clear();
    ss.str("");

    //Read the string back to a stream
    ss << row;

    //Read the next row header from the stream
    getline(ss,field,dataDelimiter_);
    rowHeaders.push_back(field);

    vector<string> rawVector(nColumns);
    for(size_t i = 0; i < nColumns; ++i) {
      getline(ss,rawVector[i],dataDelimiter_);
    }
    assert(!ss.fail());
    assert(ss.eof());
    rawMatrix.push_back(rawVector);
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
  isFeatureNumerical.resize(nFeatures);
  for(size_t i = 0; i < nFeatures; ++i) {
    if(Treedata::isValidNumericalHeader(featureHeaders[i])) {
      isFeatureNumerical[i] = true;
    } else {
      isFeatureNumerical[i] = false;
    }
  }
}

void Treedata::readARFF(ifstream& featurestream, vector<vector<string> >& rawMatrix, vector<string>& featureHeaders, vector<bool>& isFeatureNumerical) {

  string row;

  bool hasRelation = false;
  bool hasData = false;

  size_t nFeatures = 0;
  //TODO: add Treedata::clearData(...)
  rawMatrix.clear();
  featureHeaders.clear();
  isFeatureNumerical.clear();
  
  //Read one line from the ARFF file
  while ( getline(featurestream,row) ) {

    //This is the final branch: once relation and attributes are read, and we find data header, we'll start reading the data in 
    if ( hasData && hasRelation ) {
      //There must be at least two attributes, otherwise the ARFF file makes no sense
      if ( nFeatures < 2 ) {
        cerr << "too few attributes ( < 2 ) found from the ARFF file" << endl;
        assert(false);
      }

      rawMatrix.resize(nFeatures);

      //Read data row-by-row
      while(true) {
        //++nsamples_;
        string field;
        stringstream ss(row);
        
        for(size_t attributeIdx = 0; attributeIdx < nFeatures; ++attributeIdx) {
          getline(ss,field,',');
          //cout << " " << field;
          rawMatrix[attributeIdx].push_back(field);
        }
        //cout << endl;

        if(!getline(featurestream,row)) {
          break;
        }
      }

      break;
    }

    //Comment lines and empty lines are omitted
    if(row[0] == '%' || row == "") {
      continue;  
    }

    //Read relation
    if(!hasRelation && row.compare(0,9,"@relation") == 0) {
      hasRelation = true;
      //cout << "found relation header: " << row << endl;
    } else if(row.compare(0,10,"@attribute") == 0) {    //Read attribute 
      string attributeName = "";
      bool isNumerical;
      ++nFeatures;
      //cout << "found attribute header: " << row << endl;
      Treedata::parseARFFattribute(row,attributeName,isNumerical);
      featureHeaders.push_back(attributeName);
      isFeatureNumerical.push_back(isNumerical);

    } else if(!hasData && row.compare(0,5,"@data") == 0) {    //Read data header 

      hasData = true;
      //cout << "found data header:" << row << endl;
    } else {      //If none of the earlier branches matched, we have a problem
      cerr << "incorrectly formatted ARFF file" << endl;
      assert(false);
    }
  }
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
  if(datadefs::toUpperCase(attributeType) == "NUMERIC" || datadefs::toUpperCase(attributeType) == "REAL" ) {
    isFeatureNumerical = true;
  } else {
    isFeatureNumerical = false;
  }
  //prefix.append(attributeName);
  //attributeName = prefix;
}

bool Treedata::isValidNumericalHeader(const string& str) {
  
  stringstream ss(str);
  string typeStr;
  getline(ss,typeStr,headerDelimiter_);

  return(  typeStr == "N" );
}

bool Treedata::isValidCategoricalHeader(const string& str) {

  stringstream ss(str);
  string typeStr;
  getline(ss,typeStr,headerDelimiter_);

  return( typeStr == "C" || typeStr == "B" );
}

bool Treedata::isValidFeatureHeader(const string& str) {
  return( isValidNumericalHeader(str) || isValidCategoricalHeader(str) );
}

size_t Treedata::nFeatures() {
  return( features_.size() / 2 );
}

size_t Treedata::nSamples() {
  return( sampleHeaders_.size() );
}

// WILL BECOME DEPRECATED
num_t Treedata::pearsonCorrelation(size_t featureidx1, size_t featureidx2) {
  num_t r;
  datadefs::pearson_correlation(features_[featureidx1].data,features_[featureidx2].data,r);
  return(r);
}

// WILL BECOME DEPRECATED
void Treedata::getMatchingTargetIdx(const string& targetStr, size_t& targetIdx) {
  
  bool isFoundAlready = false;

  for ( size_t featureIdx = 0; featureIdx < Treedata::nFeatures(); ++featureIdx ) {
    
    if ( features_[featureIdx].name == targetStr ) {
    
      if ( isFoundAlready ) {
	cerr << "Multiple instances of the same target found in the data!" << endl;
	assert(false);
      }
      
      isFoundAlready = true;
      targetIdx = featureIdx;
    
    }
  }

  if ( !isFoundAlready ) {
    cerr << "Feature '" << targetStr << "' not found" << endl;
    assert(false);
  }

}

string Treedata::getFeatureName(const size_t featureIdx) {
  return(features_[featureIdx].name);
}

string Treedata::getSampleName(const size_t sampleIdx) {
  return(sampleHeaders_[sampleIdx]);
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


void Treedata::permuteContrasts() {

  for(size_t i = Treedata::nFeatures(); i < 2*Treedata::nFeatures(); ++i) {
    Treedata::permute(features_[i].data);
  }

}

bool Treedata::isFeatureNumerical(size_t featureIdx) {

  return(features_[featureIdx].isNumerical);

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

  return( features_[featureIdx].nCategories );

}

size_t Treedata::nMaxCategories() {

  size_t ret = 0;
  for( size_t i = 0; i < Treedata::nFeatures(); ++i ) {
    if( ret < features_[i].nCategories ) {
      ret = features_[i].nCategories;
    }
  }
  
  return( ret ); 

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


void Treedata::permute(vector<size_t>& ics) {
  for (size_t i = 0; i < ics.size(); ++i) {
    size_t j = randomInteger_() % (i + 1);
    ics[i] = ics[j];
    ics[j] = i;
  }
}

void Treedata::permute(vector<num_t>& data) {
  size_t n = data.size();
  vector<size_t> ics(n);

  Treedata::permute(ics);

  for(size_t i = 0; i < n; ++i) {
    num_t temp = data[i];
    data[i] = data[ics[i]];
    data[ics[i]] = temp;
  }
}


void Treedata::bootstrapFromRealSamples(const bool withReplacement, 
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
  for(size_t i = 0; i < Treedata::nSamples(); ++i) {
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
      ics[sampleIdx] = allIcs[randomInteger_() % nRealSamples];
    }
  } else {  //If sampled without replacement...
    vector<size_t> foo(nRealSamples);
    Treedata::permute(foo);
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

void Treedata::getFilteredDataPair(const size_t featureIdx1, const size_t featureIdx2, vector<size_t>& sampleIcs, vector<num_t>& featureData1, vector<num_t>& featureData2) {

  size_t n = sampleIcs.size();
  featureData1.resize(n);
  featureData2.resize(n);
  size_t nReal = 0;
  for(size_t i = 0; i < n; ++i) {

    num_t v1 = features_[featureIdx1].data[sampleIcs[i]];
    num_t v2 = features_[featureIdx2].data[sampleIcs[i]];
    
    if(!datadefs::isNAN(v1) && !datadefs::isNAN(v2)) {
      sampleIcs[nReal] = sampleIcs[i];
      featureData1[nReal] = v1;
      featureData2[nReal] = v2;
      ++nReal;
    }
  }
  featureData1.resize(nReal);
  featureData2.resize(nReal);
  sampleIcs.resize(nReal);

}

vector<num_t> Treedata::operator[](size_t featureIdx) {
  return( features_[featureIdx].data );
}

vector<num_t> Treedata::operator[](const string& featureName) {
  return( features_[ name2idx_[featureName] ].data );
}


string Treedata::getRawFeatureData(const size_t featureIdx, const size_t sampleIdx) {

  num_t value = features_[featureIdx].data[sampleIdx];

  if(datadefs::isNAN(value)) {
    return(datadefs::STR_NAN);
  } else {
    if(features_[featureIdx].isNumerical) {
      stringstream ss;
      ss << value;
      return(ss.str());
    } else {
      return(features_[featureIdx].backMapping[ value ]);
    }
  }
    
}

map<string,num_t> Treedata::getDataMapping(const size_t featureIdx) {

  return( features_[featureIdx].mapping );

}


// DEPRECATED ??
void Treedata::impurity(vector<num_t>& data, bool isFeatureNumerical, num_t& impurity, size_t& nreal) {
  
  size_t n = data.size();
  
  
  nreal = 0;
  if(isFeatureNumerical) {
    num_t mu = 0.0;
    num_t se = 0.0;
    for(size_t i = 0; i < n; ++i) {
      datadefs::forward_sqerr(data[i],nreal,mu,se);  
    }
    impurity = se / nreal;
  } else {
    map<num_t,size_t> freq;
    size_t sf = 0;
    for(size_t i = 0; i < n; ++i) {
      datadefs::forward_sqfreq(data[i],nreal,freq,sf);
    }
    impurity = 1.0 - 1.0 * sf / (nreal * nreal);
  }
}
