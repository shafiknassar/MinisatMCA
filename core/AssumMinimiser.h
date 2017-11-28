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

namespace Minisat {

class AssumMinimiser {
    Solver& s;
    vec<Lit> initAssum;
    Queue<Lit> currAssum;
    //Statistics
    int nSolveCalls, nSAT, nUNSAT;
    lbool isSat;         //specifies if the formula is sat w/o assum

public:
    AssumMinimiser(Solver& s, vec<Lit> assum) : s(s), initAssum(), currAssum(assum) {
        initAssum.copyFrom(assum);
        nSolveCalls = nSAT = nUNSAT = 0;

    }

    vec<Lit> iterativeDel();
    void     PrintStats() const;
};

vec<Lit> AssumMinimiser::iterativeDel() {
    Lit p = lit_Undef;
    lbool ret;

    if (isSat == l_Undef) isSat = s.solveLimited(vec<Lit>());
    if (isSat == l_False) return vec<Lit>();

    /*TODO check that the copy c'tors work as they should initAssum == currAssum */

    for (int i = 0; i < initAssum.size(); ++i) {
        p = currAssum.peek();
        currAssum.pop();
        ret = s.solveLimited(currAssum.toVec());
        if (ret == l_True)
            currAssum.insert(p);
    }

    return currAssum.toVec();
}


}

#endif /* CORE_ASSUMMINIMISER_H_ */
