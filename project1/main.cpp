#include <climits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include "bptree.h"
#include "bptree.cpp"
using namespace std;

// Global variables
int memSize = 100000000;
int blkSize = 500;

// block structure declaration
struct block
{ // header 14 Bytes
    int ID;
    int byteLeft;
    int blockEnd;
    bool isFull;
    bool inUsed;

    // manually update blkSize here
    char data[200 - sizeof(int) * 3 - sizeof(bool) - 3];
};

struct record
{ // size = 18 Bytes
    char tconst[10];
    float averageRating;
    int numVotes;
};

void *init_disk()
{
    cout << "................Initializing Disk................" << endl
         << endl;

    // allocating memory for the "Disk"
    cout << "..........Assigning memory for the Disk.........." << endl
         << endl;
    void *memStart = malloc(memSize);

    // assigning block header for every block
    cout << ".............Initializing the Blocks............." << endl
         << endl;
    cout << "...............Block size:" << blkSize << " Bytes.............." << endl
         << endl;
    void *ptr = memStart;

    // assigning header to the blocks
    for (int i = 0; i < memSize / blkSize; i++)
    {
        // Block ID
        *(int *)ptr = i;
        // isFull flag
        *(bool *)(ptr + sizeof(int)) = false;
        // inUse flag
        *(bool *)(ptr + sizeof(int) + sizeof(bool)) = false;
        // offset to start of free space in block
        *(int *)(ptr + sizeof(int) + sizeof(bool) * 2) = sizeof(int) * 3 + sizeof(bool) * 2;
        // Byte left in block
        *(int *)(ptr + sizeof(int) * 2 + sizeof(bool) * 2) = blkSize - sizeof(int) * 3 - sizeof(bool) * 2;
        // address of start of data
        for (int j = 0; j < blkSize - sizeof(int) * 3 - sizeof(bool) * 2; j++)
        {
            *(char *)(ptr + sizeof(int) * 3 + sizeof(bool) * 2 + j) = '\0';
        }

        ptr = ptr + blkSize;
    }

    cout << "............Disk initialization done!............"
         << endl
         << endl
         << "............" << memSize / blkSize << " Blocks initialized............" << endl;
    return memStart;
}

BPTree read_record(void *memStart, int blkSize, int MAX, int *blkUsed)
{

    // Create a text string, which is used to output the text file
    string line;
    vector<string> record;
    string field, temp;
    char tconst[10];
    float averageRating;
    int numVotes;
    int recordSize = sizeof(tconst) + sizeof(averageRating) + sizeof(numVotes);
    int *blkID;
    bool *isFull;
    bool *inUse;
    int *offSetToFreeSpaceInBlk;
    int *BytesLeft;
    void *startFreeSpace;
    BPTree node;
    // Read from the text file
    ifstream MyReadFile("data.tsv");

    blkID = (int *)(memStart);
    isFull = (bool *)(memStart + sizeof(int));
    inUse = (bool *)(memStart + sizeof(bool) + sizeof(int));
    offSetToFreeSpaceInBlk = (int *)(memStart + sizeof(bool) * 2 + sizeof(int));
    BytesLeft = (int *)(memStart + sizeof(bool) * 2 + sizeof(int) * 2);
    startFreeSpace = (memStart + sizeof(bool) * 2 + sizeof(int) * 3);

    (*blkUsed)++;

    // Use a while loop together with the getline() function to read the file line by line
    // skip first line using 1 read
    getline(MyReadFile, line);
    while (getline(MyReadFile, line))
    {

        record.clear();
        stringstream s(line);
        while (getline(s, field, '\t'))
        {

            // add all the column data
            // of a row to a vector
            record.push_back(field);
        }

        // Inserting record into memory
        if (*BytesLeft >= recordSize)
        {
            *inUse = true;
            for (int i = 0; i < 10; i++)
            {
                *(char *)(startFreeSpace + i) = record[0][i];
            }
            startFreeSpace = startFreeSpace + 10;
            *(float *)startFreeSpace = stof(record[1]);
            startFreeSpace = startFreeSpace + 4;
            *(int *)startFreeSpace = stoi(record[2]);
            node.insert(stoi(record[2]), startFreeSpace - 14, MAX);
            *BytesLeft = *BytesLeft - recordSize;
            *offSetToFreeSpaceInBlk = *offSetToFreeSpaceInBlk + recordSize;

            startFreeSpace = startFreeSpace + 4;
        }
        else
        {
            *isFull = true;

            memStart = memStart + blkSize;
            blkID = (int *)(memStart);
            isFull = (bool *)(memStart + sizeof(int));
            inUse = (bool *)(memStart + sizeof(bool) + sizeof(int));
            offSetToFreeSpaceInBlk = (int *)(memStart + sizeof(bool) * 2 + sizeof(int));
            BytesLeft = (int *)(memStart + sizeof(bool) * 2 + sizeof(int) * 2);
            startFreeSpace = (memStart + sizeof(bool) * 2 + sizeof(int) * 3);

            (*blkUsed)++;

            *inUse = true;
            for (int i = 0; i < 10; i++)
            {
                *(char *)(startFreeSpace + i) = record[0][i];
            }
            startFreeSpace = startFreeSpace + 10;
            *(float *)startFreeSpace = stof(record[1]);
            startFreeSpace = startFreeSpace + 4;
            *(int *)startFreeSpace = stoi(record[2]);
            node.insert(stoi(record[2]), startFreeSpace - 14, MAX);
            *BytesLeft = *BytesLeft - recordSize;
            *offSetToFreeSpaceInBlk = *offSetToFreeSpaceInBlk + recordSize;

            startFreeSpace = startFreeSpace + 4;
        }
    }
    cout << "last record inserted: " << record[0] << endl;

    // Close the file
    MyReadFile.close();
    return node;
}
/*void experiment1(Storage &storage, BPTree &bptree)
{
    std::cout << "---Experiment 1---\n";

    std::cout << "Number of blocks: " << storage.getUsedBlocks() << '\n';
    int noOfNodes = 0;
    bptree.getNoOfNodes(bptree.getRoot(), &noOfNodes);
    std::cout << "Size of database: " << (storage.getUsedSize() + noOfNodes * BLOCK_SIZE) / 1000000.0 << " MB\n";
}*/

