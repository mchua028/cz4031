#pragma once
const int NODE_KEYS = 3;

struct ptrs_struct{
    Node *nodePtr;
    vector <byte*> recordPtrs;
};

class Node {
    friend class BPTree;
    private:
        int size;
        int *keys;
        // If leaf, points to record bytes, else, points to nodes
        // void *ptrs[NODE_KEYS + 1];  //some point to Node, some point to Record
        // vector <void*> *ptrs;
        ptrs_struct *ptrs;
        bool isLeaf;
    
    public:
        Node();
        Node(int i);
        ~Node();
};

class BPTree {
    private:
        Node *root;
        void insertInternal(int, Node *, Node *);
        Node *findParent(Node *, Node *);

    public:
        BPTree();
        ~BPTree();
        void insert(int key, byte* recordPtr);
        Node* search(int key);
        void searchRange(int startKey, int endKey);
        void remove();
        void display(Node *);
        Node *getRoot();
};