#pragma once
#include <cstddef>
#include <unordered_set>
#include <vector>
#include <tuple>

struct Record {
    char tconst[10];
    float averageRating;
    int numVotes;
};

class Storage {
    private:
        // Storage size (bytes)
        int size;
        // Block size (bytes)
        int blockSize;
        // Record size (bytes)
        int recordSize;

        // Store the starting pointers of available spaces for records 
        std::unordered_set<std::byte*> available;

        // Pointer to the first byte of the storage
        std::byte *storagePtr;
        // Pointer to the starting byte to insert a record
        std::byte *headPtr;
        
        bool isValidStartPtr(std::byte* startPtr);
    public:
        Storage(int size, int blockSize, int recordSize);
        ~Storage();
        int getSize();
        int getBlockSize();
        int getRecordSize();
        std::tuple<Record, std::unordered_set<int>> getRecord(std::byte* startPtr);
        std::tuple<std::vector<Record>, std::unordered_set<int>> getRecords(std::vector<std::byte *> startPtrs); 
        std::byte* insertRecord(Record r);
        void deleteRecord(std::byte* startPtr);
};
