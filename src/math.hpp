#ifndef MATH_HPP
#define MATH_HPP

#include <vector>
#include "datadefs.hpp"
#include "errno.hpp"

using namespace std;
using datadefs::num_t;


namespace math {

  /**
     Returns the p'th percentile of the data vector x
  */
  num_t percentile(vector<num_t> x, const num_t p);


  /**
     Updates the mean and squared error by ADDING x_n to the set
     NOTE: NANs will corrupt the data
  */
  inline void incrementSquaredError(const num_t& x_n,
				    const size_t& n,
				    num_t& mu,
				    num_t& se) {
    
    // Save the current mean
    num_t mu_old = mu;
    
    // Update mean
    mu += (x_n - mu_old) / n;
    
    // If there are already at least two data points, 
    // squared error can be calculated, otherwise assign se := 0.0
    if(n > 1) {
      se += (x_n - mu) * (x_n - mu_old);
    } else { 
      se = 0.0; /** Implementation note: this may spuriously invoke on
		    overflow. size_t is assumed to always be unsigned in
		    accordance with the 1999 ISO C standard (C99). */
    }
  } 


  /**
     Updates the mean and squared error by REMOVING x_n from the set
     NOTE: NANs will corrupt the data
  */
  inline void decrementSquaredError(const num_t& x_n,
				    const size_t& n,
				    num_t& mu,
				    num_t& se) {
    
    
    if(n > 1) {
      num_t mu_old = mu;
      mu -= (x_n - mu) / n;
      se -= (x_n - mu) * (x_n - mu_old);
    } else if(n == 1) {
      mu -= (x_n - mu) / n;
      se = 0.0;
    } else {
      mu = 0.0;
      se = 0.0;
    }
  }

  /**
     Updates the squared frequency by ADDING x_n to the set
     NOTE: NANs will corrupt the data
  */
  inline void incrementSquaredFrequency(const num_t x_n,
					map<num_t,size_t>& freq,
					size_t& sqFreq) {


    // Check if the value already exists in the frequency map
    map<num_t,size_t>::const_iterator it(freq.find(x_n));
    if(it == freq.end()) {

      // If not, squared frequency becomes updated by 1
      sqFreq += 1;
      freq[x_n] = 1;

    } else {

      // Otherwise the squared frequency becomes updated by 
      // 2*freq + 1
      sqFreq += 2*freq[x_n] + 1;
      ++freq[x_n];

    }
  }
 
  /**
     Updates the squared frequency by REMOVING x_n
     from the set
     NOTE: NANs will corrupt the data
  */ 
  inline void decrementSquaredFrequency(const num_t x_n,
                                        map<num_t,size_t>& freq,
                                        size_t& sqFreq) {
    
    assert( freq.find(x_n) != freq.end() );
    assert( freq[x_n] > 0);
    
    sqFreq -= 2*freq[x_n] - 1;
    --freq[x_n];
    
    if(freq[x_n] == 0) {
      freq.erase(x_n);
    }
  }
  
}
  
#endif
