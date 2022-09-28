#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "storage.h"

const int SIZE = 1e8;
const int BLOCK_SIZE = 200;
const int RECORD_SIZE = 18;

void importData(Storage &storage, const char* filename) {
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
    }

    dataFile.close();
} 

void experiment1(Storage &storage) {
    std::cout << "---Experiment 1---\n";

    std::cout << "Number of blocks: " << storage.getUsedBlocks() << '\n';
    std::cout << "Size of database: " << storage.getUsedSize() / 1000000.0 << " MB\n";
}

int main() {
    Storage storage(SIZE, BLOCK_SIZE, RECORD_SIZE);
    importData(storage, "data.tsv");

    experiment1(storage);

    return 0;
}