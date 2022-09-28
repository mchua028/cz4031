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

BPTree importData(Storage storage, BPTree bptree, const char* filename) {
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
        bptree.insert(r.numVotes,recordPtr);
        bptree.display(bptree.getRoot(),0);
        
    }
    cout << "fetched record: "<<endl;
    vector<byte *> recordPtrs=bptree.searchRecords(1807);
    
    for (int i=0;i<recordPtrs.size();i++){
        Record *r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=(Record *)recordPtrs[i];

        cout << r->tconst<<endl;
        cout <<get<0>(storage.getRecord(recordPtrs[i])).tconst<< endl;
    }
    dataFile.close();
    return bptree;
} 


int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    BPTree bptree;

    bptree=importData(storage, bptree,"./data2.tsv");
    // bptree.display(bptree.getRoot(),0);
    vector<byte *> recordPtrs=bptree.searchRecords(154);
    
    for (int i=0;i<recordPtrs.size();i++){
        Record r;
        cout << "fetch recordAdd:" << recordPtrs[i] <<endl;
        r=get<0>(storage.getRecord(recordPtrs[i]));

        cout << r.tconst<<endl;
    }

    
    return 0;
}