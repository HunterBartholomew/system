
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include "cachelab.h"

typedef struct{
    unsigned valid;
    unsigned long tag;
    unsigned LRU;
}line;

typedef struct{
    unsigned long t;
    unsigned long s;
    unsigned long b;
}address;

address parseAddress(long, unsigned, unsigned);
unsigned goToCache(line **, address,unsigned,unsigned);
int parseCMD(int,char **, unsigned*,unsigned*,unsigned*,char*);

int main(int argc, char ** argv)
{
    unsigned s = 0;
    unsigned E = 0;
    unsigned b = 0;
    char path[20] = "";

    parseCMD(argc, argv, &s, &E, &b,path);

    unsigned S = 1<<s;
    unsigned hit_count = 0;
    unsigned miss_count = 0;
    unsigned eviction_count = 0;

    line **cache;
    cache = (line**) malloc(S*sizeof(line *));
    for (int i=0;i<S;i++){
        cache[i] = (line*)malloc(E*sizeof(line));
    }

    for (int i=0; i<S; i++) {
        for (int j=0; j<E; j++) {
            cache[i][j].LRU = 0;
            cache[i][j].tag = 0;
            cache[i][j].valid = 0;
        }
    }

    char identifier;
    unsigned long addr;
    int size;
    FILE *pFile;
    pFile = fopen(path, "r");

    while (fscanf(pFile, " %c %lx,%d",&identifier, &addr, &size)!=EOF) {
        if (identifier == 'I') {
            continue;
        }
        if (identifier == 'M') {
            hit_count += 1;
        }
        address suchAddr = parseAddress(addr,s,b);
        unsigned hitOrMiss = goToCache(cache, suchAddr, S, E);

        switch (hitOrMiss) {
            case 1:
                hit_count += 1;
                break;
            case 2:
                miss_count += 1;
                break;
            case 3:
                eviction_count += 1;
                miss_count +=1;
                break;
            default:
                printf("something wents wrong");
                break;
        }
    }
	
    fclose(pFile);
    for (int i=0; i<S; i++) {
        free(cache[i]);
    }
	
    free(cache);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;

}

int parseCMD(int argc, char** argv, unsigned* s, unsigned* E, unsigned* b, char* path){
    opterr = 0;
    int opt=0;
    while((opt = getopt(argc, argv, "s:E:b:t:")) != -1){
        switch (opt) {
            case 's':
                *s = atoi(optarg);
                break;
            case 'E':
                *E = atoi(optarg);
                break;
            case 'b':
                *b = atoi(optarg);
                break;
            case 't':
                strcpy(path, optarg);
                break;
            default:
                printf("wrong cmd");
                break;
        }
    }
    return 0;
}

address parseAddress(long addr, unsigned s, unsigned b){
    address result = {0,0,0};
    result.b = addr& (0x7fffffffffffffff>>(63-b));
    addr = addr>>b;
    result.s = addr& (0x7fffffffffffffff>>(63-s));
    addr = addr>>s;
    result.t = addr;
    return result;
}

unsigned goToCache(line **cache, address suchAddress, unsigned S,unsigned E){
    unsigned returnCode = 0;
    unsigned long setIndex = suchAddress.s;
    int hit = 0;
    int emptyLine = -1;
    int maxLRU = 0;
    int minLRU = -1;
    line *currentCache = cache[setIndex];
    for (int j=0; j<E; j++) {
        line currentLineInCache = currentCache[j];
        if (currentLineInCache.valid == 0) {
            emptyLine = j;
            continue;
        }

        //request hits
        if (currentLineInCache.valid == 1 && currentLineInCache.tag == suchAddress.t) {
            hit = 1;
            returnCode = 1;
            cache[setIndex][j].LRU = 0;
            break;
        }

        if (currentLineInCache.valid == 1 && currentLineInCache.tag != suchAddress.t) {
            cache[setIndex][j].LRU += 1;
            
        }
    }
    
    //request hit, added
    if (emptyLine != -1 && hit == 0) {
        line addLine = {1,suchAddress.t,0};
        cache[setIndex][emptyLine] = addLine;
        returnCode = 2;
    }
    
    //request did not hit, evicted
    if (emptyLine == -1 && hit == 0) {
        for (int k=0; k<E; k++) {
            if (cache[setIndex][k].LRU>=maxLRU &&cache[setIndex][k].valid != 0) {
                maxLRU= cache[setIndex][k].LRU;
                minLRU = k;
            }
        }
        line replaceLine = {1,suchAddress.t,0};
        cache[setIndex][minLRU] = replaceLine;
        returnCode = 3;
    }
    
    return returnCode;
}
