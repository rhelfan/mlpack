/*
 *  n_point.cc
 *  
 *
 *  Created by William March on 2/24/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "n_point.h"

// returns true if the indices violate the symmetry requirement
bool NPointAlg::PointsViolateSymmetry_(index_t ind1, index_t ind2) {
  return (ind2 <= ind1);
} // PointsViolateSymmetry_()

int NPointAlg::BaseCaseHelper_(ArrayList<ArrayList<index_t> >& point_sets,
                                  ArrayList<bool>& permutation_ok,
                                  ArrayList<index_t>& points_in_tuple,
                                  double* weighted_result, int k) {
  
  int result = 0;
  
  ArrayList<bool> permutation_ok_copy;
  permutation_ok_copy.InitCopy(permutation_ok);
  
  ArrayList<index_t> k_rows;
  k_rows.InitAlias(point_sets[k]);
  
  bool bad_symmetry = false;
  
  // loop over possible points for the kth member of the tuple
  for (index_t i = 0; !bad_symmetry && i < k_rows.size(); i++) {
    
    index_t point_index_i = k_rows[i];
    
    bool this_point_works = true;

    Vector point_i;
    data_points_.MakeColumnVector(point_index_i, &point_i);
    
    // TODO: is this too inefficient?
    permutation_ok_copy.Clear();
    permutation_ok_copy.AppendCopy(permutation_ok);
    
    // loop over previously chosen points to see if there is a conflict
    // IMPORTANT: I'm assuming here that once a point violates symmetry,
    // all the other points will too
    // This means I'm assuming that the lists of points are in a continuous 
    // order
    for (index_t j = 0; !bad_symmetry && this_point_works && j < k; j++) {
      
      index_t point_index_j = points_in_tuple[j];
      
      // need to swap indices here, j should come before i
      bad_symmetry = PointsViolateSymmetry_(point_index_j, point_index_i);
      
      // don't compute the distances if we don't have to
      if (!bad_symmetry) {
        Vector point_j;
        data_points_.MakeColumnVector(point_index_j, &point_j);
        
        double point_dist_sq = la::DistanceSqEuclidean(point_i, point_j);
        
        //printf("Testing point pair (%d, %d)\n", j, k);
        this_point_works = matcher_.TestPointPair(point_dist_sq, j, k, 
                                                  permutation_ok_copy);
        
      } // compute the distances and check the matcher
      
    } // for j
    
    /*
    printf("Considering point %d in position %d.  bad_symmetry: %d, works: %d\n",
           point_index_i, k, bad_symmetry, this_point_works);
    */
    
    // now, if the point passed, we put it in place and recurse
    if (this_point_works && !bad_symmetry) {
      
      points_in_tuple[k] = point_index_i;
      
      // base case of the recursion
      if (k == tuple_size_ - 1) {
        
        result++;
        
        // take care of weights
        double these_weights = 1.0;
        
        for (index_t a = 0; a < num_tuples_; a++) {
          these_weights *= data_weights_[points_in_tuple[a]];
        } // for a
        
        (*weighted_result) = (*weighted_result) + these_weights;
        
      } // base case
      else {
        
        result += BaseCaseHelper_(point_sets, permutation_ok_copy, 
                                  points_in_tuple, weighted_result, k+1);
        
      } // recurse
      
      DEBUG_ONLY(points_in_tuple[k] = -1);
      
    } // did the point work
    
  } // for i
  
  return result;
  
} // BaseCaseHelper_()


int NPointAlg::BaseCase_(ArrayList<ArrayList<index_t> >& point_sets, 
                         double* weighted_result) {

  ArrayList<bool> permutation_ok;
  permutation_ok.Init(matcher_.num_permutations());
  for (index_t i = 0; i < matcher_.num_permutations(); i++) {
    permutation_ok[i] = true;
  } // for i
  
  ArrayList<index_t> points_in_tuple;
  points_in_tuple.Init(tuple_size_);
  
#ifdef DEBUG
  for (index_t j = 0; j < tuple_size_; j++) {
    points_in_tuple[j] = -1;
  } // for j
#endif
  
  int result = BaseCaseHelper_(point_sets, permutation_ok, points_in_tuple, 
                               weighted_result, 0);
  
  return result;
  
} // BaseCase_()


