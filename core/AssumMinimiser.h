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
#define TRACE(s)	std::cout << __FUNCTION__ << ": " << s << std::endl
#elif
#define TRACE(s) ;
#endif


#include "core/Solver.h"
#include "mtl/Queue.h"
#include <iostream>

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
    void    PrintStats   () const;
};


void AssumMinimiser::iterativeDel(vec<Lit> &result) {
    Lit p = lit_Undef;
    lbool ret;
    result.clear(false);
    vec<Lit> tmpAssum;
    if (isSat == l_Undef) isSat = s.solveLimited(vec<Lit>());
    if (isSat == l_False) return;

    /*TODO check that the copy c'tors work as they should initAssum == currAssum */

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
