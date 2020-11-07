// tsig.c ... functions on Tuple Signatures (tsig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"
#include "hash.h"


// make a tuple signature

Bits makeTupleSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	char **attributes = tupleVals(r,t);
	Count nbits = tsigBits(r);
	Count k = codeBits(r);
	Bits Tsig = newBits(nbits); // Tuple Signature initially to be zero

	for (int i = 0; i < nAttrs(r); i++){
		Bits curr = codeword(attributes[i],nbits,k);
		orBits(Tsig, curr);
	}

	return Tsig;
}

// find "matching" pages using tuple signatures
// npages examined, ntuples examined and nfalse are calculated in scanAndDisplayMatchingTuples(q), not here

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);
	Reln r = q->rel;
	Bits Qsig = makeTupleSig(q->rel,q->qstring);
    //showBits(Qsig);printf("\n");
	Bits Tsig = newBits(tsigBits(r));
	unsetAllBits(q->pages);
    //printf("nTsigPages is : %d. nTsigs is : %d\n",nTsigPages(r),nTsigs(r));
	for(int i = 0; i < nTsigPages(r); i++){
		Page p = getPage(tsigFile(r),i);
		for(int j = 0; j < maxTsigsPP(r); j++){
            int tid = i * maxTsigsPP(r) + j;
            if(tid < nTsigs(r)){
			    getBits(p,j,Tsig);
			    if (isSubset(Qsig,Tsig)){
				    // get pid corresponding to matching tuple (not Tsig), but tuple id and tsig id are the same
				    int pid =tid / maxTupsPP(r);
				    setBit(q->pages,pid);
			    }
			    q->nsigs++;
            }
		}
		q->nsigpages++;
	}


	// The printf below is primarily for debugging
	// Remove it before submitting this function
	//printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}

Bits codeword(char *attr_value, int nbits, int k){
	int  nset = 0;   // count of set bits
   	Bits cword = newBits(nbits);   // cword is Bits struct of length nbits, initially to be 0
    if(strcmp(attr_value,"?") != 0){
       	srandom(hash_any(attr_value,strlen(attr_value))); // Hash this attribute value randomly
       	while (nset < k) {
          	int i = random() % nbits; // Get a random bit position
          	if (!bitIsSet(cword,i)) { // This bit is not set yet
             	setBit(cword,i); // Set this bit to 1
             	nset++;
          	}
       	}
    }
   	return cword;  // m-bits with k 1-bits and m-k 0-bits

}
