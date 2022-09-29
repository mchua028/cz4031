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
        cout << "tconst: " <<r.tconst<<", avgrating: "<<r.averageRating<<", numVotes: "<<r.numVotes<<endl;
        cout << "insert recordAdd: "<< recordPtr <<endl;
        //insert each record into bptree
        bptree.insert(r.numVotes,recordPtr);
        bptree.display(bptree.getRoot(),0);
        std::cout <<"going to next line"<<endl;
    }
    std::cout <<"end of reading data"<<endl;

    vector<byte *> recordPtrs=bptree.searchRecords(20);
    
    for (int i=0;i<recordPtrs.size();i++){
        Record *r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=(Record *)recordPtrs[i];

        cout << r->tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
    }

    recordPtrs=bptree.searchRecords(29);
    
    for (int i=0;i<recordPtrs.size();i++){
        Record *r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=(Record *)recordPtrs[i];

        cout << r->tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
    }

    recordPtrs=bptree.searchRange(10,30);
    for (int i=0;i<recordPtrs.size();i++){
        Record *r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=(Record *)recordPtrs[i];

        cout << r->tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
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
    cout << "fetched records for key="<< key << ": " <<endl;
    vector<byte *> recordPtrs=bptree.searchRecords(key);
    
    for (int i=0;i<recordPtrs.size();i++){
        Record r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=get<0>(storage.getRecord(recordPtrs[i]));

        cout << r.tconst<<endl;
        // cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
    }
}

int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    BPTree bptree;
    
    importData(storage, bptree, "./data2.tsv");

    cout <<"Experiment 1:"<<endl;
    experiment1(storage);

    // cout <<"Experiment 3:"<<endl;
    // experiment3(storage, bptree, 1645);

    return 0;
}