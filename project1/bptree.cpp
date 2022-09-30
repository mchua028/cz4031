#include <cstring>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include "bptree.h"
using namespace std;

// Node::Node()
// {
//     keys = new int[NODE_KEYS];
//     ptrs = new ptrs_struct[NODE_KEYS + 1];
// }

Node::~Node()
{
    delete[] keys;
    delete[] ptrs;
}

Node::Node(int i)
{
    keys = new int[i];
    ptrs = new ptrs_struct[i + 1];
}

BPTree::BPTree()
{
    root = NULL;
}

BPTree::~BPTree()
{
    cleanUp(root);
}

// recursively delete nodes of bptree
void BPTree::cleanUp(Node *cursor)
{
    cout << "Cleaning up bptree" << endl;
    if (cursor != NULL)
    {
        if (!cursor->isLeaf)
        {
            for (int i = 0; i < cursor->size + 1; i++)
            {
                cleanUp((Node *)cursor->ptrs[i].nodePtr);
            }
        }
        // cout << "deleting node starting with key " << cursor->keys[0] << endl;
        delete[] cursor->keys;
        delete[] cursor->ptrs;
        delete cursor;
    }
}

// Search operation for insert
Node *BPTree::search(int key)
{
    if (root == NULL)
    {
        cout << "Tree is empty\n";
        return NULL;
    }
    else
    {
        Node *cursor = root;
        // find leaf node which may contain key
        while (cursor->isLeaf == false)
        {
            for (int i = 0; i < cursor->size; i++)
            {
                if (key < cursor->keys[i])
                {
                    cursor = (Node *)cursor->ptrs[i].nodePtr;
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptrs[i + 1].nodePtr;
                    break;
                }
            }
        }
        // find key in leaf node
        for (int i = 0; i < cursor->size; i++)
        {
            if (cursor->keys[i] == key)
            {
                cout << "Found\n";
                return (Node *)cursor;
            }
        }
        cout << "Not found\n";
        return NULL;
    }
}

// Search operation for a key
vector<byte *> BPTree::searchRecords(int key)
{
    bool found = false;
    vector<vector<int>> indexes;
    int noOfIndexes = 0;
    vector<int> newIndex;
    vector<byte *> recordList;
    if (root == NULL)
    {
        cout << "Tree is empty\n";
    }
    else
    {
        Node *cursor = root;
        // find leaf node which may contain key
        while (cursor->isLeaf == false)
        {
            // capturing index node's contents
            noOfIndexes++;
            newIndex.clear();
            for (int i = 0; i < cursor->size; i++)
            {
                newIndex.push_back(cursor->keys[i]);
            }
            indexes.push_back(newIndex);

            for (int i = 0; i < cursor->size; i++)
            {
                if (key < cursor->keys[i])
                {
                    cursor = (Node *)cursor->ptrs[i].nodePtr;
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptrs[i + 1].nodePtr;
                    break;
                }
            }
        }
        // capturing leaf node's contents
        noOfIndexes++;
        newIndex.clear();
        for (int i = 0; i < cursor->size; i++)
        {
            newIndex.push_back(cursor->keys[i]);
        }
        indexes.push_back(newIndex);

        // find key in leaf node
        for (int i = 0; i < cursor->size; i++)
        {
            if (cursor->keys[i] == key)
            {
                cout << "Found\n";
                found = true;
                recordList = cursor->ptrs[i].recordPtrs;
                break;
            }
        }
        if (!found)
        {
            cout << "Not found\n";
        }
    }
    cout << "Number of index blocks accessed: " << indexes.size() << endl;
    for (int i = 0; i < indexes.size() && i < 5; i++)
    {
        cout << "Contents of index block " << i << ":" << endl;
        for (int j = 0; j < indexes[i].size(); j++)
        {
            cout << indexes[i][j] << ", ";
        }
        cout << "\n";
    }
    // cout << "Returned recordList size: " << recordList.size() << endl;
    return recordList;
}

