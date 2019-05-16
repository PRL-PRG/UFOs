
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bTree.h"
#include "../unstdLib/vars.h"


/* getters */


bNode* getParent(bNode* n){ return n->metadata.parent; }
int    getKeyOccupancy(bNode* n){ return n->metadata.occupancy; }
void   setParent(bNode* n, bNode* p){ n->metadata.parent = p; }
void   setOccupancy(bNode* n, int o){ n->metadata.occupancy = o; }

static bool isLeaf(bNode* n){ return NULL == n->children[0]; }


void assertOccupancy0(bNode* n){
  int seenKeys = 0, seenChildren = 0, i;
  for(i = 0; i < bMaxChildren; i++){
    seenKeys += (NULL == n->keys[i] ? 0 : 1);
    seenChildren += (NULL == n->children[i] ? 0 : 1);
  }
  seenChildren += (NULL == n->children[i] ? 0 : 1);

  assert(seenKeys + 1 == seenChildren);
  assert(seenKeys == getKeyOccupancy(n));
}

#ifndef NDEBUG
  #define  assertOccupancy(n) assertOccupancy0(n)
#else
  #define  assertOccupancy(n) ({})
#endif

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
      case -1: return i; //Once the key k is lower than the one we are looking for then we have found the insertion point
      case  0: return dupeIdx(i); // found an exact match, indicate with a negative index
      default: continue; // Keep looking
    }
  }
  return i; // End of the list
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
  //      dest,       src,      # bytes
  memmove(list+idx+1, list+idx, (length - idx) * sizeof(void*));
  list[idx] = element;
}
#define listInsert(list, length, idx, element) listInsert0((void**)list, length, idx, element)

static void listRemove0(void** list, int length, int idx){
  //      dest,     src,        # bytes
  memmove(list+idx, list+idx+1, (length - idx) * sizeof(void*));
  list[length-1] = NULL;
}
#define listRemove(list, length, idx) listRemove0((void**)list, length, idx)

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

#define MergeLeft  1
#define MergeRight 2

/*
 * When stealing right the "leafmost" index is the one on the left (0)
 * This is because we want the first key from the right
 * Opposite with left, we find the key by going right repeatedly
 *  */
#define leafmostKeyIdxFromDirection(direction, node) (StealLeft == direction ? getKeyOccupancy(node) - 1 : 0)
#define leafmostChildIdxFromDirection(direction, node) (StealLeft == direction ? getKeyOccupancy(node): 0)

// Left is zero, right is the last key index
#define keyIdxFromDirection(direction, node) (StealLeft == direction ? 0 : getKeyOccupancy(node) - 1)
#define childIdxFromDirection(direction, node) (StealLeft == direction ? 0 : getKeyOccupancy(node))

#define noParent              1
#define treeError             2
#define noLeaf                3
#define noSibling             4
#define siblingWillUnderflow  5
#define cannotRotate          6

static int findChildIdxInParent(bNode* child, int* idx){
  bNode* parent = getParent(child);
  if(NULL == parent)
    return noParent; // root

  for(int i = 0 ; i <= getKeyOccupancy(parent); i++){
    if(parent->children[i] == child){
      *idx = i;
      return 0;
    }
  }
  assert(0); // thats not good, we weren't a child of our parent?
  return treeError;
}

static int findStealableLeaf(bNode* startingNode, uint keyIdx, int* direction_p, bNode** leaf_p, int* leafKeyIdx){
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
      goto done; //Found a leaf with a key to spare

    switch(direction){ //Try the other direction
      case StealLeft : direction = StealRight; break;
      case StealRight: goto done; // Doesn't matter if there are not enough keys, we need something to rob
      default:
        assert(false); // We should never see a direction other than left or right
        goto err;
    }
  }while(true);
  done:

  if(child == startingNode)
    goto err;

  *direction_p = direction;
  *leaf_p      = child;
  *leafKeyIdx  = StealLeft ? getKeyOccupancy(child) : 0;
  return 0;

  err:
  *direction_p = 0;
  *leaf_p      = NULL;
  *leafKeyIdx  = -1;
  return noLeaf;
}

