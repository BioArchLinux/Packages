/////////////////////////////////////////////////////////////////
// EvolutionaryTree.hpp
//
// Utilities for reading/writing multiple sequence data.
/////////////////////////////////////////////////////////////////

#ifndef __EVOLUTIONARYTREE_HPP__
#define __EVOLUTIONARYTREE_HPP__

#include <string>
#include <list>
#include <stdio.h>
#include "SafeVector.h"
#include "MultiSequence.h"
#include "Sequence.h"
#include "Util.hpp"

using namespace std;


/////////////////////////////////////////////////////////////////
// TreeNode
//
// The fundamental unit for representing an alignment tree.  The
// guide tree is represented as a binary tree.
/////////////////////////////////////////////////////////////////
namespace MXSCARNA {
class TreeNode {
  int sequenceLabel;                  // sequence label
  float sequenceIdentity;             // sequence identity
  TreeNode *left, *right, *parent;    // pointers to left, right children
  float leftLength, rightLength;      // the length of left and right edge
  /////////////////////////////////////////////////////////////////
  // TreeNode::PrintNode()
  //
  // Internal routine used to print out the sequence comments
  // associated with the evolutionary tree, using a hierarchical
  // parenthesized format.
  /////////////////////////////////////////////////////////////////

  void PrintNode (ostream &outfile, const MultiSequence *sequences) const {

    // if this is a leaf node, print out the associated sequence comment
    if (sequenceLabel >= 0)
	//outfile << sequences->GetSequence (sequenceLabel)->GetHeader();
	outfile << sequences->GetSequence (sequenceLabel)->GetLabel();

    // otherwise, it must have two children; print out their subtrees recursively
    else {
      assert (left);
      assert (right);

      outfile << "(";
      left->PrintNode (outfile, sequences);
      outfile << ",";
      right->PrintNode (outfile, sequences);
      outfile << ")";
    }
  }

 public:

  /////////////////////////////////////////////////////////////////
  // TreeNode::TreeNode()
  //
  // Constructor for a tree node.  Note that sequenceLabel = -1
  // implies that the current node is not a leaf in the tree.
  /////////////////////////////////////////////////////////////////

  TreeNode (int sequenceLabel) : sequenceLabel (sequenceLabel),
    left (NULL), right (NULL), parent (NULL) {
    assert (sequenceLabel >= -1);
  }

  /////////////////////////////////////////////////////////////////
  // TreeNode::~TreeNode()
  //
  // Destructor for a tree node.  Recursively deletes all children.
  /////////////////////////////////////////////////////////////////

  ~TreeNode (){
    if (left){ delete left; left = NULL; }
    if (right){ delete right; right = NULL; }
    parent = NULL;
  }


  // getters
  int GetSequenceLabel () const { return sequenceLabel; }
  TreeNode *GetLeftChild () const { return left; }
  TreeNode *GetRightChild () const { return right; }
  TreeNode *GetParent () const { return parent; }
  float GetIdentity () const { return sequenceIdentity; }
  float GetLeftLength () const { return leftLength; }
  float GetRightLength () const { return rightLength; }
  // setters
  void SetSequenceLabel (int sequenceLabel){ this->sequenceLabel = sequenceLabel; assert (sequenceLabel >= -1); }
  void SetLeftChild (TreeNode *left){ this->left = left; }
  void SetRightChild (TreeNode *right){ this->right = right; }
  void SetParent (TreeNode *parent){ this->parent = parent; }
  void SetIdentity (float identity) { this->sequenceIdentity = identity; }
  void SetLeftLength (float identity) { this->leftLength = identity; }
  void SetRightLength (float identity) {this->rightLength = identity; }
  /////////////////////////////////////////////////////////////////
  // TreeNode::ComputeTree()
  //
  // Routine used to compute an evolutionary tree based on the
  // given distance matrix.  We assume the distance matrix has the
  // form, distMatrix[i][j] = expected accuracy of aligning i with j.
  /////////////////////////////////////////////////////////////////

  static TreeNode *ComputeTree (const VVF &distMatrix, const VVF &identityMatrix){

    int numSeqs = distMatrix.size();                 // number of sequences in distance matrix
    VVF distances (numSeqs, VF (numSeqs));           // a copy of the distance matrix
    SafeVector<TreeNode *> nodes (numSeqs, NULL);    // list of nodes for each sequence
    SafeVector<int> valid (numSeqs, 1);              // valid[i] tells whether or not the ith
                                                     // nodes in the distances and nodes array
                                                     // are valid
    VVF identities (numSeqs, VF (numSeqs));
    SafeVector<int> countCluster (numSeqs, 1);

    // initialization: make a copy of the distance matrix
    for (int i = 0; i < numSeqs; i++) {
	for (int j = 0; j < numSeqs; j++) {
	    distances[i][j]  = distMatrix[i][j];
	    identities[i][j] = identityMatrix[i][j];
	}
    }

    // initialization: create all the leaf nodes
    for (int i = 0; i < numSeqs; i++){
      nodes[i] = new TreeNode (i);
      assert (nodes[i]);
    }

    // repeat until only a single node left
    for (int numNodesLeft = numSeqs; numNodesLeft > 1; numNodesLeft--){
	float bestProb = -1;
      pair<int,int> bestPair;

      // find the closest pair
      for (int i = 0; i < numSeqs; i++) if (valid[i]){
        for (int j = i+1; j < numSeqs; j++) if (valid[j]){
          if (distances[i][j] > bestProb){
            bestProb = distances[i][j];
            bestPair = make_pair(i, j);
          }
        }
      }

      // merge the closest pair
      TreeNode *newParent = new TreeNode (-1);
      newParent->SetLeftChild (nodes[bestPair.first]);
      newParent->SetRightChild (nodes[bestPair.second]);
      nodes[bestPair.first]->SetParent (newParent);
      nodes[bestPair.second]->SetParent (newParent);
      nodes[bestPair.first] = newParent;
      nodes[bestPair.second] = NULL;
      newParent->SetIdentity(identities[bestPair.first][bestPair.second]);


      // now update the distance matrix
      for (int i = 0; i < numSeqs; i++) if (valid[i]){
	  distances[bestPair.first][i] = distances[i][bestPair.first]
	    = (distances[i][bestPair.first]*countCluster[bestPair.first] 
	       + distances[i][bestPair.second]*countCluster[bestPair.second])
	       / (countCluster[bestPair.first] + countCluster[bestPair.second]);
//	  distances[bestPair.first][i] = distances[i][bestPair.first]
//	      = (distances[i][bestPair.first] + distances[i][bestPair.second]) * bestProb / 2;
	identities[bestPair.first][i] = identities[i][bestPair.first]
	    = (identities[i][bestPair.first]*countCluster[bestPair.first] 
	       + identities[i][bestPair.second]*countCluster[bestPair.second])
	       / (countCluster[bestPair.first] + countCluster[bestPair.second]);
      }

      // finally, mark the second node entry as no longer valid
      countCluster[bestPair.first] += countCluster[bestPair.second];
      valid[bestPair.second] = 0;
    }

    assert (nodes[0]);
    return nodes[0];
  }

  /////////////////////////////////////////////////////////////////
  // TreeNode::Print()
  //
  // Print out the subtree associated with this node in a
  // parenthesized representation.
  /////////////////////////////////////////////////////////////////

  void Print (ostream &outfile, const MultiSequence *sequences) const {
//    outfile << "Alignment tree: ";
    PrintNode (outfile, sequences);
    outfile << endl;
  }
};
}
#endif //__EVOLUTIONARYTREE_HPP__
