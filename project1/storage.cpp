#include <cstring>
#include <stdexcept>
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
    this->blockSize = blockSize;
    this->recordSize = recordSize;

    // Allocate memory
    this->storagePtr = new std::byte[size];
    // Point to the first byte of the storage
    this->headPtr = this->storagePtr;

    // Save the available record locations
    for (std::byte *ptr = this->storagePtr; ptr - this->storagePtr + 1 <= this->size; ptr += recordSize) {
        this->available.insert(ptr);
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
    // Multiple of recordSize and not out of bounds
    return (startPtr - this->storagePtr) % this->recordSize == 0 && startPtr - this->storagePtr + 1 < this->size;
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
 * @return Tuple of (Record, unordered set of block indices that are accessed)
 * @throw std::invalid_argument if the starting pointer is invalid
 */
std::tuple<Record, std::unordered_set<int>> Storage::getRecord(std::byte* startPtr) {
    // Wrong starting pointer or not occupied
    if (!this->isValidStartPtr(startPtr) || !occupied.count(startPtr)) {
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

    int startBlockIdx = ( startPtr - this->storagePtr ) / this->blockSize;
    int endBlockIdx = (ptr - this->storagePtr) / this->blockSize;

    std::unordered_set<int> blockIndices;
    for (int i = startBlockIdx; i <= endBlockIdx; i++) {
        blockIndices.insert(i);
    }

    return {r, blockIndices};
}

/**
 * @brief Get the records and the block indices accessed based on starting pointers to records
 * 
 * @param startPtrs Vector of pointers to the first byte of records
 * @return Tuple of (vector of records, unordered set of block indices that are accessed)
 */
std::tuple<std::vector<Record>, std::unordered_set<int>> Storage::getRecords(std::vector<std::byte *> startPtrs) {
    std::vector<Record> records;
    std::unordered_set<int> allBlockIndices;

    for (auto startPtr: startPtrs) {
        Record r;
        std::unordered_set<int> rBlockIndices;

        std::tie(r, rBlockIndices) = this->getRecord(startPtr);

        // Keep track of fetched records and block accesses
        records.push_back(r);
        allBlockIndices.merge(rBlockIndices);
    }

    return {records, allBlockIndices};
}

/**
 * @brief Insert a record to the storage
 * 
 * @param r Record 
 * @return A pointer to the starting byte of the record, or NULL if the storage is already full 
 * @throw std::runtime_error if the storage is already full
 */
std::byte* Storage::insertRecord(Record r) {
    if (this->available.empty()) {
        throw std::runtime_error("Storage is already full");
    }

    std::byte* startPtr = this->headPtr;

    // Update markings
    this->available.erase(this->headPtr);
    this->occupied.insert(this->headPtr);

    // Copy the record to the storage and move the headPtr along
    std::memcpy(this->headPtr, &r.tconst, sizeof(r.tconst));
    this->headPtr += sizeof(r.tconst);

    std::memcpy(this->headPtr, &r.averageRating, sizeof(r.averageRating));
    this->headPtr += sizeof(r.averageRating);

    std::memcpy(this->headPtr, &r.numVotes, sizeof(r.numVotes));
    this->headPtr += sizeof(r.numVotes);

    // Update headPtr if pointing to an unavailable location
    if (!this->available.count(this->headPtr)) {
        // No available locations left, i.e. full
        if (this->available.empty()) {
            this->headPtr = NULL;
        } else {
            // Point to an available location
            this->headPtr = *(this->available.begin());
        }
    }

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
    if (!this->isValidStartPtr(startPtr) || this->available.count(startPtr)) {
        throw std::invalid_argument("Invalid starting pointer");
    }

    if (this->headPtr == NULL) {
        this->headPtr = startPtr;
    }

    // Update markings
    available.insert(startPtr);
    occupied.erase(startPtr);
}
