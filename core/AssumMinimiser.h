/*
 * AssumMinimiser.h
 *
 *  Created on: Nov 28, 2017
 *      Author: pierre
 */

#ifndef CORE_ASSUMMINIMISER_H_
#define CORE_ASSUMMINIMISER_H_

#include "core/Solver.h"
#include "mtl/Queue.h"
#include "core/global_defs.h"

namespace Minisat {

class AssumMinimiser {
    Solver& s;
    vec<Lit>     initAssum; // Must not be edited after c'tor !!!
    class LitHash {
    public:
    	uint32_t operator()(const Lit& k) const {
    		return var(k);
    	}
    };
    Map<Lit, bool, LitHash> litBitMap;

    // TODO: add flags for solver limitations: yield after a certain number of conflicts, time, decisions...

    //Statistics TODO might want to add run times for SAT, UNSAT separately
    int          nSAT, nUNSAT;
    int          nSolveCalls()         { return nSAT+nUNSAT; }
    lbool        isSatWith, isSatWo;         //specifies if the formula is sat w/o assum
    /*
     * Private helper functions
     * */

    lbool        solveWithAssum(vec<Lit>& assum);
    // create vector of assumptions based on a bitmap masked on init assum. INVARIANT: the assum is a subset of init assum!
    void         litBitMapToVec(vec<Lit>& assum);
    // create bit map of literals based on the vector assum. INVARIANT: same as previous method + assum is created by minisat!
    void 		 vecToLitBitMap(const vec<Lit>& assum);

public:

    AssumMinimiser(Solver& s, vec<Lit>& assum) : s(s), initAssum(), isSatWith(l_Undef),
                                                 isSatWo(l_Undef) {
        initAssum.copyFrom(assum);
        nSAT = nUNSAT = 0;
        isSatWith = isSatWo = l_Undef;
        for (int i = 0; i < initAssum.size(); ++i) {
        	litBitMap.insert(initAssum[i], true);
        }
    }

    /** These two methods checks if the formula is SAT with and without the assumptions
     * respectively, if it's SAT with the assumptions or UNSAT without the assumptions, then
     * minimizing the conflicting assumptions is irrelevant or trivial - respectively.
     */
    lbool    isSatWithAssum();
    lbool    isSatWoAssum  ();
    /*
     * Implementations of the simple Iterative Deletion
     * */
    void     iterativeDel  (vec<Lit> &result);
    void     iterativeDel2 (vec<Lit> &result);
    /*
     * Implementations of the simple Iterative Insertion
     * */
    void     iterativeIns  (vec<Lit> &result);


    void     PrintStats    () const;
};

lbool    AssumMinimiser::solveWithAssum(vec<Lit>& assum) {
    lbool ret;
    TRACE("Begin Solving");
    ret = s.solveLimited(assum);
    TRACE("Solving ended");
    if (ret == l_True) {
        TRACE("SAT");
        nSAT++;
    } else {
        TRACE("UNSAT");
        nUNSAT++;
    }
    return ret;
}

void AssumMinimiser::litBitMapToVec(vec<Lit>& assum) {
	foreach(i, initAssum.size()) {
		if (litBitMap[initAssum[i]])
			assum.push(initAssum[i]);
	}
}

void AssumMinimiser::vecToLitBitMap(const vec<Lit>& assum) {
    Lit negLit;
    foreach(i, initAssum.size()) {
		litBitMap[initAssum[i]] = false;
	}
    foreach(i, assum.size()) {
        TRACE(assum[i].toString());
    }
    foreach(i, assum.size()) {
        negLit = ~assum[i];     // minisat saves the literal negated, so we negate it back to get the right value.
	    TRACE("Adding Lit=" << negLit.toString());
		assert(litBitMap.has(negLit));
		litBitMap[negLit] = true;
	}
}

lbool AssumMinimiser::isSatWithAssum() {
    if (isSatWith == l_Undef) {
        isSatWith = s.solveLimited(initAssum);
        if (isSatWith == l_True) isSatWo = l_True;
    }
    //if (isSatWith) TRACE()
    return isSatWith;
}


lbool AssumMinimiser::isSatWoAssum() {
    if (isSatWo == l_Undef) {
        isSatWo = s.solveLimited(vec<Lit>());
        if (isSatWo == l_False) isSatWith = l_False;
    }
    return isSatWo;
}

void AssumMinimiser::iterativeDel(vec<Lit> &result) {
    Lit p = lit_Undef;
    lbool ret;
    result.clear(false);
    vec<Lit> vecAssum;
    Queue<Lit> currAssum;
    currAssum.fromVec(initAssum);

    if (isSatWithAssum() == l_True) return;

    foreach(i, initAssum.size()) {
        p = currAssum.peek();
        TRACE("Removing " << p.toString() << " from currAssum");
        currAssum.pop();
        currAssum.toVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(p.toString() << " is essential");
        	TRACE("Added it back to currAssum");
            currAssum.insert(p);
        } else {
        	TRACE(p.toString() << " isn't essential");
        }
        vecAssum.clear(true);
    }
    currAssum.toVec(result);
    return;
}

void AssumMinimiser::iterativeDel2(vec<Lit> &result) {
    lbool ret;
    result.clear(false);
    vec<Lit> vecAssum;

    if (isSatWithAssum() == l_True) return;

    foreach(i, initAssum.size()) {
        if(litBitMap[initAssum[i]] == false) continue;
        litBitMap[initAssum[i]] = false;
        TRACE("Removing " << p.toString() << " from currAssum");
        litBitMapToVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(p.toString() << " is essential");
        	TRACE("Added it back to currAssum");
            litBitMap[initAssum[i]] = true;
        } else {
        	TRACE(p.toString() << " isn't essential" << std::endl
        			<< "Updating current assumptions");
        	vecToLitBitMap(s.conflict);
        }
        vecAssum.clear(true);
    }
    litBitMapToVec(result);
    return;
}

/* if CNF is UNSAT without assumptions, 1 literal will be
 * returned as a set of conflicting assumptions */
void AssumMinimiser::iterativeIns(vec<Lit> &result) {
	lbool res;
    if (isSatWithAssum() == l_True) return;
    /*
     * for performance optimization's sake,
     * we don't call the solver on an empty set of assumptions
     */
	foreach (i, initAssum.size()) {
		TRACE("Adding " << initAssum[i].toString() << " to Assumptions");
		result.push(initAssum[i]);
		res = solveWithAssum(result);
		if (res == l_False)
		{
			TRACE("Found Minimal Set Of Conflicting Assumptions!");
			return;
		}
	}
	assert(0);
	return;
}



/* TODO implement insertion algorithm */
/* TODO try to minimize the SAT calls */


}

#endif /* CORE_ASSUMMINIMISER_H_ */
