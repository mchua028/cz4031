const int NODE_KEYS = 3;

class Node {
    friend class BPTree;
    private:
        int size;
        int keys[NODE_KEYS];
        // If leaf, points to record bytes, else, points to nodes
        void *ptrs[NODE_KEYS + 1];
        bool isLeaf;
    
    public:
        Node();
        ~Node();
};

class BPTree {
    private:
        Node *root;

    public:
        BPTree();
        ~BPTree();
        void build();
        void search();
        void remove();
};