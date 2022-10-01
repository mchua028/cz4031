// Searching on a B+ tree in C++

#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "bptree.h"
using namespace std;

Node::Node(int i)
{
    key = new int[i];
    ptr = (void **)new void *[i + 1];
}

BPTree::BPTree()
{
    root = NULL;
}

BPTree::~BPTree()
{
    delete (root);
}

// Search operation
void *BPTree::search(int x)
{
    if (root == NULL)
    {
        cout << "Tree is empty\n";
        return NULL;
    }
    else
    {
        Node *cursor = root;
        while (cursor->IS_LEAF == false)
        {
            for (int i = 0; i < cursor->size; i++)
            {
                if (x < cursor->key[i])
                {
                    cursor = (Node *)cursor->ptr[i];
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptr[i + 1];
                    break;
                }
            }
        }
        for (int i = 0; i < cursor->size; i++)
        {
            if (cursor->key[i] == x)
            {
                // cout << "Found\n";
                return cursor->ptr[i];
            }
        }
        // cout << "Not found\n";
        return NULL;
    }
}

// Insert Operation
void BPTree::insert(int x, void *recordAdd, int MAX)
{

    if (root == NULL) // if no root
    {
        void *test;
        root = new Node(MAX);
        root->key[0] = x;
        //
        vector<void *> *v = new vector<void *>; // insertion point 1
        root->ptr[0] = v;
        v->push_back(recordAdd);
        //
        //
        // cout << "address of vector: " << &v << endl;
        // cout << "size of vector: " << v->size() << endl;
        //
        root->IS_LEAF = true;
        root->size = 1;
        // delete v;
    }
    else // if root exsists
    {
        Node *cursor = root;
        Node *parent;
        while (cursor->IS_LEAF == false) // traverse to the leaf level
        {
            parent = cursor;
            for (int i = 0; i < cursor->size; i++)
            {
                if (x < cursor->key[i])
                {
                    cursor = (Node *)cursor->ptr[i];
                    break;
                }
                if (i == cursor->size - 1)
                {
                    cursor = (Node *)cursor->ptr[i + 1];
                    break;
                }
            }
        }
        if (cursor->size < MAX) // if this leaf node is not full
        {
            // check if key already exist,
            vector<void *> *V = (vector<void *> *)this->search(x);
            // cout << "from search: " << this->search(x) << endl;
            //  cout << "V size:" << V->size() << endl;

            if (V != NULL) // if key already exist
            {
                // cout << "entered" << endl;
                V->push_back(recordAdd); // insertion point
                return;
            }

            int i = 0;
            while (x > cursor->key[i] && i < cursor->size) // find the index of the first key that is larger than x
                i++;
            for (int j = cursor->size; j > i; j--) // copy the keys from the back to make space for new key insertion point
            {
                cursor->key[j] = cursor->key[j - 1];
                cursor->ptr[j] = cursor->ptr[j - 1];
            }

            cursor->key[i] = x;
            cursor->size++;
            cursor->ptr[cursor->size] = cursor->ptr[cursor->size - 1]; // update the pointer to next leaf

            vector<void *> *v = new vector<void *>;
            cursor->ptr[i] = v;
            v->push_back(recordAdd);
        }
        else // if the leaf node is full
        {
            // check if key already exist,  // insertion point
            vector<void *> *V = (vector<void *> *)this->search(x);
            // cout << "from search: " << this->search(x) << endl;
            //  cout << "V size:" << V->size() << endl;

            if (V != NULL) // if key already exists
            {
                // cout << "entered" << endl;
                V->push_back(recordAdd); // insertion point
                return;
            }

            Node *newLeaf = new Node(MAX);
            // int virtualNode[MAX + 1]; // create an array of size max+1 to store the keys, including the new key
            Node *virtualNode = new Node(MAX + 1); // create a virtualnode with 1 more key+pointer
            for (int i = 0; i < MAX; i++)
            {
                // virtualNode[i] = cursor->key[i];
                virtualNode->key[i] = cursor->key[i];
                virtualNode->ptr[i] = cursor->ptr[i];
            }
            int i = 0, j;
            while (x > virtualNode->key[i] && i < MAX) // i = index of first key larger than x
                i++;
            for (int j = MAX + 1; j > i; j--)
            {
                virtualNode->key[j] = virtualNode->key[j - 1];
                virtualNode->ptr[j] = virtualNode->ptr[j - 1];
            }
            virtualNode->key[i] = x; // slot in x
            //
            vector<void *> *v = new vector<void *>; // insertion point
            virtualNode->ptr[i] = v;
            v->push_back(recordAdd);
            //
            newLeaf->IS_LEAF = true;
            cursor->size = (MAX + 1) / 2;
            newLeaf->size = MAX + 1 - (MAX + 1) / 2;        // splitting the node into 2 and deciding th sizes
            cursor->ptr[cursor->size] = newLeaf;            // update pointer to next leaf node
            newLeaf->ptr[newLeaf->size] = cursor->ptr[MAX]; // exhange pointers to next leaf
            cursor->ptr[MAX] = NULL;                        // remove original link to next leaf
            for (i = 0; i < cursor->size; i++)
            {
                cursor->key[i] = virtualNode->key[i]; // transfering keys into old node insertion point 3
                cursor->ptr[i] = virtualNode->ptr[i]; // transfering ptrs into old node insertion point 3
            }
            for (i = 0, j = cursor->size; i < newLeaf->size; i++, j++)
            {
                newLeaf->key[i] = virtualNode->key[j]; // transfering keys into new node insertion point 4
                newLeaf->ptr[i] = virtualNode->ptr[j]; // transfering keys into new node insertion point 4
            }
            if (cursor == root)
            {
                Node *newRoot = new Node(MAX);
                newRoot->key[0] = newLeaf->key[0];
                newRoot->ptr[0] = cursor;
                newRoot->ptr[1] = newLeaf;
                newRoot->IS_LEAF = false;
                newRoot->size = 1;
                root = newRoot;
            }
            else
            {
                insertInternal(newLeaf->key[0], parent, newLeaf, MAX);
            }
        }
    }
}

