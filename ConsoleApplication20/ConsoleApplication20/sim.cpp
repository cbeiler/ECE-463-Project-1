// ConsoleApplication20.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <string>
#include <cmath>
#include <vector>
using namespace std;

Cache::Cache(long long cacheSize, long long blockSize, uint32_t setAssoc, int level) {

	this->cacheSize = cacheSize;
	this->blockSize = blockSize;
	this->setAssoc = setAssoc;
	this->level = level;

	numSets = cacheSize / (blockSize*setAssoc);
	blockOffsetSize = log2(blockSize);
	indexSize = log2(numSets);

	

	this->blocks = new Block**[numSets];
	for (int i = 0; i < numSets; i++) {
		blocks[i] = new Block*[setAssoc];
		for (int j = 0; j < setAssoc; j++)
			blocks[i][j] = new Block(setAssoc);
	}


}
Block::Block(uint32_t setAssoc) {
	this -> valid = 0;
	this-> dirty = 0;
	this->lru = 0;
	this->data = 0;
	this->index = 0;
	this->blockOffset = 0;
	this->tag = 0;
	this->address = 0;
}

void Cache::print() {
	for (int i = 0; i < numSets; i++) {
		for (int j = 0; j < setAssoc; j++) {
			printf("%x %d %x ", blocks[i][j]->tag, blocks[i][j]->lru, blocks[i][j]->dirty);//debug porpoises
		}
		printf("\n");
	}
}

//should be done
uint32_t Cache::updateLRU(uint32_t tag, uint32_t index,bool hitUpdate) {
	int oldLRU = 0;
	int updatedIndex = -1;

	for (int j = 0; j < setAssoc; j++) {
		if (!(blocks[index][j]->valid) && (!hitUpdate)) {
			updatedIndex = j;
			break;
		}
		
		if (blocks[index][j]->tag == tag) {
			oldLRU = blocks[index][j]->lru;
			blocks[index][j]->lru = 0;
			updatedIndex = j;
			//
			break;
		}
	}
	if (updatedIndex == -1) {
		//means we missed and there are no open spots
		for (int l = 0; l < setAssoc; l++) {
			if (blocks[index][l]->lru == (setAssoc - 1)) {
				updatedIndex = l;
				oldLRU = blocks[index][l]->lru;
				blocks[index][l]->lru = 0;
				break;
			}
		}

	}
	if (blocks[index][updatedIndex]->valid) {
		for (int k = 0; k < setAssoc; k++) {
			if (blocks[index][k]->lru < oldLRU && (blocks[index][k]->valid) && (k!=updatedIndex)) {
				blocks[index][k]->lru++;
			}
			//printf("%x %d  ", blocks[index][k]->tag, blocks[index][k]->lru);//debug porpoises

		}
	}
	else {//if the evicted block was not valid
		for (int k = 0; k < setAssoc; k++) {
			if ((blocks[index][k]->valid)) {
				blocks[index][k]->lru++;
			}
		}
	}
		//printf("\n");
	return updatedIndex;//returns index to set to dirty if needed
}


//not done
void Cache::readRequest(uint32_t address) {
	bool hit = 0;
	uint32_t setNum = 0;
	reads++;
	uint32_t blockOffset = address & (0xFFFFFFFF >> (32 - blockOffsetSize));
	uint32_t tag = (address >> (blockOffsetSize + indexSize)) & (0xFFFFFFFF);
	uint32_t index = (address >> blockOffsetSize) & (0xFFFFFFFF >> 32 - indexSize);
	for (int j = 0; j < setAssoc; j++) {
		if (blocks[index][j]->tag == tag) {
			readHits = 1;
			break;
		}
	}
	if (hit) {
		hits++;
		readHits++;
		setNum = updateLRU(tag, index, hit);
		blocks[index][setNum]->dirty = 0xD;
	}
	else {
		//miss
		//if evicted block is dirty, write request L2, and read request L2, if clean, just read request L2
		misses++;
		setNum = updateLRU(tag, index, hit);

		if (level == 1) {
			if (this->blocks[index][setNum]->dirty == 0xD) {
				this->next->writeRequest(blocks[index][setNum]->address);
				writebacks++;
			}
			this->next->readRequest(address);
			readMisses++;
		}
		else if(level ==2){
			//L2 read miss, memory read request
			if (this->blocks[index][setNum]->dirty == 0xD)
				writebacks++;
			readMisses++;
			memReads++;
			;
		}
		//INSTALL BLOCK clean
		
		blocks[index][setNum]->address = address;
		blocks[index][setNum]->valid = 1;
		blocks[index][setNum]->tag = tag;
		blocks[index][setNum]->dirty = 0xC;
	}

	//printf("set %x: ", index);
	//for (int j = 0; j < setAssoc; j++) {
	//	printf(" %x %d %x ", blocks[index][j]->tag, blocks[index][j]->lru, blocks[index][j]->dirty);//debug porpoises
	//}
	//printf("\n");
}

