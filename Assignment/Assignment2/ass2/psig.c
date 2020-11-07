// psig.c ... functions on page signatures (psig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include <string.h>
#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "bits.h"
#include "hash.h"

// make a page signature

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
    char **attributes = tupleVals(r, t);
    Count m = psigBits(r);
    Count k = codeBits(r);
    Bits Psig = newBits(m); //Page Signature initially to be zero

    for (int i = 0; i < nAttrs(r); i++) {
        Bits curr = pagecodeword(attributes[i], m, k);
        orBits(Psig, curr);
    }

	return Psig;
}


void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	Reln r = q->rel;
	Bits Qsig = makePageSig(r, q->qstring);
    Bits Psig = newBits(psigBits(r));
    unsetAllBits(q->pages);

    for (int i = 0; i < nPsigPages(r); i++) {
        Page p = getPage(psigFile(r), i);
        for (int j = 0; j < pageNitems(p); j++) {
            getBits(p, j, Psig);
            if (isSubset(Qsig, Psig)) {
                int pid = i * maxPsigsPP(r) + j;
                setBit(q->pages, pid);
            }
            q->nsigs++;
        }
        q->nsigpages++;
    }
}

Bits pagecodeword(char *attr_value, int m, int k){
    int  nbits = 0;   // count of set bits
    Bits cword = newBits(m);   // cword is Bits struct of length m, initially to be 0
    if(strcmp(attr_value,"?") != 0){
        srandom(hash_any(attr_value,strlen(attr_value))); // Hash this attribute value randomly
        while (nbits < k) {
            int i = random() % m; // Get a random bit position
            if (!bitIsSet(cword,i)) { // This bit is not set yet
                setBit(cword,i); // Set this bit to 1
                nbits++;
            }
        }
    }
    return cword;  // m-bits with k 1-bits and m-k 0-bits

}

