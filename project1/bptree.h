#pragma once
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

// pointrt vector: a list of address of records wit the same key

// BP node
class Node
{
    bool IS_LEAF;
    int *key, size;
    void **ptr;
    friend class BPTree;

public:
    Node(int);
};

// BP tree
class BPTree
{
    Node *root;
    void insertInternal(int, Node *, Node *, int);
    Node *findParent(Node *, Node *);
    int findSmallestKeyInSubtree(Node *);

public:
    BPTree();
    ~BPTree();
    void *search(int);
    void *search(int, int, int *);
    void insert(int, void *, int);
    void display(Node *, int);
    int get_h(Node *);
    void get_size(Node *, int *);
    Node *getRoot();
    void remove(int x, int MAX);
    void removeInternal(int x, Node *cursor, Node *child);
    void getRootContents();
    void getRootChildContents();
    void cleanUp(Node *);
};
