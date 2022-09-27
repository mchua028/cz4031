#include "storage.cpp"
#include "bptree.cpp"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
using namespace std;

int main() {
    cout <<"<------------------- Database Storage Component ------------------->\n"
           "Database is created by allocating a memory pool, divided into blocks\n"
           "We shall make use of a fixed-size memory pool for storage" << "\n" << "\n";

    cout << "<------------------- Data file read started ------------------->" << "\n" << "\n";

    // vector<tuple<void *, uint>> dataset;
    ifstream dataFile("./data2.tsv");

    Storage memPool{100000000, 200, 20};
    BPTree bptree;

    if(dataFile.is_open()) {
        string line;
        while (getline(dataFile, line)) {
            Record record;
            string tempLine;
            //copy bytes till we encounter tabspace
            strcpy(record.tconst, line.substr(0, line.find('\t')).c_str());


            stringstream linestream(line);
            getline(linestream, tempLine,'\t');
            linestream >> record.averageRating >> record.numVotes;
            byte *recordAdd = memPool.insertRecord(record);
            bptree.insert(record.numVotes,recordAdd);
            

            // void * pointer stores the address of the block, but in order to perform pointer arithmetic
            // we have to cast into uint or uchar pointer.

            // memcpy may seem to be potentially dangerous, but it is safe in this usage as it is the only function that
            // allows copying of byte data and it is being performed inside the container.
            // void *rcdAdr = (uchar *)get<0>(dataRecord) + get<1>(dataRecord);
            // memcpy(rcdAdr, &record, sizeof(record));
//            cout << rcdAdr << " " << record.tconst << '\n';

        }
        cout <<"finsish reading data" <<endl;
        bptree.display(bptree.getRoot());
        // cout << "<------------------- Data file read ended ------------------->" << "\n" << "\n";

        // cout << "<------------------- Database Statistics ------------------->" << "\n";
        // cout << "1. Size of Memory Pool: " << memPool.getMemPoolSize() << "\n";
        // cout << "2. Size of 1 block: " << memPool.getBlkSize() << "\n";
        // cout << "3. Number of blocks available at start: " << memPool.getMemPoolSize() / memPool.getBlkSize() << "\n";
        // cout << "4. Number of allocated blocks: " << memPool.getNumAllocBlks() << "\n";
        // cout << "5. Number of available blocks: " << memPool.getNumAvailBlks() << "\n" << '\n';


        dataFile.close();
    }
    return 0;
}
