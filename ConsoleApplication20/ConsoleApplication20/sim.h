#ifndef SIM_CACHE_H
#define SIM_CACHE_H
#include <vector>

typedef 
struct {
   uint32_t BLOCKSIZE = 32;
   uint32_t L1_SIZE = 8192;
   uint32_t L1_ASSOC = 4;
   uint32_t L2_SIZE = 262144;
   uint32_t L2_ASSOC = 8;
   uint32_t PREF_N = 3;
   uint32_t PREF_M = 10;
} cache_params_t;

// Put additional data structures here as per your requirement.

//Cache Class! to be instantiated in main as L1 and L2 Cache
class Cache;
class Block;

class Block {
public:
	uint32_t valid, dirty, lru;
	uint32_t setAssoc;
	uint32_t data;
	uint32_t index;
	uint32_t blockOffset;
	uint32_t tag;
	uint32_t address;
	Block(uint32_t setAssoc);
};

class Cache {
public:
	long long hits, misses, cacheSize, blockSize, numSets, reads, writes;
	uint32_t setAssoc;
	Block *** blocks;
	Cache* next;
	long long readMisses, readHits, writeMisses, writeHits, memReads, memWrites, writebacks;

	int level;
	int blockOffsetSize;
	int indexSize;

	void readRequest(uint32_t address);
	void writeRequest(uint32_t address);
	void incHits();
	void incMisses();
	void print();
	uint32_t updateLRU(uint32_t tag, uint32_t index,bool hitUpdate);

	Cache(long long cacheSize, long long blockSize, uint32_t setAssoc, int level);

};

#endif
