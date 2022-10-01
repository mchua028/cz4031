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

void importData(Storage &storage, BPTree &bptree, const char* filename) {
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
        bptree.insert(r.numVotes,recordPtr);
        std::cout<<"first BPTREE"<<endl;
        bptree.display(bptree.getRoot(),0);
    }


    dataFile.close();
} 

void experiment1(Storage &storage, BPTree &bptree) {
    std::cout << "---Experiment 1---\n";

    std::cout << "Number of blocks: " << storage.getUsedBlocks() << '\n';
    int noOfNodes=0;
    bptree.getNoOfNodes(bptree.getRoot(),&noOfNodes);
    std::cout << "Size of database: " << (storage.getUsedSize() + noOfNodes*BLOCK_SIZE) / 1000000.0 << " MB\n";
}

void experiment2(BPTree &bptree){
    std::cout << "---Experiment 2---\n";
    std::cout <<"parameter n: "<< bptree.getNodeKeys()<<endl;
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

    vector<byte *> recordPtrs = bptree.searchRecords(key);

    vector<Record> records;
    vector<int> blockIndices;
    std::tie(records, blockIndices) = storage.getRecords(recordPtrs);

    float totalAverageRating = 0;
    for (Record r: records) {
        totalAverageRating += r.averageRating;
    }
    
    std::cout << "Number of blocks accessed:" << blockIndices.size() << endl;

    // Print the content of the first 5 accessed blocks
    for (int i = 0; i < min((int) blockIndices.size(), 5); i++) {
        auto blockContent = storage.getBlockContent(blockIndices[i]);
        for (auto tconst: blockContent) {
            std::cout << tconst << ", ";
        }
        std::cout << '\n';
    }

    std::cout <<"Average rating: "<< totalAverageRating / recordPtrs.size() <<endl;
}

void experiment4(Storage &storage, BPTree &bptree, int startKey, int endKey){
    std::cout << "---Experiment 4---\n";
    vector<byte *> recordPtrs=bptree.searchRange(startKey,endKey);

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
    std::cout <<"Number of blocks accessed:"<<blockIndexes.size()<<endl;

    //print contents of block
    for (int i=0;i<blockIndexes.size() && i<5;i++){
        std::cout <<"Contents of block "<<blockIndexes[i]<<":\n";
        vector<string> contents=storage.getBlockContent(blockIndexes[i]);
        for (int j=0;j<contents.size();j++){
            std::cout << contents[j]<<", ";
        }
        std::cout <<"\n";
    }
    //calculate average rating
    avgRating/=recordPtrs.size();
    std::cout <<"Average rating: "<<avgRating<<endl;

}
void experiment5(Storage &storage, BPTree &bptree, int key) {
    std::cout << "---Experiment 5---" << endl;
    vector<byte *> recordPtrs=bptree.searchRecords(key);
    for (int j = 0; j < recordPtrs.size(); j++)
    {
        Record r;
        r=get<0>(storage.getRecord(recordPtrs[j]));
        std::cout << "Record with numvotes = "<<key<<": " << get<0>(storage.getRecord(recordPtrs[j])).tconst << endl;                
        storage.deleteRecord(recordPtrs[j]);
            
    }
    bptree.remove(key);
    int size = 0;
    bptree.getNoOfNodes(bptree.getRoot(), &size);
    std::cout << "No. of nodes: " << size << endl;
    std::cout << "Root Contents" << endl;
    bptree.getRootContents();
    std::cout << "Root Child Contents" << endl;
    bptree.getRootChildContents();
    std::cout <<"BPTREE after removing "<<key<<":"<<endl;
    bptree.display(bptree.getRoot(),0);
}
int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    BPTree bptree(BLOCK_SIZE);
    
    importData(storage, bptree, "./data2.tsv");

    // experiment1(storage, bptree);

    // experiment2(bptree);

    // experiment3(storage, bptree, 154);

    // experiment4(storage, bptree, 30000,40000);

    experiment5(storage, bptree, 6018);

    experiment5(storage, bptree, 2127);

    experiment5(storage, bptree, 1807);

    experiment5(storage, bptree, 1645);

    experiment5(storage, bptree, 1342);

    experiment5(storage, bptree, 1000);

    return 0;
}