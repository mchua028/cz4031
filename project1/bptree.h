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
    void insertInternal(int, Node *, Node *, int);
    int findSmallestKeyInSubtree(Node *);
    Node *findParent(Node *, Node *);
    void removeInternal(int, Node *, Node *);

public:
    BPTree();
    ~BPTree();
    void insert(int key, byte *recordPtr, int NODE_KEYS);
    Node *search(int key);
    vector<byte *> searchRecords(int key);
    vector<byte *> searchRange(int startKey, int endKey, int NODE_KEYS);
    void remove(int x, int NODE_KEYS);
    void display(Node *, int);
    Node *getRoot();
    void getNoOfNodes(Node *, int *);
    int getHeight(Node *);
    void getRootContents();
    void getRootChildContents();
    void cleanUp(Node *);
};