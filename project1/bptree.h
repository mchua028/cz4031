#pragma once
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

int MAX = 3;

// BP node
class Node
{
    bool IS_LEAF;
    int *key, size;
    void **ptr;
    friend class BPTree;

public:
    Node();
    Node(int);
};

// BP tree
class BPTree
{
    Node *root;
    void insertInternal(int, Node *, Node *);
    Node *findParent(Node *, Node *);

public:
    BPTree();
    ~BPTree();
    void *search(int);
    void insert(int, void *, int);
    void display(Node *);
    Node *getRoot();
};