static int findStealableSiblingKey(bNode* node, int* direction_p, bNode** sibling_p,
    int* stealKeyIdx_p, int* stealChildIdx_p, int* stealParentKeyIdx_p){
  assert(NULL != node);
  int ret = -1;

  bNode* parent = getParent(node);
  if(NULL == parent){
    ret = noSibling;
    goto err;
  }

  int direction;
  int idx;
  int status = findChildIdxInParent(node, &idx);
  if(status){ // Shouldn't be possible as of writing (CMYK 2019.04.18)
    ret = noSibling;
    goto err;
  }

  bNode* toStealFrom = NULL;
  if(idx > 0){
    // Go Left
    toStealFrom = parent->children[idx - 1];
    direction = StealLeft;
  }
  assert(toStealFrom != node); // Can't happen, we'd be index 0 and the if would have been bypassed
  if(NULL == toStealFrom || getKeyOccupancy(toStealFrom) <= bMinChildren){
    if(idx <= getKeyOccupancy(parent)){
      // Go Right
      toStealFrom = parent->children[idx + 1];
      direction = StealRight;
      assert(toStealFrom != node); // Can't happen, we'd be the final idx and the if would have been bypassed
    }
  }

  if(NULL == toStealFrom){
    ret = noSibling;
    goto err;
  }

  int occupancy = getKeyOccupancy(toStealFrom);
  assert(occupancy > 0);

  if(occupancy <= bMinKeys){
    ret = siblingWillUnderflow;
    goto err;
  }

  *sibling_p   = toStealFrom;
  *direction_p = direction;
  *stealKeyIdx_p  = leafmostKeyIdxFromDirection(direction, toStealFrom);
  *stealChildIdx_p  = leafmostChildIdxFromDirection(direction, toStealFrom);
  *stealParentKeyIdx_p = StealLeft ? idx - 1 : idx + 1;
  assert(*stealParentKeyIdx_p >= 0);
  assert(*stealParentKeyIdx_p < getKeyOccupancy(parent));
  return 0;

  err:
  *sibling_p   = NULL;
  *direction_p = 0;
  *stealKeyIdx_p  = -1;
  *stealChildIdx_p  = -1;
  *stealParentKeyIdx_p = -1;
  return ret;
}

static int tryRotate(bNode* node){
  bNode* parent = getParent(node);
  if(NULL == parent)
    return cannotRotate;

  int direction, stealKeyIdx, stealChildIdx, stealParentKeyIdx;
  bNode* sibling;
  int status = findStealableSiblingKey(node, &direction, &sibling, &stealKeyIdx, &stealChildIdx, &stealParentKeyIdx);
  if(status)
    return cannotRotate;

  // Parent key comes from sibling
  bKey* newParentKey = sibling->keys[stealKeyIdx];
  assert(NULL != newParentKey);
  // Our new key comes from the parent
  bKey* newKey = parent->keys[stealParentKeyIdx];
  assert(NULL != newKey);
  // Our new child pointer comes from the sibling
  bNode* newChildPointer = sibling->children[stealChildIdx];
  assert(NULL != newChildPointer);

  int siblingOcc = getKeyOccupancy(sibling);
  int newSiblingOcc = siblingOcc - 1;
  assert(newSiblingOcc >= bMinKeys);
  // Take the elements from the sibling
  listRemove(sibling->keys, siblingOcc, stealKeyIdx);
  listRemove(sibling->children, siblingOcc + 1, stealChildIdx);
  // Set the sibling's occupancy
  setOccupancy(sibling, newSiblingOcc);

  int nodeOcc = getKeyOccupancy(node);
  int newOcc = nodeOcc + 1;
  assert(bMaxKeys >= newOcc);

  int insertKeyIdx = keyIdxFromDirection(direction, node);
  int insertChildIdx = childIdxFromDirection(direction, node);
  // Move things into place
  parent->keys[stealParentKeyIdx] = newParentKey;
  listInsert(node->keys, nodeOcc, insertKeyIdx, newKey);
  listInsert(node->children, nodeOcc+1, insertChildIdx, newChildPointer);
  setOccupancy(node, newOcc);

  assertOccupancy(node);
  assertOccupancy(sibling);
  assertOccupancy(parent);

  return 0;
}

