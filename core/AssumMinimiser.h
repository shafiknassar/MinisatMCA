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
    vec<Lit>     initAssum;
    Queue<Lit>   currAssum;
    //Statistics
    int          nSolveCalls, nSAT, nUNSAT;
    lbool        isSatWith, isSatWo;         //specifies if the formula is sat w/o assum

public:

    AssumMinimiser(Solver& s, vec<Lit>& assum) : s(s), initAssum(), currAssum(), isSatWith(l_Undef),
                                                 isSatWo(l_Undef) {
        initAssum.copyFrom(assum);
        nSolveCalls = nSAT = nUNSAT = 0;
        currAssum.fromVec(assum);
        isSatWith = isSatWo = l_Undef;
    }

    /** These two methods checks if the formula is SAT with and without the assumptions
     * respectively, if it's SAT with the assumptions or UNSAT without the assumptions, then
     * minimizing the conflicting assumptions is irrelevant or trivial - respectively.
     */
    lbool    isSatWithAssum();
    lbool    isSatWoAssum  ();

    void     iterativeDel  (vec<Lit> &result);
    //void    PrintStats    () const;
};

lbool AssumMinimiser::isSatWithAssum() {
    if (isSatWith == l_Undef) {
        isSatWith = s.solveLimited(initAssum);
        if (isSatWith == l_True) isSatWo = l_True;
    }
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
    vec<Lit> tmpAssum;
    if (isSatWithAssum() == l_True) return;

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
