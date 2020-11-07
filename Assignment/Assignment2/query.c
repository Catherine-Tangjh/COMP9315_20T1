// query.c ... query scan functions
// part of SIMC signature files
// Manage creating and using Query objects
// Written by John Shepherd, March 2020

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"
#include "tsig.h"
#include "psig.h"
#include "bsig.h"

// check whether a query is valid for a relation
// e.g. same number of attributes

int checkQuery(Reln r, char *q)
{
	if (*q == '\0') return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',') nattr++;
	return (nattr == nAttrs(r));
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q, char sigs)
{
	Query new = malloc(sizeof(QueryRep));
	assert(new != NULL);
	if (!checkQuery(r,q)) return NULL;
	new->rel = r;
	new->qstring = q;
	new->nsigs = new->nsigpages = 0;
	new->ntuples = new->ntuppages = new->nfalse = 0;
	new->pages = newBits(nPages(r));
	switch (sigs) {
	case 't': findPagesUsingTupSigs(new); break;
	case 'p': findPagesUsingPageSigs(new); break;
	case 'b': findPagesUsingBitSlices(new); break;
	default:  setAllBits(new->pages); break;
	}
	new->curpage = 0;
	return new;
}

// scan through selected pages (q->pages)
// search for matching tuples and show each
// accumulate query stats

void scanAndDisplayMatchingTuples(Query q)
{
	assert(q != NULL);

	Bits pages = q->pages;
	Reln r = q->rel;
	File searchFile = dataFile(r);

	for(int i = 0; i < nPages(r); i++){ // scan through all pages to be examined for query
		q->curpage = i;
		int found = 0;
		if(!bitIsSet(pages, i)) continue; // Page not in matching list, ignore this page
		Page p = getPage(searchFile, i); // Get page by pageID, which is i here
		for(int j = 0; j < pageNitems(p); j++){ // scan through all tuples in current page
			q->curtup = j;
			Tuple t = getTupleFromPage(r, p, j);
			if (tupleMatch(r, t, q->qstring)){
				found++;
				showTuple(r, t);
			}
			q->ntuples++;
		}
		q->ntuppages++;
		if (!found){
//            printf("page not found: %d\n",i);
			q->nfalse++;
		}
	}
	
}

// print statistics on query

void queryStats(Query q)
{
	printf("# sig pages read:    %d\n", q->nsigpages);
	printf("# signatures read:   %d\n", q->nsigs);
	printf("# data pages read:   %d\n", q->ntuppages);
	printf("# tuples examined:   %d\n", q->ntuples);
	printf("# false match pages: %d\n", q->nfalse);
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->pages);
	free(q);
}

