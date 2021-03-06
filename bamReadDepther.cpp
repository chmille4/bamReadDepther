#include <stdint.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <utility>
#include <sstream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

#include "zlib.h"
using namespace std;

static inline bool isBigEndian(void) {
   const uint16_t one = 0x0001;
   return ((*(char*) &one) == 0 );
}

static const bool IS_BIG_ENDIAN = isBigEndian();

static inline void SwapEndian_32(int32_t& x) {
    x = ( (x >> 24) |
         ((x << 8) & 0x00FF0000) |
         ((x >> 8) & 0x0000FF00) |
          (x << 24)
        );
}

static inline void SwapEndian_32(uint32_t& x) {
    x = ( (x >> 24) |
         ((x << 8) & 0x00FF0000) |
         ((x >> 8) & 0x0000FF00) |
          (x << 24)
        );
}

static inline void SwapEndian_64(int64_t& x) {
    x = ( (x >> 56) |
         ((x << 40) & 0x00FF000000000000ll) |
         ((x << 24) & 0x0000FF0000000000ll) |
         ((x << 8)  & 0x000000FF00000000ll) |
         ((x >> 8)  & 0x00000000FF000000ll) |
         ((x >> 24) & 0x0000000000FF0000ll) |
         ((x >> 40) & 0x000000000000FF00ll) |
          (x << 56)
        );
}

static inline void SwapEndian_64(uint64_t& x) {
    x = ( (x >> 56) |
         ((x << 40) & 0x00FF000000000000ll) |
         ((x << 24) & 0x0000FF0000000000ll) |
         ((x << 8)  & 0x000000FF00000000ll) |
         ((x >> 8)  & 0x00000000FF000000ll) |
         ((x >> 24) & 0x0000000000FF0000ll) |
         ((x >> 40) & 0x000000000000FF00ll) |
          (x << 56)
        );
}

static inline std::pair<uint32_t, uint32_t> bin2Region(const uint16_t bin) {
    const uint32_t begin = ( bin - 4681 ) * 16384;
    const uint32_t end   = begin + 16384;
    return make_pair(begin, end);
}


int main(int argc, char **argv) {

    // initialize stream
    istream* myFile = &cin;
    int numOutputBins = 0;
    int c;
    int totalBytesRead = 0;

    // parse command line
    while ((c = getopt (argc, argv, "b:")) != -1)
      switch (c) {
         case 'b':
            numOutputBins = atoi(optarg);
            break;
      }

    // read BAI magic number
    char magic[4];
    myFile->read(magic, 4);
    totalBytesRead += 4;

    if ( !myFile ) {

        cout << "error reading file" << endl;
        return 1;
    }
    if ( strncmp(magic, "BAI\1", 4) != 0 ) {
        cout << "invalid magic number" << endl;
        return 1;
    }

    // read numReferences
    int32_t numReferences;
    myFile->read((char*)&numReferences, sizeof(numReferences));
    if ( IS_BIG_ENDIAN )
        SwapEndian_32(numReferences);
    totalBytesRead += sizeof(numReferences);

    //cout << "numReferences: " << numReferences << endl;
    bool isFirst = true;

    // foreach reference
    for ( int i = 0; i < numReferences; ++ i ) {
        // array holder
        int bins[32768] = {0};
        int maxBin = 0;
        std::ostringstream magicBin;



        int32_t numBins;
        myFile->read((char*)&numBins, sizeof(numBins));
        if ( IS_BIG_ENDIAN )
            SwapEndian_32(numBins);
        totalBytesRead += sizeof(numBins);
        // cout << "totalBytesRead = " << totalBytesRead << endl;
        // cout << "Reference #:" << i << " has " << numBins << " bins" << endl;

        // foreach bin
        for ( int j = 0; j < numBins; ++j ) {

            uint32_t binId;
            myFile->read((char*)&binId, sizeof(binId));
            if ( IS_BIG_ENDIAN )
                SwapEndian_32(binId);
            totalBytesRead += sizeof(binId);

            int32_t numChunks;
            myFile->read((char*)&numChunks, sizeof(numChunks));
            if ( IS_BIG_ENDIAN )
                SwapEndian_32(numChunks);
            totalBytesRead += sizeof(numChunks);

            int byteCount = 0;
            int64_t endBlockAddress;
            int64_t startBlockAddress;
            int64_t startChunk;
            int64_t endChunk;
            int64_t chunkSize;
            // cout << "binid " << binId << " chunks " << numChunks << endl;
            for ( int k = 0; k < numChunks; ++ k ) {
               myFile->read((char*)&startChunk, sizeof(chunkSize));
               totalBytesRead += sizeof(startChunk);
               myFile->read((char*)&endChunk, sizeof(chunkSize));
               totalBytesRead += sizeof(endChunk);
               if ( IS_BIG_ENDIAN ) {
                   SwapEndian_64(startChunk);
                   SwapEndian_64(endChunk);
                }
               endBlockAddress = (endChunk   >> 16) & 0xFFFFFFFFFFFFLL;
               startBlockAddress   = (startChunk >> 16) & 0xFFFFFFFFFFFFLL;

               // cout << "endBlockAddress = " << endBlockAddress << endl;
               byteCount = byteCount + (endBlockAddress - startBlockAddress);
             }
             // only display the 16kb bins

             // if (binId == 37450) {
             //    cout << "#" << i << " binId = " << binId << endl;
             // }

             if ( binId >=  4681 && binId <= 37449) {
                if (maxBin < binId)
                   maxBin = binId;
                bins[binId - 4681] = byteCount;
             }

             // grab magic bin
             if ( binId == 37450) {
                magicBin << startChunk << "\t" << endChunk;
             }

        }

        int32_t numLinearOffsets;
        myFile->read((char*)&numLinearOffsets, sizeof(numLinearOffsets));
        if ( IS_BIG_ENDIAN )
            SwapEndian_32(numLinearOffsets);
        totalBytesRead += sizeof(numLinearOffsets);

        // skip linear offsets
        const int32_t offsetBytes = numLinearOffsets * sizeof(uint64_t);
        char eat[offsetBytes];
        myFile->read(eat,offsetBytes);
        totalBytesRead += offsetBytes;

        //cout << "totalbytes = " << totalBytesRead << endl;
       // output bins
       // substract starting bin to get total bins

       maxBin = maxBin - 4681;
       if (numBins > 0) {
          if (!isFirst)
             cout << endl;
          else
             isFirst = false;
         cout << "#" << i << "\t" << magicBin.str();
          int roundsToAdd;
          if (numOutputBins == 0 || maxBin == 0)
             roundsToAdd = 1;
          else if (maxBin < numOutputBins)
             roundsToAdd = maxBin;
          else
             roundsToAdd = floor(maxBin / numOutputBins);

          int sum = 0;
          for (int m=0; m <= maxBin; m++) {
             sum += bins[m];
             if ( (m % roundsToAdd) == (roundsToAdd-1)) {
               cout << endl << m*16384 << "\t" << sum/roundsToAdd;
                sum = 0;
             }

          }

       }

    }

    int64_t n_no_coor;
    myFile->read((char*)&n_no_coor, sizeof(n_no_coor));
    cout << endl << "*" << "\t" << "\t" << n_no_coor;
    return 0;
}
