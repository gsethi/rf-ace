#include "datadefs.hpp"
#include<math.h>
#include<cassert>
#include<sstream>
#include<algorithm>
#include<iostream>

using namespace std;

//const datadefs::cat_t datadefs::cat_nan = -1;
const datadefs::num_t datadefs::num_nan = sqrt(-1.0);

const string initNANs[] = {"NA","NAN"};
const set<datadefs::NAN_t> datadefs::NANs(initNANs,initNANs+2);

void toupper(string& str)
{
  int (*pf)(int) = toupper;
  transform(str.begin(), str.end(), str.begin(), pf);
}

void datadefs::strv2catv(vector<string>& strvec, vector<datadefs::num_t>& catvec, map<string,size_t>& str2valmap)
{
  assert(strvec.size() == catvec.size());

  //Reset the map
  map<string,size_t> foo;
  str2valmap = foo;
  size_t val(0);

  //Map unique strings to values and store values in catvec as floats 
  for(size_t i = 0; i < strvec.size(); ++i)
    {
      //Transform string to uppercase
      toupper(strvec[i]);

      //If the string is not defined to NaN
      if(!datadefs::is_nan(strvec[i]))
	{
	  map<string,size_t>::iterator it;

	  //Try to find the string in the map. If it's not found, add the map...
	  it = str2valmap.find(strvec[i]);
	  if(it == str2valmap.end())
	    {
	      str2valmap.insert(pair<string,size_t>(strvec[i],val));
	      ++val;
	    }
	  //...and use the map to set the value for the output vector
	  catvec[i] = float(str2valmap[strvec[i]]);
	}
      //If the string is defined to NaN, however...
      else
	{
	  catvec[i] = datadefs::num_nan;
	}
    }  
}

void datadefs::strv2numv(vector<string>& strvec, vector<datadefs::num_t>& numvec)
{
  assert(strvec.size() == numvec.size());
  
  for(size_t i = 0; i < numvec.size(); ++i)
    {
      toupper(strvec[i]);
      if(!datadefs::is_nan(strvec[i]))
	{
	  numvec[i] = str2num(strvec[i]);
	}
      else
	{
	  numvec[i] = datadefs::num_nan;
	}
    }
}

datadefs::num_t datadefs::str2num(string& str)
{
  stringstream ss(str);
  datadefs::num_t ret;
  ss >> ret;
  return(ret);
}

bool datadefs::is_nan(string& str)
{
  set<string>::iterator it(NANs.find(str));
  if(it == NANs.end())
    {
      return(false);
    }
  else
    {
      return(true);
    }
}

bool datadefs::is_nan(datadefs::num_t value)
{
  if(value != value)
    {
      return(true);
    }
  else
    {
      return(false);
    }
}

void datadefs::sqerr(vector<datadefs::num_t> const& data, 
		     datadefs::num_t& mu, 
		     datadefs::num_t& se)
{
  
  size_t n(0);
  mu = 0.0;
  se = 0.0;
  if(!datadefs::is_nan(data[0]))
    {
      mu = data[0];
      n = 1;
    }
  
  for(size_t i = 1; i < data.size(); ++i)
    {
      if(!datadefs::is_nan(data[i]))
	{
	  ++n;
	  mu += data[i];
	}
    }
  
  mu /= n;
  
  //This should be computed iteratively inside the previous loop (speed-up)!
  for(size_t i = 0; i < data.size(); ++i)
    {
      if(!datadefs::is_nan(data[i]))
	{
	  se += pow(data[i] - mu,2);
	}
    }
}

//Assuming x_n is a current member of the "right" branch, subtract it from "right" and add it to "left", and update the branch data counts, means, and squared errors. NOTE: NaN checks not implemented
void datadefs::update_sqerr(const datadefs::num_t x_n,
			    const size_t n_left,
			    datadefs::num_t& mu_left,
			    datadefs::num_t& se_left,
			    const size_t n_right,
			    datadefs::num_t& mu_right,
			    datadefs::num_t& se_right)
{
  
  //Subtract x_n from "right" and update mean and squared error
  datadefs::num_t mu_old(mu_right);
  mu_right -= (x_n - mu_right) / n_right;

  //As long as there are at least two data points on the "right" branch, squared error can be calculated, otherwise assign se_right := 0.0
  if(n_right > 1)
    {
      se_right -= (x_n - mu_right) * (x_n - mu_old);
    }
  else
    {
      se_right = 0.0;
    }

  //Add x_n to "left" and update mean and squared error
  mu_old = mu_left;
  mu_left += (x_n - mu_left) / n_left;

  //If there are already at least two data points on the "left" branch, squared error can be calculated, otherwise assign se_left := 0.0
  if(n_left > 1)
    {
      se_left += (x_n - mu_left) * (x_n - mu_old);
    }
  else
    {
      se_left = 0.0;
    }
}

void datadefs::gini(const size_t n, 
		    map<datadefs::num_t,size_t> const& freq, 
		    datadefs::num_t& gi)
{
  //cout << "freq: ";
  gi = 0.0;
  map<datadefs::num_t,size_t>::const_iterator it;
  for(it = freq.begin(); it != freq.end(); ++it)
    {
      //cout << " " << it->first << "(" << it->second << ")";
      gi += pow(it->second,2);
    }
  gi = 1-gi/pow(n,2);
  //cout << " => impurity = " << gi << endl;
}

void datadefs::gini(vector<datadefs::num_t> const& data,
		    map<datadefs::num_t,size_t>& freq,
		    datadefs::num_t& gi)
{
  size_t n = data.size();
  map<datadefs::num_t,size_t> freqfoo;
  freq = freqfoo;
  map<datadefs::num_t,size_t>::const_iterator it;
  for(size_t i = 0; i < n; ++i)
    {
      if(!datadefs::is_nan(data[i]))
	{
	  it = freq.find(data[i]);
          if(it == freq.end())
            {
              freq.insert(pair<datadefs::num_t,size_t>(data[i],1));
            }
	  else
	    {
	      ++freq[data[i]];
	    }
	}
    }

  datadefs::gini(n,freq,gi); 
}

void datadefs::update_gini(num_t x_n,
			   const size_t n_left,
			   num_t& gi_left,
			   map<num_t,size_t>& freq_left,
			   const size_t n_right,
			   num_t& gi_right,
			   map<num_t,size_t>& freq_right)
{

  assert(false);

  map<datadefs::num_t,size_t>::const_iterator it(freq_left.find(x_n));
  if(it == freq_left.end())
    {
      freq_left.insert(pair<datadefs::num_t,size_t>(x_n,1));
    }
  else
    {
      ++freq_left[x_n];
    }

  it = freq_right.find(x_n);
  assert(it != freq_right.end() && it->second > 0);

  --freq_right[x_n];

  if(it->second == 0)
    {
      freq_right.erase(x_n);
    }

  datadefs::gini(n_left,freq_left,gi_left);
  datadefs::gini(n_right,freq_right,gi_right);

}

