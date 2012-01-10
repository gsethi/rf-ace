#include <iostream>
#include <cassert>
#include "partitionsequence.hpp"

PartitionSequence::PartitionSequence(const size_t nMaxLength) {

  //assert( nMaxLength > 0 );
  //assert( nMaxLength < sizeof(graycode_t)*8 );

  if( nMaxLength == 0 ) {
    cerr << endl << "ERROR: Gray Code cannot be initialized with 0 bits. Quitting..." << endl;
    exit(1);
  }

  if( nMaxLength > sizeof(graycode_t)*8 - 1 )  {
    cerr << endl << "ERROR: one or more features have too high cardinality ("  << nMaxLength + 1 << " > " << sizeof(graycode_t)*8 << endl;
    exit(1);
  }

  size_t nCodes = (1 << nMaxLength);

  graycode_.resize(nCodes,0);
  bitSequence_.resize(nCodes-1,0);
  addBit_.resize(nCodes-1,false);

  for ( size_t codeIdx = 0; codeIdx < nCodes; ++codeIdx ) {

    // Gray Code as integer
    graycode_t codeAsInt = static_cast<graycode_t>(codeIdx);
    
    // Integer transformed to Gray Code
    // Reference: http://answers.yahoo.com/question/index?qid=20071013065704AAgrGp3
    graycode_[codeIdx] |= codeAsInt ^ ( codeAsInt >> 1 );

  }

  for ( size_t codeIdx = 0; codeIdx < nCodes - 1; ++codeIdx) {
    
    graycode_t graycodeXOR = graycode_[codeIdx] ^ graycode_[codeIdx+1];
    while ( graycodeXOR > 1 ) {
      graycodeXOR = ( graycodeXOR >> 1 );
      ++bitSequence_[codeIdx];
    }

    if ( graycode_[codeIdx+1] > graycode_[codeIdx] ) {
      addBit_[codeIdx] = true;
    } 

  }
  
}

PartitionSequence::~PartitionSequence() {

  //cout << "PartitionSequence::dtor" << endl;

}

bool PartitionSequence::isAdded(const size_t pos) {

  return( addBit_[pos] );

}

size_t PartitionSequence::at(const size_t pos) {

  return( bitSequence_[pos] );

}
