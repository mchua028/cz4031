#pragma once
#include <cstddef>
#include <unordered_set>
#include <vector>
#include <tuple>
#include <string>

struct Record {
    char tconst[10];
    float averageRating;
    int numVotes;
};

class Storage {
    private:
        // Storage size (bytes)
        int size;
        // Used storage size (bytes)
        int usedSize;

        // Block size (bytes)
        int blockSize;
        // Record size (bytes)
        int recordSize;

        // Number of blocks that contain at least 1 record
        int usedBlocks;

        // Store the starting pointers of available spaces for records 
        std::unordered_set<std::byte*> availableRecordPtrs;

        // Pointer to the first byte of the storage
        std::byte *storagePtr;
        // Pointer to the starting byte to insert a record
        std::byte *headPtr;
        
        bool isValidStartPtr(std::byte* startPtr);
        int getBlockOffset(std::byte* startPtr);
        int getBlockIndex(std::byte* startPtr);
    public:
        Storage(int size, int blockSize, int recordSize);
        ~Storage();
        int getSize();
        int getBlockSize();
        int getRecordSize();
        int getUsedBlocks();
        int getUsedSize();
        std::vector<std::string> getBlockContent(std::byte* startPtr);
        std::tuple<Record, int> getRecord(std::byte* startPtr);
        std::tuple<std::vector<Record>, std::vector<int>> getRecords(std::vector<std::byte *> startPtrs); 
        std::byte* insertRecord(Record r);
        void deleteRecord(std::byte* startPtr);
};
