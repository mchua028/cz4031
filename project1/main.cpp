#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "storage.h"
#include "bptree.h"
#include "storage.cpp"
#include "bptree.cpp"

const int SIZE = 1e8;
const int BLOCK_SIZE = 200;
const int RECORD_SIZE = 18;

void importData(Storage &storage, BPTree &bptree, int NODE_KEYS, const char* filename) {
    std::ifstream dataFile(filename);
    std::string line;

    std::getline(dataFile, line); // Skip first line
    while (std::getline(dataFile, line)){
        std::stringstream buffer(line);
        std::string token;
        Record r;

        // Parse line
        std::getline(buffer, token, '\t');
        std::memcpy(r.tconst, token.c_str(), 10);
        buffer >> r.averageRating >> r.numVotes;

        std::byte *recordPtr = storage.insertRecord(r);
        //insert each record into bptree
        bptree.insert(r.numVotes,recordPtr,NODE_KEYS);
        bptree.display(bptree.getRoot(),0);
    }


    dataFile.close();
} 

void experiment1(Storage &storage) {
    std::cout << "---Experiment 1---\n";

    std::cout << "Number of blocks: " << storage.getUsedBlocks() << '\n';
    std::cout << "Size of database: " << storage.getUsedSize() / 1000000.0 << " MB\n";
}

void experiment2(BPTree &bptree, int NODE_KEYS){
    std::cout << "---Experiment 2---\n";
    std::cout <<"parameter n: "<< NODE_KEYS<<endl;
    int noOfNodes=0;
    bptree.getNoOfNodes(bptree.getRoot(),&noOfNodes);
    std::cout <<"Number of nodes of bplus tree:"<<noOfNodes<<endl;
    std::cout <<"Height of bplus tree: "<<bptree.getHeight(bptree.getRoot())<<endl;
    std::cout <<"Root contents:\n";
    bptree.getRootContents();
    std::cout <<"First child node contents:\n";
    bptree.getRootChildContents();
}

void experiment3(Storage &storage, BPTree &bptree, int key){
    std::cout << "---Experiment 3---\n";
    // std::cout << "fetched records for key="<< key << ": " <<endl;
    vector<byte *> recordPtrs=bptree.searchRecords(key);

    vector<int> blockIndexes;
    float avgRating=0;
    
    //get all blocks accessed
    for (int i=0;i<recordPtrs.size();i++){
        byte *recordAdd=recordPtrs[i];
        Record r;
        int blockIdx;
        r=get<0>(storage.getRecord(recordAdd));
        avgRating+=r.averageRating;
        blockIdx=get<1>(storage.getRecord(recordAdd));
        cout << "block index:"<<blockIdx<<endl;
        if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx) == blockIndexes.end() ){
            blockIndexes.push_back(blockIdx);
        }
        uintptr_t intPtr = reinterpret_cast<uintptr_t>(recordAdd);
        uintptr_t storagePtr = reinterpret_cast<uintptr_t>(storage.getStoragePtr());

        if ((intPtr+RECORD_SIZE-1-storagePtr+1)>BLOCK_SIZE){
            if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx+1) == blockIndexes.end() ){
                blockIndexes.push_back(blockIdx+1);
            }
        }

    }

    cout <<"Number of blocks accessed:"<<blockIndexes.size()<<endl;
    //print contents of block
    for (int i=0;i<blockIndexes.size() && i<5;i++){
        cout <<"Contents of block "<<blockIndexes[i]<<":\n";
        vector<string> contents=storage.getBlockContent(blockIndexes[i]);
        for (int j=0;j<contents.size();j++){
            cout << contents[j]<<", ";
        }
        cout <<"\n";
    }
    //calculate average rating
    avgRating/=recordPtrs.size();
    cout <<"Average rating: "<<avgRating<<endl;
    

}

void experiment4(Storage &storage, BPTree &bptree, int startKey, int endKey, int NODE_KEYS){
    std::cout << "---Experiment 4---\n";
    vector<byte *> recordPtrs=bptree.searchRange(startKey,endKey,NODE_KEYS);

    vector<int> blockIndexes;
    float avgRating=0;
    
    //get all blocks accessed
    for (int i=0;i<recordPtrs.size();i++){
        byte *recordAdd=recordPtrs[i];
        Record r;
        int blockIdx;
        r=get<0>(storage.getRecord(recordAdd));
        avgRating+=r.averageRating;
        blockIdx=get<1>(storage.getRecord(recordAdd));
        if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx) == blockIndexes.end() ){
            blockIndexes.push_back(blockIdx);
        }
        uintptr_t intPtr = reinterpret_cast<uintptr_t>(recordAdd);
        uintptr_t storagePtr = reinterpret_cast<uintptr_t>(storage.getStoragePtr());

        if ((intPtr+RECORD_SIZE-1-storagePtr+1)>BLOCK_SIZE){
            if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx+1) == blockIndexes.end() ){
                blockIndexes.push_back(blockIdx+1);
            }
        }

    }
    cout <<"Number of blocks accessed:"<<blockIndexes.size()<<endl;

    //print contents of block
    for (int i=0;i<blockIndexes.size() && i<5;i++){
        cout <<"Contents of block "<<blockIndexes[i]<<":\n";
        vector<string> contents=storage.getBlockContent(blockIndexes[i]);
        for (int j=0;j<contents.size();j++){
            cout << contents[j]<<", ";
        }
        cout <<"\n";
    }
    //calculate average rating
    avgRating/=recordPtrs.size();
    cout <<"Average rating: "<<avgRating<<endl;

}

void experiment5(Storage &storage, BPTree &bptree, int startKey, int endKey, int NODE_KEYS) {
    cout << "Experiment 5" << endl;
    vector<byte *> recordPtrs=bptree.searchRange(startKey,endKey,NODE_KEYS);
    for (int j = 0; j < recordPtrs.size(); j++)
    {
        Record r;
        cout << "fetch recordAdd:" << recordPtrs[j] <<endl;
        r=get<0>(storage.getRecord(recordPtrs[j]));
        if (r.numVotes == 1000) {
            cout << "Record with numvotes = 1000: " << get<0>(storage.getRecord(recordPtrs[j])).tconst << endl;                
            storage.deleteRecord(recordPtrs[j]);
            bptree.remove(j, NODE_KEYS);
        }
    }
    int size = 0;
    cout << "No. of nodes: " << bptree.getNoOfNodes(bptree.getRoot(), &size) << endl;
    ;
    cout << "Root Contents" << endl;
    bptree.getRootContents();
    cout << "Root Child Contents" << endl;
    bptree.getRootChildContents();
}

    int main()
    {
        Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
        BPTree bptree;
        int NODE_KEYS = (BLOCK_SIZE - 16) / 20;

        importData(storage, bptree, NODE_KEYS, "./data2.tsv");

        experiment1(storage);

        experiment2(bptree, NODE_KEYS);

        experiment3(storage, bptree, 262);

        experiment4(storage, bptree, 100, 2000, NODE_KEYS);
        
        experiment5(storage, bptree, 100, 2000, NODE_KEYS);
        // cout <<"Experiment 3:"<<endl;
        // experiment3(storage, bptree, 1645);

        return 0;
}