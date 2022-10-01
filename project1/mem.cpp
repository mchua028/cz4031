#include <iostream>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
using namespace std;

// Global variables
int memSize = 100000000;
int blkSize = 500;

// block structure declaration
struct block
{ // header 14 Bytes
  int ID;
  int byteLeft;
  int blockEnd;
  bool isFull;
  bool inUsed;

  // manually update blkSize here
  char data[200 - sizeof(int) * 3 - sizeof(bool) - 3];
};

struct record
{ // size = 18 Bytes
  char tconst[10];
  float averageRating;
  int numVotes;
};

bool read_record(void *memStart, int blkSize)
{

  // Create a text string, which is used to output the text file
  string line;
  vector<string> record;
  string field, temp;
  char tconst[10];
  float averageRating;
  int numVotes;
  int recordSize = sizeof(tconst) + sizeof(averageRating) + sizeof(numVotes);
  int *blkID;
  bool *isFull;
  bool *inUse;
  int *offSetToFreeSpaceInBlk;
  int *BytesLeft;
  void *startFreeSpace;
  // Read from the text file
  ifstream MyReadFile("data2.tsv");

  blkID = (int *)(memStart);
  isFull = (bool *)(memStart + sizeof(int));
  inUse = (bool *)(memStart + sizeof(bool) + sizeof(int));
  offSetToFreeSpaceInBlk = (int *)(memStart + sizeof(bool) * 2 + sizeof(int));
  BytesLeft = (int *)(memStart + sizeof(bool) * 2 + sizeof(int) * 2);
  startFreeSpace = (memStart + sizeof(bool) * 2 + sizeof(int) * 3);
  // Use a while loop together with the getline() function to read the file line by line
  // skip first line using 1 read
  getline(MyReadFile, line);
  while (getline(MyReadFile, line))
  {

    record.clear();
    stringstream s(line);
    while (getline(s, field, '\t'))
    {

      // add all the column data
      // of a row to a vector
      record.push_back(field);
    }

    // Inserting record into memory
    if (*BytesLeft >= recordSize)
    {
      *inUse = true;
      for (int i = 0; i < 10; i++)
      {
        *(char *)(startFreeSpace + i) = record[0][i];
      }
      // memcpy(startFreeSpace, &(record[0]), 10);
      startFreeSpace = startFreeSpace + 10;
      *(float *)startFreeSpace = stof(record[1]);
      startFreeSpace = startFreeSpace + 4;
      *(int *)startFreeSpace = stoi(record[2]);
      *BytesLeft = *BytesLeft - recordSize;
      *offSetToFreeSpaceInBlk = *offSetToFreeSpaceInBlk + recordSize;
      // break;

      startFreeSpace = startFreeSpace + 4;
    }
    else
    {
      return true;
    }
  }

  // Close the file
  MyReadFile.close();
  return true;
}

int main()
{

  cout << "................Initializing Disk................" << endl
       << endl;

  // allocating memory for the "Disk"
  cout << "..........Assigning memory for the Disk.........." << endl
       << endl;
  void *memStart = malloc(memSize);

  // assigning block header for every block
  cout << ".............Initializing the Blocks............." << endl
       << endl;
  cout << "...............Block size:" << blkSize << " Bytes.............." << endl
       << endl;
  void *ptr = memStart;

  // assigning header to the blocks
  for (int i = 0; i < memSize / blkSize; i++)
  {
    // Block ID
    *(int *)ptr = i;
    // isFull flag
    *(bool *)(ptr + sizeof(int)) = false;
    // inUse flag
    *(bool *)(ptr + sizeof(int) + sizeof(bool)) = false;
    // offset to start of free space in block
    *(int *)(ptr + sizeof(int) + sizeof(bool) * 2) = sizeof(int) * 3 + sizeof(bool) * 2;
    // Byte left in block
    *(int *)(ptr + sizeof(int) * 2 + sizeof(bool) * 2) = blkSize - sizeof(int) * 3 - sizeof(bool) * 2;
    // address of start of data
    for (int j = 0; j < blkSize - sizeof(int) * 3 - sizeof(bool) * 2; j++)
    {
      *(char *)(ptr + sizeof(int) * 3 + sizeof(bool) * 2 + j) = '\0';
    }

    ptr = ptr + blkSize;
  }

  cout << "............Disk initialization done!............"
       << endl
       << endl
       << "............" << memSize / blkSize << " Blocks initialized............" << endl;
  ptr = memStart;

  read_record(memStart, blkSize);

  cout << "blk id: " << *(int *)ptr << endl;
  cout << "isFull: " << *(bool *)(ptr + 4) << endl;
  cout << "inUse: " << *(bool *)(ptr + 5) << endl;
  cout << "offset to start of free space: " << *(int *)(ptr + 6) << endl;
  cout << "Bytes left in block: " << *(int *)(ptr + 10) << endl;
  ptr = ptr + 14;
  for (int i = 0; i < 30; i++)
  {

    for (int i = 0; i < 10; i++)
    {
      cout << (*(char *)(ptr + i));
    }
    cout << endl
         << *(float *)(ptr + 10) << endl
         << *(int *)(ptr + 14) << endl
         << endl;
    ptr = ptr + 18;
  }

  free(memStart);
  return 0;
}