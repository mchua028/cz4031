#include <cstring>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include "storage.h"

/**
 * @brief Construct a new Storage object
 * 
 * @param size Storage size in bytes
 * @param blockSize Block size in bytes
 * @param recordSize Record size in bytes
 */
Storage::Storage(int size, int blockSize, int recordSize) {
    this->size = size;
    this->usedSize = 0;

    this->blockSize = blockSize;
    this->recordSize = recordSize;

    // Allocate memory
    this->storagePtr = new std::byte[size];
    // Point to the first byte of the storage
    this->headPtr = this->storagePtr;

    // Save the available record locations
    for (std::byte *ptr = this->storagePtr; ptr - this->storagePtr + 1 <= this->size; ptr += recordSize) {
        this->availableRecordPtrs.insert(ptr);
    }
}

Storage::~Storage() {
    delete[] this->storagePtr;
}

/**
 * @brief Checks if the starting pointer is the start of (possibly) a record
 * 
 * @param startPtr 
 * @return true if it is a valid starting pointer,
 * @return false otherwise
 */
bool Storage::isValidStartPtr(std::byte* startPtr) {
    int offset = ((startPtr - this->storagePtr) % blockSize);
    return (offset % recordSize) == 0 && // correct offset
           (startPtr - this->storagePtr + 1) < this->size; // within size
}

/**
 * @brief Get block offset of a pointer
 * 
 * @param startPtr
 * @return Block offset
 */
int Storage::getBlockOffset(std::byte* startPtr) {
    return (startPtr - this->storagePtr) % this->blockSize;
}

/**
 * @brief Get block index of a pointer
 * 
 * @param startPtr 
 * @return Block index 
 */
int Storage::getBlockIndex(std::byte* startPtr) {
    return (startPtr - this->storagePtr) / this->blockSize;
}

/**
 * @brief Get the number of used blocks
 * 
 * @return Number of used blocks 
 */
int Storage::getUsedBlocks() {
    return this->usedBlocks;
}

/**
 * @brief Get the space used by records (in bytes)
 * 
 * @return Size used (in bytes)
 */
int Storage::getUsedSize() {
    return this->usedSize;
}

/**
 * @brief Get the content (tconsts) of the block
 * 
 * @param blockIdx 
 * @return Vector of tconst 
 */
std::vector<std::string> Storage::getBlockContent(int blockIdx) {
    if (blockIdx < 0 || blockIdx >= this->size / this->blockSize) {
        throw std::invalid_argument("Block index out of range");
    }

    std::vector<std::string> content;
    std::byte *startBlockPtr = this->storagePtr + blockIdx * this->blockSize;
    char tconst[10];

    for (std::byte *p = startBlockPtr; p < startBlockPtr + this->blockSize; p += this->recordSize) {
        // Skip empty byte
        if ((int) *p == 0x00) {
            continue;
        }
        std::memcpy(tconst, p, 10);
        content.push_back(std::string((char *) tconst));
    }

    return content;
}

/**
 * @brief Get the storage size in bytes
 * 
 * @return Storage size in bytes 
 */
int Storage::getSize() {
    return this->size;
}

/**
 * @brief Get the block size in bytes
 * 
 * @return Block size in bytes
 */
int Storage::getBlockSize() {
    return this->blockSize;
}

/**
 * @brief Get the record size in bytes
 * 
 * @return Record size in bytes 
 */
int Storage::getRecordSize() {
    return this->recordSize;
}

/**
 * @brief Get the record and the block indices that are accessed based on a starting pointer to the record
 * 
 * @param startPtr A pointer to the first byte of the record 
 * @return Tuple of (Record, accessed block index)
 * @throw std::invalid_argument if the starting pointer is invalid
 */
