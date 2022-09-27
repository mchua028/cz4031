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

Node::Node(){
    keys = new int[NODE_KEYS];
    ptrs = new ptrs_struct[NODE_KEYS+ 1];
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
    // delete[] root;
}

// Search operation
Node* BPTree::search(int x)
{
    if (root == NULL)
    {
        cout << "Tree is empty\n";
        return NULL;
    }
    else
    {
        Node *cursor = root;
        while (cursor->isLeaf == false)
        {
            for (int i = 0; i < cursor->size; i++)
            {
                if (x < cursor->keys[i])
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
        for (int i = 0; i < cursor->size; i++)
        {
            if (cursor->keys[i] == x)
            {
                cout << "Found\n";
                return (Node *)cursor->ptrs[i].nodePtr;
            }
        }
        cout << "Not found\n";
        return NULL;
    }
}

// Insert Operation
void BPTree::insert(int key, byte *recordAdd)
{
    if (root == NULL) // if no root
    {
        void *test;
        root = new Node();
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
        //check if key already exist in b+ tree
        Node* searchKey;
        searchKey = search(key);
        //if key already exist in b+ tree
        if (searchKey != nullptr){
            for(int i = 0; i < searchKey->size; i++)
            {
                if(searchKey->keys[i] == key)
                {
                    searchKey->ptrs[i].recordPtrs.push_back(recordAdd);
                    break;
                }
            }
            return;
        }

        //if key does not already exist in b+ tree
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
            cursor->ptrs[i]=newptr; 
            cursor->size++;
            // cursor->ptrs[cursor->size] = cursor->ptrs[cursor->size - 1]; // update the pointer to next leaf
            // cursor->ptrs[cursor->size - 1] = NULL;
        }
        else // if the leaf node is full
        {
            Node *newLeaf = new Node();
            // int virtualNode[MAX + 1]; // create an array of size max+1 to store the keys, including the new key
            Node *virtualNode = new Node(NODE_KEYS + 1); // create a virtualnode with 1 more key+pointer
            //copy contents of current leaf node to virtualNode
            for (int i = 0; i < NODE_KEYS; i++)
            {
                // virtualNode[i] = cursor->key[i];
                virtualNode->keys[i] = cursor->keys[i];
                virtualNode->ptrs[i] = cursor->ptrs[i];
            }
            //find position to insert new key
            int i = 0, j;
            while (key > virtualNode->keys[i] && i < NODE_KEYS) // i = index of first key larger than key
                i++;
            //make space for new key
            for (int j = NODE_KEYS; j > i; j--)
            {
                virtualNode->keys[j] = virtualNode->keys[j - 1];
                virtualNode->ptrs[j] = virtualNode->ptrs[j - 1];
            }
            virtualNode->keys[i] = key;         // replace key
            ptrs_struct newptr;
            newptr.recordPtrs.push_back(recordAdd);
            virtualNode->ptrs[i] = newptr; // replace recordaddress

            newLeaf->isLeaf = true;
            cursor->size = (NODE_KEYS + 1) / 2;
            newLeaf->size = NODE_KEYS + 1 - (NODE_KEYS + 1) / 2;        // splitting the node into 2 and deciding th sizes
            newLeaf->ptrs[NODE_KEYS] = cursor->ptrs[NODE_KEYS];     // exhange pointers to next leaf
            cursor->ptrs[NODE_KEYS].nodePtr = newLeaf;            // update pointer to next leaf node
             
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

            if (cursor == root)
            {
                Node *newRoot = new Node();
                newRoot->keys[0] = newLeaf->keys[0];
                newRoot->ptrs[0].nodePtr = cursor;
                newRoot->ptrs[1].nodePtr = newLeaf;
                newRoot->isLeaf = false;
                newRoot->size = 1;
                root = newRoot;
            }
            else
            {
                insertInternal(newLeaf->keys[0], parent, newLeaf);
            }
        }
    }
}

// Insert Operation
void BPTree::insertInternal(int x, Node *cursor, Node *child)
{
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
    else
    {
        Node *newInternal = new Node();
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
        //find position to insert x
        int i = 0, j;
        while (x > virtualKey[i] && i < NODE_KEYS)
            i++;
        //make space for x
        for (int j = NODE_KEYS; j > i; j--)
        {
            virtualKey[j] = virtualKey[j - 1];
        }
        virtualKey[i] = x;
        //make space for pointer to child
        for (int j = NODE_KEYS + 1; j > i + 1; j--)
        {
            virtualPtr[j] = virtualPtr[j - 1];
        }
        virtualPtr[i + 1] = child;

        newInternal->isLeaf = false;

        cursor->size = (NODE_KEYS+ 1) / 2;
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
        //assign keys and ptrs of newInternal
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
            Node *newRoot = new Node();
            newRoot->keys[0] = cursor->keys[cursor->size];
            newRoot->ptrs[0].nodePtr = cursor;
            newRoot->ptrs[1].nodePtr = newInternal;
            newRoot->isLeaf = false;
            newRoot->size = 1;
            root = newRoot;
        }
        else
        {
            insertInternal(cursor->keys[cursor->size], findParent(root, cursor), newInternal);
        }
    }
}

// Find the parent
Node *BPTree::findParent(Node *cursor, Node *child)
{
    Node *parent;
    if (cursor->isLeaf || ((Node *)cursor->ptrs[0].nodePtr)->isLeaf)
    {
        return NULL;
    }
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
void BPTree::display(Node *cursor)
{
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
                display((Node *)cursor->ptrs[i].nodePtr);
            }
        }
    }
}

// Get the root
Node *BPTree::getRoot()
{
    return root;
}