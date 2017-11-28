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
    int numOfCalls;

public:
    AssumMinimiser(Solver& s, vec<Lit> assum) : s(s), numOfCalls(0){
        initAssum(assum);
        currAssum(assum);
    }
    a
};



}

#endif /* CORE_ASSUMMINIMISER_H_ */
