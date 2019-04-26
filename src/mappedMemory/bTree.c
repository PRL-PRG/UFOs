
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "bTree.h"
#include "../unstdLib/vars.h"


/* getters */


bNode* getParent(bNode* n){ return n->metadata.parent; }
int    getKeyOccupancy(bNode* n){ return n->metadata.occupancy; }
void   setParent(bNode* n, bNode* p){ n->metadata.parent = p; }
void   setOccupancy(bNode* n, int o){ n->metadata.occupancy = o; }

static bool isLeaf(bNode* n){ return NULL == n->children[0]; }


void assertOccupancy(bNode* n){
#ifndef NDEBUG
  int seenKeys = 0, seenChildren = 0, i;
  for(i = 0; i < bMaxChildren; i++){
    seenKeys += (NULL == n->keys[i] ? 0 : 1);
    seenChildren += (NULL == n->children[i] ? 0 : 1);
  }
  seenChildren += (NULL == n->children[i] ? 0 : 1);

  assert(seenKeys + 1 == seenChildren);
  assert(seenKeys == getKeyOccupancy(n));
#endif
}

static int compare(bKey* k, uint64_t x){
  if(x <  k->start.i) return -1;
  if(x >= k->end.i  ) return 1;
  return 0;
}

#define   dupeIdx(i) (-((i) + 1))
#define posOnlyIdx(i) ((-i) - 1)