// search Range operation
vector<byte *> BPTree::searchRange(int startKey, int endKey, int NODE_KEYS)
{
    bool found = false;
    bool end = false;
    int startPos;
    vector<vector<int>> indexes;
    int noOfIndexes = 0;
    vector<int> newIndex;
    vector<byte *> recordList;
    if (root == NULL)
    {
        cout << "Tree is empty\n";
    }
    else
    {
        Node *cursor = root;
        // find leaf node which may contain startKey
        while (cursor->isLeaf == false)
        {
            // capturing index node's contents
            noOfIndexes++;
            newIndex.clear();
            for (int i = 0; i < cursor->size; i++)
            {
                newIndex.push_back(cursor->keys[i]);
            }
            indexes.push_back(newIndex);

            for (int i = 0; i < cursor->size; i++)
            {
                if (startKey < cursor->keys[i])
                {
                    cursor = (Node *)cursor->ptrs[i].nodePtr;
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptrs[i + 1].nodePtr;
                    break;
                }
            }
        }
        // capturing leaf node's contents
        noOfIndexes++;
        newIndex.clear();
        for (int i = 0; i < cursor->size; i++)
        {
            newIndex.push_back(cursor->keys[i]);
        }
        indexes.push_back(newIndex);

        // find startKey in leaf node
        for (int i = 0; i < cursor->size; i++)
        {
            if (cursor->keys[i] >= startKey)
            {
                cout << "Found first record\n";
                found = true;
                recordList = cursor->ptrs[i].recordPtrs;
                startPos = i;
                break;
            }
        }
        // if startKey not found in the leaf node, try next leaf node
        if (!found)
        {
            // capture index node contents

            cursor = (Node *)cursor->ptrs[NODE_KEYS].nodePtr;
            if (cursor != NULL)
            {
                noOfIndexes++;
                newIndex.clear();
                for (int i = 0; i < cursor->size; i++)
                {
                    newIndex.push_back(cursor->keys[i]);
                }
                indexes.push_back(newIndex);

                recordList = cursor->ptrs[0].recordPtrs;
                startPos = 0;
                cout << "Found first record\n";
                found = true;
            }
        }
        if (!found)
        {
            cout << "No records found in this range\n";
        }
        // get all records in the range [startKey,endKey]
        // iterate through current index node
        if (found)
        {
            for (int i = startPos + 1; i < cursor->size; i++)
            {
                if (cursor->keys[i] <= endKey)
                {
                    recordList.insert(recordList.end(), cursor->ptrs[i].recordPtrs.begin(), cursor->ptrs[i].recordPtrs.end());
                }
                else
                {
                    end = true;
                    break;
                }
            }
            if (!end)
            {
                // iterate through adjacent index nodes
                cursor = (Node *)cursor->ptrs[NODE_KEYS].nodePtr;
                int i = 0;
                if (cursor != NULL)
                {
                    noOfIndexes++;
                    newIndex.clear();
                    for (int i = 0; i < cursor->size; i++)
                    {
                        newIndex.push_back(cursor->keys[i]);
                    }
                    indexes.push_back(newIndex);
                }
                while (cursor != NULL && cursor->keys[i] <= endKey)
                {
                    recordList.insert(recordList.end(), cursor->ptrs[i].recordPtrs.begin(), cursor->ptrs[i].recordPtrs.end());
                    i++;
                    if (i == cursor->size)
                    {
                        cursor = (Node *)cursor->ptrs[NODE_KEYS].nodePtr;
                        i = 0;
                        if (cursor != NULL)
                        {
                            noOfIndexes++;
                            newIndex.clear();
                            for (int i = 0; i < cursor->size; i++)
                            {
                                newIndex.push_back(cursor->keys[i]);
                            }
                            indexes.push_back(newIndex);
                        }
                    }
                }
            }
        }
    }
    cout << "Number of index blocks accessed: " << indexes.size() << endl;
    for (int i = 0; i < indexes.size() && i < 5; i++)
    {
        cout << "Contents of index block " << i << ":" << endl;
        for (int j = 0; j < indexes[i].size(); j++)
        {
            cout << indexes[i][j] << ", ";
        }
        cout << "\n";
    }
    return recordList;
}

