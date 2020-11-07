// bsig.c ... functions on Tuple Signatures (bsig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"

void findPagesUsingBitSlices(Query q)
{
    assert(q != NULL);
    Reln r = q->rel;
    Bits Qsig = makePageSig(r, q->qstring);
    Bits matches = newBits(bsigBits(r));
    setAllBits(matches);
    PageID pid; PageID off; PageID id;
    int nb = 0;
    for (int i = 0; i < psigBits(r); i++) {
        if (bitIsSet(Qsig, i)) {
            pid = i / maxBsigsPP(r);
            off = i % maxBsigsPP(r);
            Bits Bsig = newBits(bsigBits(r));
            Page p = getPage(r->bsigf, pid);
            getBits(p, off, Bsig);
            andBits(matches, Bsig);
            q->nsigs++;
            if (nb == 0 || id < pid) {
                id = pid;
                q->nsigpages++;
            }
            nb++;
            free(Bsig);
        }
    }

    for (int i = 0; i < nPages(r); i++) {
        if (bitIsSet(matches, i)) {
            setBit(q->pages, i);
        }
    }

//	assert(q != NULL);
//	Reln r = q->rel;
//	Bits Qsig = makePageSig(r, q->qstring);
//	setAllBits(q->pages);
//	PageID pid; PageID off; PageID id;
//	int nb = 0;
//	for (int i = 0; i < psigBits(r); i++) {
//	    if (bitIsSet(Qsig, i)) {
//	        pid = i / maxBsigsPP(r);
//	        off = i % maxBsigsPP(r);
//            Bits Bsig = newBits(bsigBits(r));
//	        Page p = getPage(r->bsigf, pid);
//            getBits(p, off, Bsig);
//            q->nsigs++;
//            if (nb == 0 || id < pid) {
//                id = pid;
//                q->nsigpages++;
//            }
//            nb++;
//            for (int j = 0; j < nPsigs(r); j++) {
//                if (!bitIsSet(Bsig, j))
//                    unsetBit(q->pages, j);
//            }
//            free(Bsig);
//	    }
//	}

}