static int findInsertionPoint(bNode* node, uint64_t toFind){
  int i;
  for(i = 0; i < getKeyOccupancy(node); i++){
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
  const int occ = getKeyOccupancy(node);
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

static void listRemove(void** list, int length, int idx){
  for(int i = idx; i < length; i++)
    list[i] = list[i+1];
  list[length-1] = NULL;
}

#define medianKeyIdx ((bMaxKeys - 1) >> 1)

static bNode* trySplitNode(bNode*);

static bNode* trySplitParent(bNode* relevantChild){
  trySplitNode(getParent(relevantChild));
  return getParent(relevantChild);
}

static bNode* splitNode(bNode* toSplit){
  assert(bMaxKeys == getKeyOccupancy(toSplit));
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
  assert(getKeyOccupancy(parent) < bMaxKeys);

  listInsert(parent->keys, getKeyOccupancy(parent), ins, medianKey);
  // +1 because we are inserting to the right of the new key which was inserted at `ins`
  listInsert(parent->children, getKeyOccupancy(parent)+1, ins, newNode);

  setOccupancy(parent, getKeyOccupancy(parent)+1);
  assertOccupancy(parent);
  setOccupancy(toSplit, medianKeyIdx); //Index of the median key is the same as the number of elements left
  assertOccupancy(toSplit);
  setOccupancy(newNode, (bMaxKeys - medianKeyIdx) - 1);
  assertOccupancy(newNode);

  return newNode;
}


static bNode* trySplitNode(bNode* toSplit){
  if(bMaxChildren == getKeyOccupancy(toSplit))
    return splitNode(toSplit);
  return NULL;
}

bInsertResult bInsert(bNode* node, bKey* key){
  const int occ = getKeyOccupancy(node);
  if(0 == occ){ //TODO: is this needed?
    //First element of a node? We are the very first key in the tree
    setOccupancy(node, 1);
    node->keys[0] = key;
    return (bInsertResult) { 0, NULL};
  }

  int i = findInsertionPoint(node, key->start.i);
  if(i < 1){ // Potential in-place update
    int idx = posOnlyIdx(i);
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
  assert(isLeaf(node) == (NULL ==  child));
  if(NULL != child)
    return bInsert(child, key);

  // we are in the leaf

  /* see if this node requires splitting
     if this node is split simply go back up to the parent, which will decide if we belong in the new or old node
     even if more than one parent is split, it won't move our target node farther away than an adjacent sibling
     This is because
      • it splits before it sends a key up
      • We are greater than the dividing key to the left in the parent
      • we are less than the dividing key to the right in the parent
      • since we belong between those two keys and we don't change they keys in the parent before splitting there are only two cases to consider
       1. We still belong in this node and will simply be found again
       2. We belong in the new node
         · When this node is split we split the parent first and then resolve the parent of this node
         · We then proceed with our split and insert the new node into that parent
         · Thus we guarantee that our key belongs below the parent no matter how the tree above was split
   */
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

#define StealLeft  1
#define StealRight 2

#define idxFromDirection(direction, node) (StealLeft  == direction ? 0 : (getOccupancy(node) - 1))

#define noParent              1
#define treeError             2
#define noLeaf                3
#define noSibling             4
#define siblingWillUnderflow  5

static int findChildIdxInParent(bNode* child, int* idx){
  bNode* parent = getParent(child);
  if(NULL == parent)
    return noParent;

  for(int i = 0 ; i <= getKeyOccupancy(parent); i++){
    if(parent->children[i] == child){
      *idx = i;
      return 0;
    }
  }
  assert(0); // thats not good, we weren't a child of our parent?
  return treeError;
}

static int findStealableLeaf(bNode* startingNode, uint keyIdx, int* direction_p, bNode** leaf_p){
  assert(getKeyOccupancy(startingNode) > keyIdx);
  bNode* child;
  int direction = StealLeft;
  do{
  // Every key divides two branches, so start by going the correct direction
    if(StealLeft  == direction)
      child = startingNode->children[keyIdx];
    else
      child = startingNode->children[keyIdx+1];

    assert(NULL != child);

    while(!isLeaf(child)){
      //When stealing from the left we want to find the rightmost child in the left subtree ...
      if(StealLeft  == direction)
        child = child->children[getKeyOccupancy(child)];
      else // ... and vice versa
        child = child->children[0];
      assert(NULL != child);
    }
    if(getKeyOccupancy(child) > bMinKeys)
      goto done;
    switch(direction){
      case StealLeft : direction = StealRight; break;
      case StealRight: goto done;
      default:
        assert(false);
        goto err;
    }
  }while(true);
  done:

  if(child == startingNode)
    goto err;

  *direction_p = direction;
  *leaf_p      = child;
  return 0;

  err:
  *direction_p = 0;
  *leaf_p      = NULL;
  return noLeaf;
}

//TODO: always delete logically-"from" leafs, to do this we really are stealing from the leafs when working in internal nodes

static int findStealableSiblingKey(bNode* node, int* direction_p, bNode** sibling_p, int* stealIdx_p){
  assert(NULL != node);

  bNode* parent = getParent(node);

  int direction;
  int idx;
  int status = findChildIdxInParent(node, &idx);
  if(status) // fails harmlessly at root
    goto _noSibling;

  bNode* toStealFrom = NULL;
  if(idx > 0){
    toStealFrom = parent->children[idx - 1];
    direction = StealLeft;
  }
  assert(toStealFrom != node); // Can't happen, we'd be index 0 and the if would have been bypassed
  if(NULL == toStealFrom || getKeyOccupancy(toStealFrom) <= bMinChildren){
    if(idx <= getKeyOccupancy(parent)){
      toStealFrom = parent->children[idx + 1];
      direction = StealRight;
      assert(toStealFrom != node); // Can't happen, we'd be the final idx and the if would have been bypassed
    }
  }

  if(NULL == toStealFrom)
    goto _noSibling;

  int occupancy = getKeyOccupancy(toStealFrom);
  assert(occupancy > 0);

  *sibling_p   = toStealFrom;
  *direction_p = direction;
  *stealIdx_p  = idxFromDirection(direction, toStealFrom);
  return (occupancy > bMinKeys) ? 0 : siblingWillUnderflow;

  _noSibling:
  *sibling_p   = NULL;
  *direction_p = 0;
  *stealIdx_p  = -1;
  return noSibling;
}

#define cannotRotate 1

static int tryRotate(bNode* node){
  bNode* parent = getParent(node);
  if(NULL == parent)
    return cannotRotate;

  int direction, stealIdx;
  bNode* sibling;
  int status = findStealableSiblingKey(node, &direction, &sibling, &stealIdx);
  if(status)
    return cannotRotate;

  bKey* newParentKey = sibling->keys[stealIdx];
  assert(NULL != newParentKey);
  listRemove(sibling->keys, getKeyOccupancy(sibling ), stealIdx);

//  int insertIdx = idxFromDirection(direction, node);
//  listInsert0(node-> keys, getKeyOccupancy(node), insertIdx, newParentKey);

}

static bKey* rebalanceAfterDelete(bNode* node, bKey* removed){
  if(getKeyOccupancy(node) >= bMinKeys || NULL == getParent(node))
    return removed; // Done

  //TODO: everything

  /* rotates */


  /* parent stealing & further rebalancing */

  return removed;
}

bKey* bRemove(bNode* node, bKey* toRemove){
  if(NULL == node) return NULL; // Not found

  bKey* removed;
  int i = findInsertionPoint(node, toRemove->start);
  if(i < 0){ // potential match
    int idx = posOnlyIdx(i);
    removed = node->keys[idx];
    assert(NULL != removed);

    if(toRemove->start.i != removed->start.i || toRemove->end.i != removed->end.i)
      return NULL; // Not an exact match

    if(isLeaf(node)){
      listRemove(node->keys, getOccupancy(node), idx);
      return rebalanceAfterDelete(node, removed);
    }else{
      //TODO: steal a key from a leaf and then start rebalance at the leaf
    }

    //TODO: how do we get the new root, if root was changed?
  }else{
    //recurse
    bNode child = node->children[i];
    assert(isLeaf(node) || child != NULL);
    return bRemove(child, toRemove);
  }
}


void* bLookup(bNode* node, uint64_t toFind){
  if(NULL == node) return NULL; // No child, not foud

  const int i = findInsertionPoint(node, toFind);
  if(i < 0)
    return node->keys[posOnlyIdx(i)]->value;
  else
    return bLookup(node->children[i], toFind);
}

