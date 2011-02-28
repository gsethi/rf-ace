//node.hpp
//
//A node class for CARTs

#ifndef NODE_HPP
#define NODE_HPP

#include<cstdlib>
#include<vector>
#include<set>
#include "datadefs.hpp"

using namespace std;
using datadefs::cat_t;
using datadefs::num_t;

class Node {
public:
  //Initializes node to store at max nsamples. Excess memory will be reserved in order to avoid dynamic memory allocation.
  Node(int nsamples);
  ~Node();

  //Sets a splitter feature for the node.
  //NOTE: splitter can be assigned only once! Subsequent setter calls will raise an assertion failure.
  void set_splitter(int splitter, set<cat_t> classet, Node& leftchild, Node& rightchild);
  void set_splitter(int splitter, num_t threshold, Node& leftchild, Node& rightchild);

  //Gets the splitter for the node
  int get_splitter();
  
  //Given value, descends to either one of the child nodes if existent and returns true, otherwise false.
  //NOTE: childp is a ref-to-ptr that will be modified to point to the child node if descend is successful. 
  bool descend(cat_t value, Node** childp);
  bool descend(num_t value, Node** childp);

  //Add an index of a sample to the node
  void add_trainsample_idx(int idx);
  void add_testsample_idx(int idx);

  //Reset sample indices
  void reset_trainsample_ics();
  void reset_testsample_ics();

  //Logic test whether the node has children or not
  bool is_leaf();

  //Helper functions
  void print();
  void print_compact();

private:
  bool isnum_;

  int splitter_;
  num_t threshold_;
  set<cat_t> classet_;

  vector<int> trainsampleics_;
  vector<int> testsampleics_;

  //vector<num_t> num_trainsamples_;
  //vector<num_t> num_testsamples_;

  size_t ntrainsamples_;
  size_t ntestsamples_;
  
  bool haschildren_;
  Node* leftchild_;
  Node* rightchild_;
};

#endif