// Insert Operation
void BPTree::insert(int key, byte *recordAdd, int NODE_KEYS)
{
    if (root == NULL) // if no root
    {
        void *test;
        root = new Node(NODE_KEYS);
        root->keys[0] = key;
        // insert adress of record insertion point 1
        root->ptrs[0].recordPtrs.push_back(recordAdd);
        root->isLeaf = true;
        root->size = 1;
    }
    else // if root exsists
    {
        Node *cursor = root;
        Node *parent;
        // check if key already exist in b+ tree
        Node *searchKey;
        searchKey = search(key);
        // if key already exist in b+ tree
        if (searchKey != nullptr)
        {
            for (int i = 0; i < searchKey->size; i++)
            {
                if (searchKey->keys[i] == key)
                {
                    searchKey->ptrs[i].recordPtrs.push_back(recordAdd);
                    break;
                }
            }
            return;
        }

        // if key does not already exist in b+ tree
        while (cursor->isLeaf == false) // traverse to the leaf level
        {
            parent = cursor;
            for (int i = 0; i < cursor->size; i++)
            {
                if (key < cursor->keys[i])
                {
                    cursor = (Node *)cursor->ptrs[i].nodePtr;
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptrs[i + 1].nodePtr;
                    break;
                }
            }
        }
        if (cursor->size < NODE_KEYS) // if this leaf node is not full
        {
            int i = 0;
            while (key > cursor->keys[i] && i < cursor->size) // find the index of the first key that is larger than x
                i++;
            for (int j = cursor->size; j > i; j--) // copy the keys from the back to make space for new key insertion point
            {
                cursor->keys[j] = cursor->keys[j - 1];
                cursor->ptrs[j] = cursor->ptrs[j - 1];
            }

            cursor->keys[i] = key;
            ptrs_struct newptr;
            newptr.recordPtrs.push_back(recordAdd);
            cursor->ptrs[i] = newptr;
            cursor->size++;
            // cursor->ptrs[cursor->size] = cursor->ptrs[cursor->size - 1]; // update the pointer to next leaf
            // cursor->ptrs[cursor->size - 1] = NULL;
        }
        else // if the leaf node is full
        {
            Node *newLeaf = new Node(NODE_KEYS);
            // int virtualNode[MAX + 1]; // create an array of size max+1 to store the keys, including the new key
            Node *virtualNode = new Node(NODE_KEYS + 1); // create a virtualnode with 1 more key+pointer
            // copy contents of current leaf node to virtualNode
            for (int i = 0; i < NODE_KEYS; i++)
            {
                // virtualNode[i] = cursor->key[i];
                virtualNode->keys[i] = cursor->keys[i];
                virtualNode->ptrs[i] = cursor->ptrs[i];
            }
            // find position to insert new key
            int i = 0, j;
            while (key > virtualNode->keys[i] && i < NODE_KEYS) // i = index of first key larger than key
                i++;
            // make space for new key
            for (int j = NODE_KEYS; j > i; j--)
            {
                virtualNode->keys[j] = virtualNode->keys[j - 1];
                virtualNode->ptrs[j] = virtualNode->ptrs[j - 1];
            }
            virtualNode->keys[i] = key; // replace key
            ptrs_struct newptr;
            newptr.recordPtrs.push_back(recordAdd);
            virtualNode->ptrs[i] = newptr; // replace recordaddress

            newLeaf->isLeaf = true;
            cursor->size = (NODE_KEYS + 1) / 2;
            newLeaf->size = NODE_KEYS + 1 - (NODE_KEYS + 1) / 2; // splitting the node into 2 and deciding th sizes
            newLeaf->ptrs[NODE_KEYS] = cursor->ptrs[NODE_KEYS];  // exhange pointers to next leaf
            cursor->ptrs[NODE_KEYS].nodePtr = newLeaf;           // update pointer to next leaf node

            // cursor->ptrs[NODE_KEYS].clear();                        // remove original link to next leaf

            for (i = 0; i < cursor->size; i++)
            {
                cursor->keys[i] = virtualNode->keys[i]; // transfering keys into old node insertion point 3
                cursor->ptrs[i] = virtualNode->ptrs[i]; // transfering ptrs into old node insertion point 3
            }
            for (i = 0, j = cursor->size; i < newLeaf->size; i++, j++)
            {
                newLeaf->keys[i] = virtualNode->keys[j]; // transfering keys into new node insertion point 4
                newLeaf->ptrs[i] = virtualNode->ptrs[j]; // transfering keys into new node insertion point 4
            }

            // if there is only cursor and newLeaf, just create a new root
            if (cursor == root)
            {
                Node *newRoot = new Node(NODE_KEYS);
                newRoot->keys[0] = newLeaf->keys[0];
                newRoot->ptrs[0].nodePtr = cursor;
                newRoot->ptrs[1].nodePtr = newLeaf;
                newRoot->isLeaf = false;
                newRoot->size = 1;
                root = newRoot;
            }
            else // there exist at least 2 levels, insert a new key into internal nodes
            {
                insertInternal(newLeaf->keys[0], parent, newLeaf,NODE_KEYS);
            }
        }
    }
}

