
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "bTree.h"
#include "../unstdLib/vars.h"


/* getters */


bNode* getParent(bNode* n){ return n->metadata.parent; }
int    getOccupancy(bNode* n){ return n->metadata.occupancy; }
void   setParent(bNode* n, bNode* p){ n->metadata.parent = p; }
void   setOccupancy(bNode* n, int o){ n->metadata.occupancy = o; }

void assertOccupancy(bNode* n){
#ifndef NDEBUG
  int seenKeys = 0, seenChildren = 0, i;
  for(i = 0; i < bMaxChildren; i++){
    seenKeys += (NULL == n->keys[i] ? 0 : 1);
    seenChildren += (NULL == n->children[i] ? 0 : 1);
  }
  seenChildren += (NULL == n->children[i] ? 0 : 1);

  assert(seenKeys + 1 == seenChildren);
  assert(seenKeys == getOccupancy(n));
#endif
}

static int compare(bKey* k, uint64_t x){
  if(x <  k->start.i) return -1;
  if(x >= k->end.i  ) return 1;
  return 0;
}

#define   dupeIdx(i) (-((i) + 1))
#define deDupeIdx(i) ((-i) - 1)

static int findInsertionPoint(bNode* node, uint64_t toFind){
  int i;
  for(i = 0; i < getOccupancy(node); i++){
    bKey* k = node->keys[i];
    int cmp = compare(k, toFind);
    switch(cmp){
      case -1: return i;
      case  0: return dupeIdx(i); // found an exact match
      default: continue;
    }
  }
  return i;
}

/*
 * Insertion
 */

static bKey* nextKeyInTree(bNode* node, int insertionPoint){
  if(NULL == node)
    return NULL;
  const int occ = getOccupancy(node);
  if(insertionPoint >= occ)
    return nextKeyInTree(getParent(node), 0);
  return node->keys[insertionPoint];
}

static void listInsert0(void** list, int length, int idx, void* element){
  for(int i = length; i > idx; i--)
    list[i] = list[i-1];
  list[idx] = element;
}
#define listInsert(list, length, idx, element) listInsert0((void**)list, length, idx, element)

#define medianKeyIdx ((bMaxKeys - 1) >> 1)

static bNode* trySplitNode(bNode*);

static bNode* trySplitParent(bNode* relevantChild){
  trySplitNode(getParent(relevantChild));
  return getParent(relevantChild);
}

static bNode* splitNode(bNode* toSplit){
  assert(bMaxKeys == getOccupancy(toSplit));
  bNode* parent = trySplitParent(toSplit);
  bNode* newNode = calloc(sizeof(bNode), 1);
  setParent(newNode, parent);

  /*Split up the children*/
  int i;
  int dstIdx;
  // The median index is the one that will get promoted, so we copy everything after that, hence +1
  int Z = medianKeyIdx + 1;
  for(i = Z; i < bMaxKeys; i++){
    dstIdx = Z - i;
    takeElement(toSplit->keys, newNode->keys[dstIdx], i, NULL);
    takeElement(toSplit->children, newNode->children[dstIdx], i, NULL);
    setParent(newNode->children[dstIdx], newNode);
  }
  //the last child pointer has an index one greater than the last key
  dstIdx = Z - i;
  takeElement(toSplit->children, newNode->children[dstIdx], i, NULL);

  /*Now insert the median key into the parent along with the pointer to the new node*/
  bKey* medianKey;
  takeElement(toSplit->keys, medianKey, medianKeyIdx, NULL);

  int ins = findInsertionPoint(parent, medianKey->start.i);
  assert(0 <= ins); // negtive numbers imply conflict, which we checked for elsewhere with regards to keys
  assert(getOccupancy(parent) < bMaxKeys);

  listInsert(parent->keys, getOccupancy(parent), ins, medianKey);
  // +1 because we are inserting to the right of the new key which was inserted at `ins`
  listInsert(parent->children, getOccupancy(parent)+1, ins, newNode);

  setOccupancy(parent, getOccupancy(parent)+1);
  assertOccupancy(parent);
  setOccupancy(toSplit, medianKeyIdx); //Index of the median key is the same as the number of elements left
  assertOccupancy(toSplit);
  setOccupancy(newNode, (bMaxKeys - medianKeyIdx) - 1);
  assertOccupancy(newNode);

  return newNode;
}


static bNode* trySplitNode(bNode* toSplit){
  if(bMaxChildren == getOccupancy(toSplit))
    return splitNode(toSplit);
  return NULL;
}

bInsertResult bInsert(bNode* node, bKey* key){
  const int occ = getOccupancy(node);
  if(0 == occ){ //TODO: is this needed?
    //First element of a node? We are the very first key in the tree
    setOccupancy(node, 1);
    node->keys[0] = key;
    return (bInsertResult) { 0, NULL};
  }

  int i = findInsertionPoint(node, key->start.i);
  if(i < 1){ // Potential in-place update
    int idx = deDupeIdx(i);
    bKey* k = node->keys[idx];
    if(k->start.i != key->start.i || k->end.i != key->end.i)
      return (bInsertResult) {BInsertConflict, k};

    //Update in-place
    node->keys[idx] = key;
    return (bInsertResult) {0, k};
  }

  // No update in place, so we are inserting somewhere

  //First try to descend
  bNode* child = node->children[i];
  if(NULL != child)
    return bInsert(child, key);

  // we are in the leaf

  // see if this node requires splitting
  // if this node is split simply go back up to the parent, which will decide if we belong in the new or old node
  if(NULL != trySplitNode(node))
    return bInsert(getParent(node), key);

  //This is the destination node
  //First check that we don't conflict with the next key in the tree. That is: our end doesn't overlap their end
  bKey* next = nextKeyInTree(node, i);
  if(NULL != next && key->end.i > next->start.i)
    return (bInsertResult) {BInsertConflict, next};

  //Finally insert
  listInsert(node->children, occ, i, key);
  return (bInsertResult) {0, NULL};
}

/*
 * Deletion
 */

bKey* bRemove(bNode* node, bKey* toRemove){

}


void* bLookup(bNode* node, uint64_t toFind){
  if(NULL == node) return NULL; // No child, not foud

  const int i = findInsertionPoint(node, toFind);
  if(i < 0)
    return node->keys[deDupeIdx(i)]->value;
  else
    return bLookup(node->children[i], toFind);
}