void experiment2(BPTree bptree)
{
    std::cout << "---Experiment 2---\n";
    // std::cout << "parameter n: " << bptree.getNodeKeys() << endl;
    int noOfNodes = 0;
    bptree.get_size(bptree.getRoot(), &noOfNodes);
    std::cout << "Number of nodes of bplus tree:" << noOfNodes << endl;
    std::cout << "Height of bplus tree: " << bptree.get_h(bptree.getRoot()) << endl;
    std::cout << "Root contents:\n";
    bptree.getRootContents();
    std::cout << "First child node contents:\n";
    bptree.getRootChildContents();
}

void experiment3(void *start, BPTree bptree, int key)
{
    std::cout << "---Experiment 3---\n";

    // part 1
    int nodeCount = 0;
    cout << "the number and the content of index nodes the process accesses: " << endl;
    // retrieve the address of the vector of the record, search function prints the node content
    // vector<void *> *records = (vector<void *> *)bptree.search(key, 3, &nodeCount);
    void *records = bptree.search(key, 3, &nodeCount);
    cout << "number of node accessed: " << nodeCount << endl;
    cout << endl;
    float totalAverageRating = 0;
    // printing the records
    void *rAdd;
    if (records != NULL)
    {
        for (int i = 0; i < ((vector<void *> *)records)->size(); i++)
        {
            rAdd = ((vector<void *> *)records)->at(i);
            for (int j = 0; j < 10; j++)
            {
                cout << (*(char *)(rAdd + j));
            }
            cout << endl
                 << *(float *)(rAdd + 10) << endl
                 << *(int *)(rAdd + 14) << endl
                 << endl;
            // increment totalaveragerating
            totalAverageRating += *(float *)(rAdd + 10);
        }
    }
    // print avarage rating
    cout << "Average rating: " << totalAverageRating / ((vector<void *> *)records)->size() << endl;

    // number of memory block accessed
    int blk = (records - start) / blkSize;
    cout << "accessing memorry block: " << blk + 1 << endl;
    cout << "content of the block: " << endl;
    void *ptr = start + blk * blkSize;
    // Block ID
    cout << "block id: " << *(int *)ptr << endl;
    // isFull flag
    cout << "isFull: " << *(bool *)(ptr + sizeof(int)) << endl;
    // inUse flag
    cout << "inUse: " << *(bool *)(ptr + sizeof(int) + sizeof(bool)) << endl;
    // offset to start of free space in block
    cout << "offset to start of free space in block: " << *(int *)(ptr + sizeof(int) + sizeof(bool) * 2) << endl;
    // Byte left in block
    cout << "Byte left in block: " << *(int *)(ptr + sizeof(int) * 2 + sizeof(bool) * 2) << endl;
    cout << "records: " << endl;
    /*for (int j = 0; j < blkSize - sizeof(int) * 3 - sizeof(bool) * 2; j++)
    {

    }*/
}