static int mergeWithSibling(bNode* node){
  bNode* parent = getParent(node);
  if(NULL == parent)
    return 0; // Root

  int direction, idx;
  int status = findChildIdxInParent(node, &idx);
  if(status){
    assert(false);
    return treeError;
  }

  //If we are the first key then merge right, otherwise merge left
  direction = (0 == idx) ? MergeRight : MergeLeft;
  int siblingIdx = MergeLeft ? idx - 1 : idx + 1;

  bNode* sibling = parent->children[siblingIdx];
  // We must have a sibling, the only way for a node to be empty is temporarily during a delte
  assert(NULL != sibling);

  int siblingOcc = getKeyOccupancy(sibling);
  int ourOcc = getKeyOccupancy(node);
  int parentOcc = getKeyOccupancy(parent);

  //Both of our siblings weren't able to donate a key, so it should always be possible to merge with either
  assert(ourOcc + siblingOcc <= bMaxKeys);

  if(MergeRight == direction){
    // Only when merging right: we need to make some room for the contents we want to migrate
    // One key from the parent, all our remaining keys, and all our children

    // Make room then move the keys, and then the same for the child pointers
    //      dest,                       src,                # bytes
    memmove(sibling->keys+ourOcc+1,     sibling->keys, siblingOcc * sizeof(void*));
    memmove(sibling->keys,            node->keys,      ourOcc     * sizeof(void*));
    sibling->keys[ourOcc] = parent->keys[idx];
    listRemove(parent->keys, parentOcc,  idx);

    memmove(sibling->children+ourOcc+1, sibling->children, (siblingOcc+1) * sizeof(void*));
    memmove(sibling->children,        node->children,    (ourOcc+1)     * sizeof(void*));
  }else{
    //TODO: steal the parent key first!!
    // Just move over the pointers to the ends of the list
    sibling->keys[siblingOcc] = parent->keys[idx-1]; // Merging left, went to use the key before us
    listRemove(parent->keys, parentOcc,  idx);
    memmove(sibling->keys+siblingOcc+1,   node->keys,      ourOcc    * sizeof(void*));
    memmove(sibling->children+siblingOcc, node->children, (ourOcc+1) * sizeof(void*));
  }

  setOccupancy(parent, parentOcc-1);
  assertOccupancy(parent);
  setOccupancy(sibling, siblingOcc + 1 + ourOcc);
  assertOccupancy(sibling);

  free(node);

  return 0;
}

static bKey* rebalanceAfterDelete(bNode* node, bKey* removed){
  if( getKeyOccupancy(node) >= bMinKeys // No balance needed
      || NULL == getParent(node)) // Root
    return removed; // Done

  /* rotation is preffered to stealing from the parent*/
  if(!tryRotate(node))
    return removed; // After a rotate the tree is balanced

  /* parent stealing & further rebalancing */
  mergeWithSibling(node);
  return rebalanceAfterDelete(getParent(node), removed);
}

bKey* bRemove(bNode* node, bKey* toRemove){
  if(NULL == node) return NULL; // Not found

  bKey* removed;
  int i = findInsertionPoint(node, toRemove->start.i);
  if(i < 0){ // potential match
    int idx = posOnlyIdx(i);
    removed = node->keys[idx];
    assert(NULL != removed);

    if(toRemove->start.i != removed->start.i || toRemove->end.i != removed->end.i)
      return NULL; // Not an exact match

    if(isLeaf(node)){
      listRemove(node->keys, getKeyOccupancy(node), idx);
      return rebalanceAfterDelete(node, removed);
    }else{
      //TODO: steal a key from a leaf and then start rebalance at the leaf
      int direction, stealIdx;
      bNode *toStealFrom;
      int status = findStealableLeaf(node, idx, &direction, &toStealFrom, &stealIdx);
      assert(!status); // There must be a leaf beneath us

      swap(node->keys[idx], toStealFrom->keys[stealIdx]);
      listRemove(toStealFrom->keys, getKeyOccupancy(toStealFrom), stealIdx);
      return rebalanceAfterDelete(toStealFrom, removed);
    }

    //TODO: how do we get the new root, if root was changed?
  }else{
    //recurse
    bNode* child = node->children[i];
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