// TODO: make this handle weighted results, should be easy
int NPointAlg::DepthFirstRecursion_(ArrayList<NPointNode*>& nodes, 
                                    index_t previous_split) {
  
  //printf("Depth first recursion on nodes: \n");
  //printf("Node1: [%d, %d); Node2: [%d, %d)\n", nodes[0]->begin(),
  //       nodes[0]->end(), nodes[1]->begin(), nodes[1]->end());
  
  DEBUG_ASSERT(nodes.size() == tuple_size_);
  
  bool all_leaves = true;
  
  int num_tuples_here = 0;
  
  index_t split_index = -1;
  int split_count = -1;
  
  ArrayList<int> permutation_ok;
  permutation_ok.Init(matcher_.num_permutations());
  for (index_t i = 0; i < permutation_ok.size(); i++) {
    permutation_ok[i] = SUBSUME;
  }
  
  // main loop
  for (index_t i = 0; i < tuple_size_; i++) {
    
    NPointNode* node_i = nodes[i];
    
    if (!(node_i->is_leaf())) {
      all_leaves = false;
    
      // is this the largest non-leaf?
      if (node_i->count() > split_count) {
        split_index = i;
        split_count = node_i->count();
      }
      
    } // is this a leaf?
    
    for (index_t j = i+1; j < tuple_size_; j++) {
      
      NPointNode* node_j = nodes[j];
      
      // TODO: would it be more efficient to make this check outside, without
      // possibly doing some permutation checks with the matcher
      // Another possibility would be to check this before making a recursive 
      // call -> this is closer to the way my HF code works
      
      // check if the nodes are in the right order, if not, return 0
      //if (node_j->stat().node_index() < node_i->stat().node_index()) {
      if (node_j->end() <= node_i->begin()) {  
      
        //printf("Returning for violated symmetry.\n\n");
        return 0;
        
      }
      
      int status = matcher_.TestHrectPair(node_i->bound(), node_j->bound(), i, 
                                          j, permutation_ok);
      
      // TODO: the auton code doesn't exit the loops here, why?
      // the comments say it has something to do with accumulating bounds
      if (status == EXCLUDE) {
        // we should be able to prune
        
        num_exclusion_prunes_++;
        
        num_tuples_here = 0;
        
        //printf("Returning for exclusion prune.\n\n");
        return num_tuples_here;
        
      } // are we able to exclude this n-tuple?
      

    } // looping over other nodes (j)
    
  } // looping over nodes in the tuple (i)
  
  // this is where we would check for a subsume prune
  
  // now, recurse, but how?
  
  if (all_leaves) {
    // call the base case
    // TODO: is it worth doing this after the prune checks?  
    // i.e. if it is an all leaf case, should we check this in the beginning and call the 
    // base case before trying to prune, since the base case will make 
    // basically the same checks anyway
    
    double this_weighted_result = 0.0;
    
    // fill in the array of point indices
    ArrayList<ArrayList<index_t> > point_sets;
    point_sets.Init(tuple_size_);
    for (index_t i = 0; i < tuple_size_; i++) {
      point_sets[i].Init(nodes[i]->count());
      
      for (index_t j = 0; j < nodes[i]->count(); j++) {
        point_sets[i][j] = j + nodes[i]->begin();
      } // for j
      
    } // for i
    
    //printf("Doing BaseCase on pair (%d, %d)\n", point_sets[0][0], point_sets[1][0]);
    num_tuples_here = BaseCase_(point_sets, &this_weighted_result);
    //printf("tuples in this base case: %d\n\n", num_tuples_here);
    
  } // base case
  else {
    
    // TODO: be clever about which pairs I have to test here
    // I should only have to check the n pairs that involve one of the new 
    // nodes, since all the rest can't have changed
    // I think this will require storing more details about what caused a 
    // permutation to be rejected before
    // This is referenced somewhere in the auton code, npt3.c?
    
    // For now, the heuristic is to split the largest non-leaf node
    NPointNode* split_node = nodes[split_index];
    
    //printf("Recursing on tuple %d\n", split_index);
    
    nodes[split_index] = split_node->left();
    num_tuples_here += DepthFirstRecursion_(nodes, split_index);
    nodes[split_index] = split_node->right();
    num_tuples_here += DepthFirstRecursion_(nodes, split_index);
    
    nodes[split_index] = split_node;
  
    //printf("Returning after recursive calls.\n\n");
    
  } // not base case
  
  return num_tuples_here;
  
} // DepthFirstRecursion_()




