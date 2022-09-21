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

// block structure, not actually used, just for reference
struct block
{ // header 14 Bytes
  int ID;
  int byteLeft;
  int blockEnd;
  bool isFull;
  bool inUsed;
  char data[200 - sizeof(int) * 3 - sizeof(bool) - 3];
};

// struct structure, not actually used, just for reference
struct record
{ // size = 18 Bytes
  char tconst[10];
  float averageRating;
  int numVotes;
};

int read_record(void *memStart, int blkSize)
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
  int i = 1;
  int Count = 0;
  // Read from the text file
  ifstream MyReadFile("data2.tsv");

  // start from first block
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
      startFreeSpace = startFreeSpace + 10;
      *(float *)startFreeSpace = stof(record[1]);
      startFreeSpace = startFreeSpace + 4;
      *(int *)startFreeSpace = stoi(record[2]);
      *BytesLeft = *BytesLeft - recordSize;
      *offSetToFreeSpaceInBlk = *offSetToFreeSpaceInBlk + recordSize;
      startFreeSpace = startFreeSpace + 4;
      Count++;
      cout << "records inserted: " << record[0] << endl;
    }
    // first block full
    // to do: add check when reaching last block
    else
    {
      cout << "inserted " << Count << "records" << endl;
      cout << "Block full, using a new block" << endl;
      *isFull = true;
      blkID = (int *)(memStart + blkSize * i);
      isFull = (bool *)(memStart + sizeof(int) + blkSize * i);
      inUse = (bool *)(memStart + sizeof(bool) + sizeof(int) + blkSize * i);
      offSetToFreeSpaceInBlk = (int *)(memStart + sizeof(bool) * 2 + sizeof(int) + blkSize * i);
      BytesLeft = (int *)(memStart + sizeof(bool) * 2 + sizeof(int) * 2 + blkSize * i);
      startFreeSpace = (memStart + sizeof(bool) * 2 + sizeof(int) * 3 + blkSize * i);
      i++;

      // Inserting record into memory
      if (*BytesLeft >= recordSize)
      {
        *inUse = true;
        for (int i = 0; i < 10; i++)
        {
          *(char *)(startFreeSpace + i) = record[0][i];
        }
        startFreeSpace = startFreeSpace + 10;
        *(float *)startFreeSpace = stof(record[1]);
        startFreeSpace = startFreeSpace + 4;
        *(int *)startFreeSpace = stoi(record[2]);
        *BytesLeft = *BytesLeft - recordSize;
        *offSetToFreeSpaceInBlk = *offSetToFreeSpaceInBlk + recordSize;
        startFreeSpace = startFreeSpace + 4;
        Count++;
        cout << "records inserted: " << record[0] << endl;
      }
    }
  }
  cout << "Last records inserted: " << record[0] << endl;
  cout << "Total records inserted: " << Count << endl;
  // Close the file
  MyReadFile.close();
  return Count;
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

  // reading file
  int inserted = read_record(memStart, blkSize);

  // test printing the records from "disk"
  ptr = ptr + 14;
  for (int i = 1; i < inserted + 1; i++)
  {
    // printing the tconst
    for (int j = 0; j < 10; j++)
    {
      cout << (*(char *)(ptr + j));
    }
    cout << endl
         << *(float *)(ptr + 10) << endl // printing rating
         << *(int *)(ptr + 14) << endl   // printing votes
         << endl;
    if (blkSize == 200) // jumping to next block when blkSize == 200
    {
      if (i % (10) == 0)
      {
        ptr = ptr + 14 + 18 + 6;
      }
      else
      {
        ptr = ptr + 18;
      }
    }
    else // jumping to next block when blkSize == 500
    {
      if (i % 27 == 0)
      {
        ptr = ptr + 14 + 18;
      }
      else
      {
        ptr = ptr + 18;
      }
    }
  }

  free(memStart);
  return 0;
}