//not done
void Cache::writeRequest(uint32_t address) {
	writes++;
	bool hit = 0;
	uint32_t setNum = 0;
	uint32_t blockOffset = address & (0xFFFFFFFF >> (32-blockOffsetSize));
	uint32_t tag = (address >> (blockOffsetSize+indexSize)) & (0xFFFFFFFF);
	uint32_t index = (address >> blockOffsetSize) & (0xFFFFFFFF >> 32-indexSize);

	for (int j = 0; j < setAssoc; j++) {
		if (blocks[index][j]->tag == tag) {
			hit = 1;
			break;
		}
	}
	if (hit) {
		hits++;
		writeHits++;
		setNum = updateLRU(tag, index, hit);
		blocks[index][setNum]->dirty = 0xD;
	}
	else {
		misses++;
		writeMisses++;
		//issue read request to next level in memory
		if (level == 1)
			this->next->readRequest(address);//WORKING ON THIS RIGHT NOW
		else if (level == 2)
			memReads++;
			//increase memory read
		//install block in LRU and update dirty and lru
		//INSTALL BLOCK
		setNum = updateLRU(tag, index, hit);
		blocks[index][setNum]->address = address;
		blocks[index][setNum]->valid = 1;
		blocks[index][setNum]->tag = tag;
		blocks[index][setNum]->dirty = 0xD;

		
	}
	//printf("set %x: ", index);
	//for (int j = 0; j < setAssoc; j++) {
	//	printf(" %x %d %x ", blocks[index][j]->tag, blocks[index][j]->lru, blocks[index][j]->dirty);//debug porpoises
	//}
	//printf("\n");
}

void Cache::incHits() {
	hits++;
}
void Cache::incMisses() {
	misses++;
}

Cache* instantiateCache(long long cacheSize, long long blockSize, long long setAssoc, int level) {
	Cache* cache = new Cache(cacheSize, blockSize, setAssoc, level);
	return cache;
}