// Insert Operation          x:newleaf 1st key, cursor: parent, child:newleaf
void BPTree::insertInternal(int x, Node *cursor, Node *child, int MAX)
{
    if (cursor->size < MAX) // if parent is not full
    {
        int i = 0;
        while (x > cursor->key[i] && i < cursor->size)
            i++;
        for (int j = cursor->size; j > i; j--)
        {
            cursor->key[j] = cursor->key[j - 1];
        }
        for (int j = cursor->size + 1; j > i + 1; j--)
        {
            cursor->ptr[j] = cursor->ptr[j - 1];
        }
        cursor->key[i] = x;
        cursor->size++;
        cursor->ptr[i + 1] = child;
    }
    else
    {
        Node *newInternal = new Node(MAX);
        int virtualKey[MAX + 1];
        Node *virtualPtr[MAX + 2];
        for (int i = 0; i < MAX; i++)
        {
            virtualKey[i] = cursor->key[i];
        }
        for (int i = 0; i < MAX + 1; i++)
        {
            virtualPtr[i] = (Node *)cursor->ptr[i];
        }
        // find position to insert x
        int i = 0, j;
        while (x > virtualKey[i] && i < MAX)
            i++;
        // make space for x
        for (int j = MAX; j > i; j--)
        {
            virtualKey[j] = virtualKey[j - 1];
        }
        virtualKey[i] = x;
        // make space for pointer to child
        for (int j = MAX + 1; j > i + 1; j--)
        {
            virtualPtr[j] = virtualPtr[j - 1];
        }
        virtualPtr[i + 1] = child;

        newInternal->IS_LEAF = false;

        cursor->size = (MAX + 1) / 2;
        newInternal->size = MAX - (MAX + 1) / 2;

        // assign key and ptrs of cursor
        for (i = 0; i < cursor->size; i++)
        {
            cursor->key[i] = virtualKey[i];
        }
        for (i = 0; i < cursor->size + 1; i++)
        {
            cursor->ptr[i] = virtualPtr[i];
        }
        // assign keys and ptrs of newInternal
        for (i = 0, j = cursor->size + 1; i < newInternal->size; i++, j++)
        {
            newInternal->key[i] = virtualKey[j];
        }
        for (i = 0, j = cursor->size + 1; i < newInternal->size + 1; i++, j++)
        {
            newInternal->ptr[i] = virtualPtr[j];
        }

        if (cursor == root)
        {
            Node *newRoot = new Node(MAX);
            // newRoot->keys[0] = cursor->keys[cursor->size];
            // get the smallest key found in the subtree under newInternal
            newRoot->key[0] = findSmallestKeyInSubtree(newInternal);
            newRoot->ptr[0] = cursor;
            newRoot->ptr[1] = newInternal;
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            root = newRoot;
        }
        else // there are more than 2 levels in the current tree
        {
            insertInternal(findSmallestKeyInSubtree(newInternal), findParent(root, cursor), newInternal, MAX);
        }
    }
}

// Find the parent
Node *BPTree::findParent(Node *cursor, Node *child)
{
    Node *parent;
    if (cursor->IS_LEAF || ((Node *)cursor->ptr[0])->IS_LEAF)
    {
        return NULL;
    }
    for (int i = 0; i < cursor->size + 1; i++)
    {
        if (cursor->ptr[i] == child)
        {
            parent = cursor;
            return parent;
        }
        else
        {
            parent = findParent((Node *)cursor->ptr[i], child);
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
            cout << cursor->key[i] << " ";
        }
        cout << "\n";
        if (cursor->IS_LEAF != true)
        {
            for (int i = 0; i < cursor->size + 1; i++)
            {
                display((Node *)cursor->ptr[i], level + 1);
            }
        }
    }
}

// get height
int BPTree::get_h(Node *cursor)
{
    int h = 0;
    if (cursor == NULL)
    {
        return 0;
    }
    if (cursor->IS_LEAF == true)
    {
        h = 0;
        return h;
    }
    if (cursor->IS_LEAF != true)
    {
        h = this->get_h((Node *)cursor->ptr[0]);
        return h + 1;
    }
    return h;
}

// Get the size
void BPTree::get_size(Node *cursor, int *size)
{
    if (cursor != NULL)
    {
        (*size)++;

        if (cursor->IS_LEAF != true)
        {
            for (int i = 0; i < cursor->size + 1; i++)
            {
                get_size((Node *)cursor->ptr[i], size);
            }
        }
    }
}

// Get the root
Node *BPTree::getRoot()
{
    return root;
}

// find smallest key in subtree
int BPTree::findSmallestKeyInSubtree(Node *cursor)
{
    int smallestKey;
    while (!cursor->IS_LEAF)
    {
        cursor = (Node *)cursor->ptr[0];
    }
    smallestKey = cursor->key[0];
    return smallestKey;
}