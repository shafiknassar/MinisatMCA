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
    void         LitBitMapToVec(vec<Lit>& assum);  // create vector of assumptions based on a bitmap masked on init assum
    void 		 VecToLitBitMap(const vec<Lit>& assum);

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

    void     iterativeDel  (vec<Lit> &result);
    void     iterativeDel2 (vec<Lit> &result);
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

void AssumMinimiser::LitBitMapToVec(vec<Lit>& assum) {
	for (int i = 0; i < initAssum.size(); ++i) {
		if (litBitMap[initAssum[i]])
			assum.push(initAssum[i]);
	}
}

void AssumMinimiser::VecToLitBitMap(const vec<Lit>& assum) {
	for (int i = 0; i < initAssum.size(); ++i) {
		litBitMap[initAssum[i]] = false;
	}
	for (int i = 0; i < assum.size(); ++i) {
		assert(litBitMap.has(assum[i]));
		litBitMap[assum[i]] = true;
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

    for (int i = 0; i < initAssum.size(); ++i) {
        p = currAssum.peek();
        TRACE("Removing " << var(p) << " from currAssum");
        currAssum.pop();
        currAssum.toVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(var(p) << " is essential");
        	TRACE("Added it back to currAssum");
            currAssum.insert(p);
        } else {
        	TRACE(var(p) << " isn't essential");
        }
        vecAssum.clear(true);
    }
    currAssum.toVec(result);
    return;
}

void AssumMinimiser::iterativeDel2(vec<Lit> &result) {
    Lit p = lit_Undef;
    lbool ret;
    result.clear(false);
    vec<Lit> vecAssum;

    if (isSatWithAssum() == l_True) return;

    for (int i = 0; i < initAssum.size(); ++i) {
        if(litBitMap[initAssum[i]] == false) continue;
        litBitMap[initAssum[i]] = false;
        TRACE("Removing " << var(p) << " from currAssum");
        LitBitMapToVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(var(p) << " is essential");
        	TRACE("Added it back to currAssum");
            litBitMap[initAssum[i]] = true;
        } else {
        	TRACE(var(p) << " isn't essential" << std::endl
        			<< "Updating current assumptions");
        	VecToLitBitMap(s.conflict);
        }
        vecAssum.clear(true);
    }
    LitBitMapToVec(result);
    return;
}

/* TODO implement insertion algorithm */
/* TODO try to minimize the SAT calls */


}

#endif /* CORE_ASSUMMINIMISER_H_ */
