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

void importData(Storage storage, BPTree bptree, const char* filename) {
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
        bptree.insert(r.numVotes,recordPtr);
        bptree.display(bptree.getRoot(),0);
    }

    dataFile.close();
} 


int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    BPTree bptree;

    importData(storage, bptree,"./data2.tsv");
    // bptree.display(bptree.getRoot(),0);
    
    return 0;
}