// Insert Operation
void BPTree::insertInternal(int x, Node *cursor, Node *child, int NODE_KEYS)
{
    // there is still space in the parent node
    if (cursor->size < NODE_KEYS)
    {
        int i = 0;
        while (x > cursor->keys[i] && i < cursor->size)
            i++;
        for (int j = cursor->size; j > i; j--)
        {
            cursor->keys[j] = cursor->keys[j - 1];
        }
        for (int j = cursor->size + 1; j > i + 1; j--)
        {
            cursor->ptrs[j] = cursor->ptrs[j - 1];
        }
        cursor->keys[i] = x;
        cursor->ptrs[i + 1].nodePtr = child;
        cursor->size++;
    }
    // no more space in the parent node, need to split the parent node
    else
    {
        Node *newInternal = new Node(NODE_KEYS);
        int virtualKey[NODE_KEYS + 1];
        Node *virtualPtr[NODE_KEYS + 2];
        for (int i = 0; i < NODE_KEYS; i++)
        {
            virtualKey[i] = cursor->keys[i];
        }
        for (int i = 0; i < NODE_KEYS + 1; i++)
        {
            virtualPtr[i] = (Node *)cursor->ptrs[i].nodePtr;
        }
        // find position to insert x
        int i = 0, j;
        while (x > virtualKey[i] && i < NODE_KEYS)
            i++;
        // make space for x
        for (int j = NODE_KEYS; j > i; j--)
        {
            virtualKey[j] = virtualKey[j - 1];
        }
        virtualKey[i] = x;
        // make space for pointer to child
        for (int j = NODE_KEYS + 1; j > i + 1; j--)
        {
            virtualPtr[j] = virtualPtr[j - 1];
        }
        virtualPtr[i + 1] = child;

        newInternal->isLeaf = false;

        cursor->size = (NODE_KEYS + 1) / 2;
        newInternal->size = NODE_KEYS - (NODE_KEYS + 1) / 2;

        // assign key and ptrs of cursor
        for (i = 0; i < cursor->size; i++)
        {
            cursor->keys[i] = virtualKey[i];
        }
        for (i = 0; i < cursor->size + 1; i++)
        {
            cursor->ptrs[i].nodePtr = virtualPtr[i];
        }
        // assign keys and ptrs of newInternal
        for (i = 0, j = cursor->size + 1; i < newInternal->size; i++, j++)
        {
            newInternal->keys[i] = virtualKey[j];
        }
        for (i = 0, j = cursor->size + 1; i < newInternal->size + 1; i++, j++)
        {
            newInternal->ptrs[i].nodePtr = virtualPtr[j];
        }

        if (cursor == root)
        {
            Node *newRoot = new Node(NODE_KEYS);
            // newRoot->keys[0] = cursor->keys[cursor->size];
            // get the smallest key found in the subtree under newInternal
            newRoot->keys[0] = findSmallestKeyInSubtree(newInternal);
            newRoot->ptrs[0].nodePtr = cursor;
            newRoot->ptrs[1].nodePtr = newInternal;
            newRoot->isLeaf = false;
            newRoot->size = 1;
            root = newRoot;
        }
        else // there are more than 2 levels in the current tree
        {
            insertInternal(findSmallestKeyInSubtree(newInternal), findParent(root, cursor), newInternal,NODE_KEYS);
        }
    }
}

// find smallest key in subtree
int BPTree::findSmallestKeyInSubtree(Node *cursor)
{
    int smallestKey;
    while (!cursor->isLeaf)
    {
        cursor = (Node *)cursor->ptrs[0].nodePtr;
    }
    smallestKey = cursor->keys[0];
    return smallestKey;
}