/*void experiment4(Storage &storage, BPTree &bptree, int startKey, int endKey)
{
    std::cout << "---Experiment 4---\n";
    vector<byte *> recordPtrs = bptree.searchRange(startKey, endKey);

    vector<int> blockIndexes;
    float avgRating = 0;

    // get all blocks accessed
    for (int i = 0; i < recordPtrs.size(); i++)
    {
        byte *recordAdd = recordPtrs[i];
        Record r;
        int blockIdx;
        r = get<0>(storage.getRecord(recordAdd));
        avgRating += r.averageRating;
        blockIdx = get<1>(storage.getRecord(recordAdd));
        if (std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx) == blockIndexes.end())
        {
            blockIndexes.push_back(blockIdx);
        }
        uintptr_t intPtr = reinterpret_cast<uintptr_t>(recordAdd);
        uintptr_t storagePtr = reinterpret_cast<uintptr_t>(storage.getStoragePtr());

        if ((intPtr + RECORD_SIZE - 1 - storagePtr + 1) > BLOCK_SIZE)
        {
            if (std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx + 1) == blockIndexes.end())
            {
                blockIndexes.push_back(blockIdx + 1);
            }
        }
    }
    cout << "Number of blocks accessed:" << blockIndexes.size() << endl;

    // print contents of block
    for (int i = 0; i < blockIndexes.size() && i < 5; i++)
    {
        cout << "Contents of block " << blockIndexes[i] << ":\n";
        vector<string> contents = storage.getBlockContent(blockIndexes[i]);
        for (int j = 0; j < contents.size(); j++)
        {
            cout << contents[j] << ", ";
        }
        cout << "\n";
    }
    // calculate average rating
    avgRating /= recordPtrs.size();
    cout << "Average rating: " << avgRating << endl;
}

void experiment5(Storage &storage, BPTree &bptree, int startKey, int endKey)
{
    cout << "Experiment 5" << endl;
    vector<byte *> recordPtrs = bptree.searchRange(startKey, endKey);
    for (int j = 0; j < recordPtrs.size(); j++)
    {
        Record r;
        cout << "fetch recordAdd:" << recordPtrs[j] << endl;
        r = get<0>(storage.getRecord(recordPtrs[j]));
        if (r.numVotes == 1000)
        {
            cout << "Record with numvotes = 1000: " << get<0>(storage.getRecord(recordPtrs[j])).tconst << endl;
            storage.deleteRecord(recordPtrs[j]);
            bptree.remove(j);
        }
    }
    int size = 0;
    bptree.getNoOfNodes(bptree.getRoot(), &size);
    cout << "No. of nodes: " << size << endl;
    cout << "Root Contents" << endl;
    bptree.getRootContents();
    cout << "Root Child Contents" << endl;
    bptree.getRootChildContents();
}*/

int main()
{
    void *start = init_disk();
    void *ptr = start;
    // ptr = memStart;
    // calculate tree node size based on blksize so a node is bounded by blksize
    // x = max no of keys, 4(x) + 8(x+1) <= blksize
    int max = (blkSize - 8) / 12;
    //
    // max = 3;
    //
    int blkUsed = 0;
    BPTree tree = read_record(start, blkSize, max, &blkUsed);
    cout << "number of block used: " << blkUsed << endl;

    // tree.display(tree.getRoot(), 0);
    int height = 0;
    height = tree.get_h(tree.getRoot());
    cout << "height: " << height << endl;
    int size = 0;
    tree.get_size(tree.getRoot(), &size);
    cout << "tree size: " << size << endl;
    int c;
    cout << "enter the numVote to retrive the record or -1 to exit: " << endl
         << endl;
    cin >> c;
    while (c != -1)
    {
        vector<void *> *record = (vector<void *> *)tree.search(c);

        void *rAdd;
        if (record != NULL)
        {
            for (int i = 0; i < record->size(); i++)
            {
                rAdd = record->at(i);
                for (int j = 0; j < 10; j++)
                {
                    cout << (*(char *)(rAdd + j));
                }
                cout << endl
                     << *(float *)(rAdd + 10) << endl
                     << *(int *)(rAdd + 14) << endl
                     << endl;
            }
        }

        cout << "enter the numVote to retrive the record or -1 to exit: " << endl
             << endl;
        cin >> c;
        cout << endl;
    }

    // Exp
    // experiment1(storage, bptree);

    experiment2(tree);

    experiment3(start, tree, 500);

    /*experiment4(storage, bptree, 100, 2000);

    experiment5(storage, bptree, 100, 2000);*/

    // free(record);
    free(start);
    //~BPTree;
    return 0;
}