#pragma once
using namespace std;
// const int NODE_KEYS = 3;

struct ptrs_struct
{
    void *nodePtr = NULL;
    vector<byte *> recordPtrs;
};

class Node
{

    friend class BPTree;

private:
    int size;
    int *keys;
    ptrs_struct *ptrs;
    bool isLeaf;

public:
    Node();
    Node(int i);
    ~Node();
};

class BPTree
{
private:
    Node *root;
    int NODE_KEYS;
    void insertInternal(int, Node *, Node *);
    int findSmallestKeyInSubtree(Node *);
    Node *findParent(Node *, Node *);
    void removeInternal(int, Node *, Node *);

public:
    BPTree(int);
    ~BPTree();
    void insert(int key, byte *recordPtr);
    Node *search(int key);
    vector<byte *> searchRecords(int key);
    vector<byte *> searchRange(int startKey, int endKey);
    void remove(int x);
    void display(Node *, int);
    Node *getRoot();
    int getNodeKeys();
    void getNoOfNodes(Node *, int *);
    int getHeight(Node *);
    void getRootContents();
    void getRootChildContents();
    void cleanUp(Node *);
};