// Find the parent
Node *BPTree::findParent(Node *cursor, Node *child)
{
    Node *parent;
    // cursor cannot be a parent if it is a leaf node and parent of an internal node cannot be in the second last level
    if (cursor->isLeaf || ((Node *)cursor->ptrs[0].nodePtr)->isLeaf)
    {
        return NULL;
    }
    // starting from root node, find the child node
    for (int i = 0; i < cursor->size + 1; i++)
    {
        if ((Node *)cursor->ptrs[i].nodePtr == child)
        {
            parent = cursor;
            return parent;
        }
        else
        {
            parent = findParent((Node *)cursor->ptrs[i].nodePtr, child);
            if (parent != NULL)
                return parent;
        }
    }
    return parent;
}

// Print the tree
void BPTree::display(Node *cursor, int level)
{
    cout << "level: " << level << endl;
    if (cursor != NULL)
    {
        for (int i = 0; i < cursor->size; i++)
        {
            cout << cursor->keys[i] << " ";
        }
        cout << "\n";
        if (cursor->isLeaf != true)
        {
            for (int i = 0; i < cursor->size + 1; i++)
            {
                display((Node *)cursor->ptrs[i].nodePtr, level + 1);
            }
        }
    }
}
void BPTree::remove(int x, int NODE_KEYS)
{
    if (root == NULL)
    {
        cout << "Tree empty\n";
    }
    else
    {
        Node *cursor = root;
        Node *parent;
        int leftSibling, rightSibling;
        while (cursor->isLeaf == false)
        {
            for (int i = 0; i < cursor->size; i++)
            {
                parent = cursor;
                leftSibling = i - 1;
                rightSibling = i + 1;
                if (x < (cursor->keys)[i])
                {
                    cursor = (Node *)cursor->ptrs[i].nodePtr;
                    break;
                }
                if (i == cursor->size - 1)
                {
                    leftSibling = i;
                    rightSibling = i + 2;
                    cursor = (Node *)cursor->ptrs[i + 1].nodePtr;
                    break;
                }
            }
        }
        bool found = false;
        int pos;
        for (pos = 0; pos < cursor->size; pos++)
        {
            if (cursor->keys[pos] == x)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            cout << "Not found\n";
            return;
        }
        for (int i = pos; i < cursor->size; i++)
        {
            cursor->keys[i] = cursor->keys[i + 1];
        }
        cursor->size--;
        if (cursor == root)
        {
            for (int i = 0; i < NODE_KEYS + 1; i++)
            {
                cursor->ptrs[i].nodePtr = NULL;
            }
            if (cursor->size == 0)
            {
                cout << "Tree died\n";
                delete[] cursor->keys;
                delete[] cursor->ptrs;
                delete cursor;
                root = NULL;
            }
            return;
        }
        cursor->ptrs[cursor->size] = cursor->ptrs[cursor->size + 1];
        cursor->ptrs[cursor->size + 1].nodePtr = NULL;
        if (cursor->size >= (NODE_KEYS + 1) / 2)
        {
            return;
        }
        if (leftSibling >= 0)
        {
            Node *leftNode = (Node *)parent->ptrs[leftSibling].nodePtr;
            if (leftNode->size >= (NODE_KEYS + 1) / 2 + 1)
            {
                for (int i = cursor->size; i > 0; i--)
                {
                    cursor->keys[i] = cursor->keys[i - 1];
                }
                cursor->size++;
                cursor->ptrs[cursor->size] = cursor->ptrs[cursor->size - 1];
                cursor->ptrs[cursor->size - 1].nodePtr = NULL;
                cursor->keys[0] = leftNode->keys[leftNode->size - 1];
                leftNode->size--;
                leftNode->ptrs[leftNode->size].nodePtr = cursor;
                leftNode->ptrs[leftNode->size + 1].nodePtr = NULL;
                parent->keys[leftSibling] = cursor->keys[0];
                return;
            }
        }
        if (rightSibling <= parent->size)
        {
            Node *rightNode = (Node *)parent->ptrs[rightSibling].nodePtr;
            if (rightNode->size >= (NODE_KEYS + 1) / 2 + 1)
            {
                cursor->size++;
                cursor->ptrs[cursor->size] = cursor->ptrs[cursor->size - 1];
                cursor->ptrs[cursor->size - 1].nodePtr = NULL;
                cursor->keys[cursor->size - 1] = rightNode->keys[0];
                rightNode->size--;
                rightNode->ptrs[rightNode->size] = rightNode->ptrs[rightNode->size + 1];
                rightNode->ptrs[rightNode->size + 1].nodePtr = NULL;
                for (int i = 0; i < rightNode->size; i++)
                {
                    rightNode->keys[i] = rightNode->keys[i + 1];
                }
                parent->keys[rightSibling - 1] = rightNode->keys[0];
                return;
            }
        }
        if (leftSibling >= 0)
        {
            Node *leftNode = (Node *)parent->ptrs[leftSibling].nodePtr;
            for (int i = leftNode->size, j = 0; j < cursor->size; i++, j++)
            {
                leftNode->keys[i] = cursor->keys[j];
            }
            leftNode->ptrs[leftNode->size].nodePtr = NULL;
            leftNode->size += cursor->size;
            leftNode->ptrs[leftNode->size] = cursor->ptrs[cursor->size];
            removeInternal(parent->keys[leftSibling], parent, cursor);
            delete[] cursor->keys;
            delete[] cursor->ptrs;
            delete cursor;
        }
        else if (rightSibling <= parent->size)
        {
            Node *rightNode = (Node *)parent->ptrs[rightSibling].nodePtr;
            for (int i = cursor->size, j = 0; j < rightNode->size; i++, j++)
            {
                cursor->keys[i] = rightNode->keys[j];
            }
            cursor->ptrs[cursor->size].nodePtr = NULL;
            cursor->size += rightNode->size;
            cursor->ptrs[cursor->size] = rightNode->ptrs[rightNode->size];
            cout << "Merging two leaf nodes\n";
            removeInternal(parent->keys[rightSibling - 1], parent, rightNode);
            delete[] rightNode->keys;
            delete[] rightNode->ptrs;
            delete rightNode;
        }
    }
}
void BPTree::removeInternal(int x, Node *cursor, Node *child)
{
    if (cursor == root)
    {
        if (cursor->size == 1)
        {
            if (cursor->ptrs[1].nodePtr == child)
            {
                delete[] child->keys;
                delete[] child->ptrs;
                delete child;
                root = (Node *)cursor->ptrs[0].nodePtr;
                delete[] cursor->keys;
                delete[] cursor->ptrs;
                delete cursor;
                cout << "Changed root node\n";
                return;
            }
            else if (cursor->ptrs[0].nodePtr == child)
            {
                delete[] child->keys;
                delete[] child->ptrs;
                delete child;
                root = (Node *)(cursor->ptrs)[1].nodePtr;
                delete[] cursor->keys;
                delete[] cursor->ptrs;
                delete cursor;
                cout << "Changed root node\n";
                return;
            }
        }
    }
}
// Get the root
Node *BPTree::getRoot()
{
    return root;
}

// Get number of nodes
void BPTree::getNoOfNodes(Node *cursor, int *size)
{
    if (cursor != NULL)
    {
        (*size)++;
        if (cursor->isLeaf != true)
        {
            for (int i = 0; i < cursor->size + 1; i++)
            {
                getNoOfNodes((Node *)cursor->ptrs[i].nodePtr, size);
            }
        }
    }
}

// get height
int BPTree::getHeight(Node *cursor)
{
    int h = 0;
    if (cursor == NULL)
    {
        return 0;
    }
    if (cursor->isLeaf == true)
    {
        h = 0;
        return h;
    }
    if (cursor->isLeaf != true)
    {
        h = this->getHeight((Node *)cursor->ptrs[0].nodePtr);
        return h + 1;
    }
    return h;
}

//get root contents
void BPTree::getRootContents(){
    for (int i=0;i<(root->size);i++){
        cout << root->keys[i]<<", ";
    }
    cout <<"\n";
}

//get first child node contents
void BPTree::getRootChildContents(){
    Node *firstChild=(Node *)(root->ptrs[0].nodePtr);
    for (int i=0;i<(firstChild->size);i++){
        cout << firstChild->keys[i]<<", ";
    }
    cout <<"\n";
}