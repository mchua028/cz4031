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
        // cout << "tconst: " <<r.tconst<<", avgrating: "<<r.averageRating<<", numVotes: "<<r.numVotes<<endl;
        // cout << "insert recordAdd: "<< recordPtr <<endl;
        //insert each record into bptree
        bptree.insert(r.numVotes,recordPtr);
        bptree.display(bptree.getRoot(),0);
    }


    dataFile.close();
} 

void experiment1(Storage &storage) {
    std::cout << "---Experiment 1---\n";

    std::cout << "Number of blocks: " << storage.getUsedBlocks() << '\n';
    std::cout << "Size of database: " << storage.getUsedSize() / 1000000.0 << " MB\n";
}

void experiment2(BPTree &bptree){

}

void experiment3(Storage &storage, BPTree &bptree, int key){
    std::cout << "---Experiment 3---\n";
    // std::cout << "fetched records for key="<< key << ": " <<endl;
    vector<byte *> recordPtrs=bptree.searchRecords(key);

    vector<int> blockIndexes;
    int avgNumVotes=0;
    
    //get all blocks accessed
    for (int i=0;i<recordPtrs.size();i++){
        byte *recordAdd=recordPtrs[i];
        // Record r;
        // std::cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        // r=get<0>(storage.getRecord(recordPtrs[i]));

        // std::cout << r.tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
        Record r;
        int blockIdx;
        r=get<0>(storage.getRecord(recordAdd));
        avgNumVotes+=r.numVotes;
        blockIdx=get<1>(storage.getRecord(recordAdd));
        if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx) == blockIndexes.end() ){
            blockIndexes.push_back(blockIdx);
        }
        uintptr_t intPtr = reinterpret_cast<uintptr_t>(recordAdd);

        if (((intPtr+RECORD_SIZE)/BLOCK_SIZE)>blockIdx){
            if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx+1) == blockIndexes.end() ){
                blockIndexes.push_back(blockIdx+1);
            }
        }

    }

    //print contents of block
    for (int i=0;i<blockIndexes.size() && i<5;i++){
        cout <<"Contents of block "<<blockIndexes[i]<<":\n";
        vector<string> contents=storage.getBlockContent(blockIndexes[i]);
        for (int j=0;j<contents.size();j++){
            cout << contents[i]<<", ";
        }
        cout <<"\n";
    }

    //calculate average numVotes
    avgNumVotes/=recordPtrs.size();
    cout <<"Average number of votes: "<<avgNumVotes<<endl;
    

}

void experiment4(Storage &storage, BPTree &bptree, int startKey, int endKey){
    std::cout << "---Experiment 4---\n";
    vector<byte *> recordPtrs=bptree.searchRange(startKey,endKey);

    vector<int> blockIndexes;
    int avgNumVotes=0;

    // int blockSize=BLOCK_SIZE;
    // int recordSize=RECORD_SIZE;
    
    //get all blocks accessed
    for (int i=0;i<recordPtrs.size();i++){
        byte *recordAdd=recordPtrs[i];
        // Record r;
        // std::cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        // r=get<0>(storage.getRecord(recordPtrs[i]));

        // std::cout << r.tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
        Record r;
        int blockIdx;
        r=get<0>(storage.getRecord(recordAdd));
        avgNumVotes+=r.numVotes;
        blockIdx=get<1>(storage.getRecord(recordAdd));
        if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx) == blockIndexes.end() ){
            blockIndexes.push_back(blockIdx);
        }
        uintptr_t intPtr = reinterpret_cast<uintptr_t>(recordAdd);

        if (((intPtr+RECORD_SIZE)/BLOCK_SIZE)>blockIdx){
            if ( std::find(blockIndexes.begin(), blockIndexes.end(), blockIdx+1) == blockIndexes.end() ){
                blockIndexes.push_back(blockIdx+1);
            }
        }

    }

    //print contents of block
    for (int i=0;i<blockIndexes.size() && i<5;i++){
        cout <<"Contents of block "<<blockIndexes[i]<<":\n";
        vector<string> contents=storage.getBlockContent(blockIndexes[i]);
        for (int j=0;j<contents.size();j++){
            cout << contents[i]<<", ";
        }
        cout <<"\n";
    }

    //calculate average numVotes
    avgNumVotes/=recordPtrs.size();
    cout <<"Average number of votes: "<<avgNumVotes<<endl;

}

int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    BPTree bptree;
    
    importData(storage, bptree, "./data.tsv");

    experiment1(storage);

    experiment3(storage, bptree, 500);

    experiment4(storage, bptree, 30000, 40000);

    return 0;
}