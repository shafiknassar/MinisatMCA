/*
 * AssumMinimiser.h
 *
 *  Created on: Nov 28, 2017
 *      Author: pierre
 */

#ifndef CORE_ASSUMMINIMISER_H_
#define CORE_ASSUMMINIMISER_H_

#define DEBUG 1
//#undef DEBUG

#ifdef DEBUG
#include <iostream>
#define TRACE(s)	std::cout << __FUNCTION__ << ": " << s << std::endl
#elif
#define TRACE(s) ;
#endif


#include "core/Solver.h"
#include "mtl/Queue.h"

namespace Minisat {

class AssumMinimiser {
    Solver& s;
    vec<Lit>     initAssum;
    Queue<Lit>   currAssum;
    //Statistics
    int          nSolveCalls, nSAT, nUNSAT;
    lbool        isSat;         //specifies if the formula is sat w/o assum

public:
    AssumMinimiser(Solver& s, vec<Lit>& assum) : s(s), initAssum(), currAssum() {
        initAssum.copyFrom(assum);
        nSolveCalls = nSAT = nUNSAT = 0;
        currAssum.fromVec(assum);
    }

    void    iterativeDel (vec<Lit> &result);
    void
    void    PrintStats   () const;
};

/*
 * Implementation of the most naive algorithm.
 * We just go through all of the assumption, trying to remove them 1 by 1.
 * If the cnf is UNSAT without assumption L, then L is not crucial
 * to the conflict thus is removed.
 * If it is SAT, then L was crucial to the conflict and should
 * remain as part of the minimal set of conflicting assumptions.
 * */
void AssumMinimiser::iterativeDel(vec<Lit> &result) {
    Lit p = lit_Undef;
    lbool ret;
    result.clear(false);
    vec<Lit> tmpAssum;
    if (isSat == l_Undef) isSat = s.solveLimited(vec<Lit>());
    if (isSat == l_False) return;

    for (int i = 0; i < initAssum.size(); ++i) {
        p = currAssum.peek();
        TRACE("Removing " << var(p) << " from currAssum");
        currAssum.pop();
        currAssum.toVec(tmpAssum);
        TRACE("Begin Solving");
        ret = s.solveLimited(tmpAssum);
        TRACE("Solving ended");
        if (ret == l_True)
        {
        	TRACE(var(p) << " is essential");
        	TRACE("Added it back to currAssum");
            currAssum.insert(p);
        } else {
        	TRACE(var(p) << " isn't essential");
        }
        tmpAssum.clear(true);
    }
    currAssum.toVec(result);
    return;
}


}

#endif /* CORE_ASSUMMINIMISER_H_ */