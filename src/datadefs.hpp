#ifndef DATADEFS_HPP
#define DATADEFS_HPP

#include<cstdlib>
#include<vector>
#include<set>
#include<string>
#include<math.h>
#include<map>
#include<cassert>
#include<iostream>

using namespace std;

namespace datadefs
{
  
  //Numerical data type
  typedef float num_t;
  //typedef num_t* num_tp;

  //NaN as represented by the program
  extern const num_t num_nan;
  extern const num_t eps;

  //NaNs supported in the delimited file 
  typedef string NAN_t;
  extern const set<NAN_t> NANs;

  void strv2catv(vector<string>& strvec, vector<num_t>& catvec);
  void strv2numv(vector<string>& strvec, vector<num_t>& numvec);

  num_t str2num(string& str);

  bool is_nan(const string& str);
  bool is_nan(const num_t value);
  
  void sqerr(vector<num_t> const& data, 
	     num_t& mu, 
	     num_t& se,
	     size_t& nreal);      
  
  //void quicksort(vector<num_t>& data);

  void count_real_values(vector<num_t> const& data, size_t& nreal);

  void update_sqerr(const num_t x_n,
		    size_t& n_left,
		    num_t& mu_left,
		    num_t& se_left,
		    size_t& n_right,
		    num_t& mu_right,
		    num_t& se_right);

  void count_freq(vector<num_t> const& data, map<num_t,size_t>& cat2freq, size_t& nreal);

  void map_data(vector<num_t>& data, 
		map<num_t,vector<size_t> >& datamap,
		size_t& nreal);

  void gini(vector<num_t>& data,
	    num_t& gi,
	    size_t& nreal);

  void gini(map<num_t,size_t>& cat2freq,
            num_t& gi);
  
  /*
    void update_gini(num_t x_n,
    const size_t n_left,
    map<num_t,size_t>& cat2freq_left,
    num_t& gi_left,
    const size_t n_right,
    map<num_t,size_t>& cat2freq_right,
    num_t& gi_right);
  */

  void sqfreq(vector<num_t> const& data, 
	      map<num_t,size_t>& freq, 
	      num_t& sf, 
	      size_t& nreal);

  void update_sqfreq(const num_t x_n,
		     size_t& n_left,
		     map<num_t,size_t>& freq_left,
		     num_t& sf_left,
		     size_t& n_right,
		     map<num_t,size_t>& freq_right,
		     num_t& sf_right);

  void range(vector<size_t>& ics);

  //A comparator functor that can be passed to STL::sort. Assumes that one is comparing first elements of pairs, first type being num_t and second T
  
  template <typename T> struct ordering {
    bool operator ()(pair<datadefs::num_t,T> const& a, pair<datadefs::num_t,T> const& b)
    {
      return(a.first < b.first);
    }
  }; 
  
  /*
    template <typename T> struct ordering {
    bool operator ()(pair<datadefs::num_t,T> const& a, pair<datadefs::num_t,T> const& b)
    {
    //cout << a.first << " < " << b.first << "(" << a.second << "," << b.second << ")" ;
    if(a.first < b.first || (a.first == a.first && b.first != b.first))
    {
    //cout << "=> true" << endl;
    return(true);
    }
    else
    {
    //cout << "=> false" << endl;
    return(false);
    }
    }
    };
  */
  

  template <typename T1,typename T2> void make_pairedv(vector<T1> const& v1,
                                                       vector<T2> const& v2,
                                                       vector<pair<T1,T2> >& p)
  {
    assert(v1.size() == v2.size() && v2.size() == p.size());
    for(size_t i = 0; i < p.size(); ++i)
      {
	p[i] = make_pair(v1[i],v2[i]);
      }
  }


  template <typename T1,typename T2> void separate_pairedv(vector<pair<T1,T2> > const& p,
                                                           vector<T1>& v1,
                                                           vector<T2>& v2)
  {
    assert(v1.size() == v2.size() && v2.size() == p.size());
    for(size_t i = 0; i < p.size(); ++i)
      {
	v1[i] = p[i].first;
	v2[i] = p[i].second;
      }
  }

  template <typename T> void sort_and_make_ref(vector<T>& v, vector<size_t>& ref_ics)
  {
    //cout << "sort_and_make_ref: in the beginning" << endl;
    assert(v.size() == ref_ics.size());
    vector<pair<T,size_t> > pairedv(ref_ics.size());
    datadefs::range(ref_ics);
    //cout << "sort_and_make_ref: used range()" << endl;
    datadefs::make_pairedv<T,size_t>(v,ref_ics,pairedv);
    //cout << "sort_and_make_ref: made pairedv" << endl;
    sort(pairedv.begin(),pairedv.end(),datadefs::ordering<T>());
    //cout << "sort_and_make_ref: pairedv sorted" << endl;
    datadefs::separate_pairedv<T,size_t>(pairedv,v,ref_ics);
  }

  //Sorts a given input data vector of type T based on a given reference ordering of type vector<int>
  template <typename T> void sort_from_ref(vector<T>& in, vector<size_t> const& ref_ics)
  {
    assert(in.size() == ref_ics.size());  
    vector<T> foo = in;
    int n = in.size();
    for (int i = 0; i < n; ++i)
      {
	in[i] = foo[ref_ics[i]];
      }
  }
}

#endif