/*  "argc" holds the number of command-line arguments.
	"argv[]" holds the arguments themselves.

	Example:
	./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
	argc = 9
	argv[0] = "./sim"
	argv[1] = "32"
	argv[2] = "8192"
	... and so on
*/
int main(int argc, char *argv[]) {
	FILE *fp;			// File pointer.
	char *trace_file;		// This variable holds the trace file name.
	cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
	char rw;			// This variable holds the request's type (read or write) obtained from the trace.
	uint32_t addr;		// This variable holds the request's address obtained from the trace.
				 // The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.
	//DEBUG SETUP

	char blocksize[3] = "16";//32
	char l1size[6] = "1024";//8192
	char l1assoc[5] = "1";
	char l2size[10] = "8192";// "262144";
	char l2assoc[5] = "4";
	char prefn[5] = "0";
	char prefm[5] = "0";
	char trcfile[25] = "../gcc_trace.txt";
	argc = 9;
	argv[1] = blocksize;
	argv[2] = l1size;
	argv[3] = l1assoc;
	argv[4] = l2size;
	argv[5] = l2assoc;
	argv[6] = prefn;
	argv[7] = prefm;
	argv[8] = trcfile;


	// Exit with an error if the number of command-line arguments is incorrect.
	if (argc != 9) {
		printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
		exit(EXIT_FAILURE);
	}

	// "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
	params.BLOCKSIZE = (uint32_t)atoi(argv[1]);
	params.L1_SIZE = (uint32_t)atoi(argv[2]);
	params.L1_ASSOC = (uint32_t)atoi(argv[3]);
	params.L2_SIZE = (uint32_t)atoi(argv[4]);
	params.L2_ASSOC = (uint32_t)atoi(argv[5]);
	params.PREF_N = (uint32_t)atoi(argv[6]);
	params.PREF_M = (uint32_t)atoi(argv[7]);
	trace_file = argv[8];



	//Instantiate L1 and L2 Cache Objects based on params
	//here is a vector with both of the cache levels
	vector<Cache*> cache(2);
	cache[0] = instantiateCache(params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC, 1);
	cache[1] = instantiateCache(params.L2_SIZE, params.BLOCKSIZE, params.L2_ASSOC, 2);
	cache[0]->next = cache[1];


	// Open the trace file for reading.
	fp = fopen(trace_file, "r");
	if (fp == (FILE *)NULL) {
		// Exit with an error if file open failed.
		printf("Error: Unable to open file %s\n", trace_file);
		exit(EXIT_FAILURE);
	}

	// Print simulator configuration.
	printf("===== Simulator configuration =====\n");
	printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
	printf("L1_SIZE:    %u\n", params.L1_SIZE);
	printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
	printf("L2_SIZE:    %u\n", params.L2_SIZE);
	printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
	printf("PREF_N:     %u\n", params.PREF_N);
	printf("PREF_M:     %u\n", params.PREF_M);
	printf("trace_file: %s\n", trace_file);
	printf("===================================\n");

	// Read requests from the trace file and echo them back.
	while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
		if (rw == 'r') {
			//printf(" r %x ===================================\n", addr);
 			cache[0]->readRequest(addr);//printf("r %x\n", addr);
			//cache[0]->print();
		}
		else if (rw == 'w') {
			//printf(" w %x ===================================\n", addr);
			cache[0]->writeRequest(addr);//printf("r %x\n", addr);
			//cache[0]->print();
			//printf("===== Simulator configuration =====\n");
			//printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
			//printf("L1_SIZE:    %u\n", params.L1_SIZE);
			//printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
			//printf("L2_SIZE:    %u\n", params.L2_SIZE);
			//printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
			//printf("PREF_N:     %u\n", params.PREF_N);
			//printf("PREF_M:     %u\n", params.PREF_M);
			//printf("trace_file: %s\n", trace_file);
			
			
		}
		else if (rw == 'x') {
			


		}
		else {
			printf("Error: Unknown request type %c.\n", rw);
			exit(EXIT_FAILURE);
		}

		///////////////////////////////////////////////////////
		// Issue the request to the L1 cache instance here.
		///////////////////////////////////////////////////////
	}

	for (int i = 0; i < cache[0]->numSets; i++) {
		printf("set\t%d:\t",i);
		for (int j = 0; j < cache[0]->setAssoc; j++) {
			if (cache[0]->blocks[i][j]->dirty != 0xC)
				printf("%u %x\t", cache[0]->blocks[i][j]->tag, cache[0]->blocks[i][j]->dirty);
			else
				printf("%x\t\t", cache[0]->blocks[i][j]->tag);
		}
		printf("\n");

	}
	for (int i = 0; i < cache[1]->numSets; i++) {
		printf("set\t%d:\t",i);
		for (int j = 0; j < cache[1]->setAssoc; j++) {
			if(cache[1]->blocks[i][j]->dirty != 0xC)
				printf("%u %x\t", cache[1]->blocks[i][j]->tag, cache[1]->blocks[i][j]->dirty);
			else 
				printf("%x\t\t", cache[1]->blocks[i][j]->tag);
		}
		printf("\n");

	}
	printf("===== Measurements =====\n");

	/*a*/printf("a. L1 reads: \t\t\t%u\n", cache[0]->reads);
	/*b*/printf("b. L1 read misses:\t\t%u\n", cache[0]->readMisses);
	/*c*/printf("c. L1 writes:\t\t\t%u\n", cache[0]->writes);
	/*d*/printf("d. L1 write misses:\t\t%u\n", cache[0]->writeMisses);
	/*e*/printf("e. L1 miss rate:\t\t%f\n", (cache[0]->readMisses + cache[0]->writeMisses)/(cache[0]->reads + cache[0]->writes));
	/*f*/printf("f. L1 writebacks:\t\t%u\n", cache[0]->writebacks);
	/*g*/printf("g. L1 prefetches:\t\t%u\n", 0);
	/*h*/printf("h. L2 reads (demand):\t\t%u\n", cache[1]->reads);
	/*i*/printf("i. L2 read misses (demand):\t%u\n", cache[1]->readMisses);
	/*j*/printf("j. L2 reads (prefetch):\t\t%u\n", 0);
	/*k*/printf("k. L2 read misses (prefetch):\t%u\n", 0);
	/*l*/printf("l. L2 writes:\t\t\t%u\n", cache[1]->writes);
	/*m*/printf("m. L2 write misses:\t\t%u\n", cache[1]->writeMisses);
	/*n*/printf("n. L2 miss rate:\t\t%f\n", (cache[1]->readMisses) / (cache[1]->reads));
	/*o*/printf("o. L2 writebacks:\t\t%u\n", cache[1]->writebacks);
	/*p*/printf("p. L2 prefetches:\t\t%u\n", 0);
	/*q*/printf("q. memory traffic:\t\t%u\n", cache[0]->memReads + cache[0]->memWrites + cache[1]->memReads + cache[1]->memWrites);
	return(0);
}