std::tuple<Record, int> Storage::getRecord(std::byte* startPtr) {
    // Wrong starting pointer or not occupied
    if (!this->isValidStartPtr(startPtr) || this->availableRecordPtrs.count(startPtr)) {
        throw std::invalid_argument("Invalid starting pointer");
    }

    std::byte *ptr = startPtr;

    Record r;
    std::memcpy(&r.tconst, ptr, sizeof(r.tconst));
    ptr += sizeof(r.tconst);

    std::memcpy(&r.averageRating, ptr, sizeof(r.averageRating));
    ptr += sizeof(r.averageRating);

    std::memcpy(&r.numVotes, ptr, sizeof(r.numVotes));
    ptr += sizeof(r.numVotes);

    int blockIdx = this->getBlockIndex(startPtr);

    return {r, blockIdx};
}

/**
 * @brief Get the records and the block indices accessed based on starting pointers to records
 * 
 * @param startPtrs Vector of pointers to the first byte of records
 * @return Tuple of (vector of records, vector of accessed block indices)
 */
std::tuple<std::vector<Record>, std::vector<int>> Storage::getRecords(std::vector<std::byte *> startPtrs) {
    std::vector<Record> records;
    std::vector<int> accessedBlockIndices;
    std::unordered_set<int> visited;

    for (auto startPtr: startPtrs) {
        Record r;
        int blockIdx;

        std::tie(r, blockIdx) = this->getRecord(startPtr);

        if (!visited.count(blockIdx)) {
            accessedBlockIndices.push_back(blockIdx);
            visited.insert(blockIdx);
        }

        // Keep track of fetched records and block accesses
        records.push_back(r);
    }

    return {records, accessedBlockIndices};
}

/**
 * @brief Insert a record to the storage
 * 
 * @param r Record 
 * @return A pointer to the starting byte of the record, or NULL if the storage is already full 
 * @throw std::runtime_error if the storage is already full
 */
std::byte* Storage::insertRecord(Record r) {
    if (this->availableRecordPtrs.empty()) {
        throw std::runtime_error("Storage is already full");
    }

    std::byte* startPtr = this->headPtr;

    // Record inserted to a new empty block
    if (this->getBlockOffset(this->headPtr) == 0) {
        this->usedBlocks++;
    }

    // Update markings
    this->availableRecordPtrs.erase(this->headPtr);

    // Copy the record to the storage and move the headPtr along
    std::memcpy(this->headPtr, &r.tconst, sizeof(r.tconst));
    this->headPtr += sizeof(r.tconst);

    std::memcpy(this->headPtr, &r.averageRating, sizeof(r.averageRating));
    this->headPtr += sizeof(r.averageRating);

    std::memcpy(this->headPtr, &r.numVotes, sizeof(r.numVotes));
    this->headPtr += sizeof(r.numVotes);

    // Point to the start of new block to accommodate the next insertion
    if (this->getBlockOffset(this->headPtr) + this->recordSize > blockSize) {
        this->headPtr = this->storagePtr + this->usedBlocks * this->blockSize;
    }
    std::cout << "returned recordAdd: " <<startPtr <<std::endl;

    // Update used size
    this->usedSize += this->recordSize;

    return startPtr;
}

/**
 * @brief Delete a specified record from the storage
 * 
 * @param startPtr Starting byte of the record
 * @return A pointer to the starting byte that is deleted, or NULL if startPtr is invalid or the location is not occupied
 * @throw std::invalid_argument if the starting pointer is invalid
 */
void Storage::deleteRecord(std::byte* startPtr) {
    if (!this->isValidStartPtr(startPtr) || this->availableRecordPtrs.count(startPtr)) {
        throw std::invalid_argument("Invalid starting pointer");
    }

    if (this->headPtr == NULL) {
        this->headPtr = startPtr;
    }

    // Update markings
    this->availableRecordPtrs.insert(startPtr);

    // Clear contents
    std::memset(startPtr, 0x00, this->recordSize);
    // Update used size
    this->usedSize -= this->recordSize;
}
