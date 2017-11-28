/*
 * AssumMinimiser.h
 *
 *  Created on: Nov 28, 2017
 *      Author: pierre
 */

#ifndef CORE_ASSUMMINIMISER_H_
#define CORE_ASSUMMINIMISER_H_

#include "core/Solver.h"

namespace Minisat {

class AssumMinimiser {
    Solver& s;
    vec<Lit> initAssum, currAssum;

    //Statistics
    int nSolveCalls, nSAT, nUNSAT;

public:
    AssumMinimiser(Solver& s, vec<Lit> assum) : s(s){
        initAssum(assum);
        currAssum(assum);
        nSolveCalls = nSAT = nUNSAT = 0;
    }

};



}

#endif /* CORE_ASSUMMINIMISER_H_ */
