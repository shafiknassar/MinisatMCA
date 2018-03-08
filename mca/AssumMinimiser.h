/*
 * AssumMinimiser.h
 *
 *  Created on: Nov 28, 2017
 *      Author: pierre
 */

#ifndef CORE_ASSUMMINIMISER_H_
#define CORE_ASSUMMINIMISER_H_

#include "mca/Solver.h"
#include "mtl/Queue.h"
#include "mca/global_defs.h"

#define INIT_ASSUM_BITMAP(bm)              \
	do                                     \
    {                                      \
        bm.clear();                        \
	    foreach (i, initAssum.size())      \
        {                                  \
            bm.insert(initAssum[i], true); \
         }                                 \
    } while(0)

#define INIT_ANTI_ASSUM_BITMAP(bm)         \
	do                                     \
    {                                      \
        bm.clear();                        \
	    foreach (i, initAssum.size())      \
        {                                  \
            bm.insert(~initAssum[i], true);\
         }                                 \
    } while(0)

/*
#define INIT_NON_ASSUM_BITMAP(bm)          \
	do                                     \
    {                                      \
        bm.clear();                        \
        foreach(i, s.nVars)                \
		{ \
        	\
        } \
	    foreach (i, initAssum.size())      \
        {                                  \
            bm.insert(initAssum[i], true); \
         }                                 \
    } while(0)
*/

#define SOLVER_STATS_TABLE \
	X(starts),             \
	X(conflicts),          \
	X(decisions),          \
	X(rnd_decisions),      \
	X(propagations)        \


namespace Minisat {

/***************************************/
/* Types and Logic used by AssumMinimiser */
/***************************************/

class LitHash {
public:
	uint32_t operator()(const Lit& k) const {
		return var(k);
	}
};


typedef Map<Lit, bool, LitHash> LitBitMap;


// Returns a vec<Lit> that contains all mutual literals in all the given clauses
// Time Complexity: O( c * l * l )
// * when c = |clauses|, l=max{literals per clause}
// Notes:
// res is dynamically allocated - must be deleted
// assumes no clause contains same literal more than once
vec<Lit>* getMutualLiteralsInClauses(vec<vec<Lit>*>& clauses)
{
	TRACE_START_FUNC;
	vec<Lit> *lastClause = clauses.last();
	vec<Lit> *res = new vec<Lit>;
	LitBitMap bm;
	Lit *pL = NULL;

	// initializing bitmap as the last clause
	foreach(i, lastClause->size())
	{
		TRACE("Adding to bitmap: " << (*lastClause)[i].toString());
		bm.insert((*lastClause)[i], true);
	}

	// marking out all literals that are not contained in a clause
	foreach(i, clauses.size())
	{
		TRACE("Checking clause: " << clauses[i]->toString());
	    MAP_FOREACH(pL, bm)
		{
	    	if (!(clauses[i]->contains(*pL)))
	    	{
	    		TRACE("    Removing: " << pL->toString());
	    		bm[*pL] = false;
	    	}
		}
	}

	// copying the unmarked literals to the returned vector
	MAP_FOREACH(pL, bm)
	{
		TRACE("Checking: " << pL->toString());
		if (true == bm[*pL]){
			TRACE("    Added to OUTPUT");
			res->push(*pL);
		}
	}

	TRACE_END_FUNC;
	return res;
}

/* Time Complexity: O( c * l * a )
 * 		when c = |clauses|, l=max{literals per clause}, a=|assums|
 *
 * 	Parameters:
 * 		clauses - list of clauses.
 * 		assums - the set of assumptions
 */
// Notes:
// res is dynamically allocated - must be deleted
// assumes no clause contains same literal more than once
vec<Lit>* getMutualAssumptionsInClauses(vec<vec<Lit>*>& clauses, vec<Lit>& assum)
{
	TRACE_START_FUNC;
	vec<Lit> *res = NULL;
	clauses.push(&assum);
	res = getMutualLiteralsInClauses(clauses);
	clauses.pop();
	TRACE_END_FUNC;
	return res;
}

class AssumMinimiser {
    Solver& s;
    vec<Lit>     initAssum; // Must not be edited after c'tor !!!
    LitBitMap litBitMap;

    // TODO: add flags for solver limitations: yield after a certain number of conflicts, time, decisions...

    //Statistics TODO might want to add run times for SAT, UNSAT separately
    int          nSAT, nUNSAT;
    int          nSolveCalls() const         { return nSAT+nUNSAT; }
    lbool        isSatWith, isSatWo;         //specifies if the formula is sat w/o assum
    // TODO statistics for per SAT, UNSAT (cpu_time), initial run.
    // assumptions progress along the way and in the end.

    int verbosity;
    // TODO find a way to print how many assumptions were minimized
    // TODO global timeout

#define X(s) curr_##s
    uint64_t SOLVER_STATS_TABLE;
#undef X

#define X(s) total_##s
    uint64_t SOLVER_STATS_TABLE;
#undef X

    double curr_cpu_time, total_cpu_time;
    /*
     * Private helper functions
     * */

    lbool        solveWithAssum(vec<Lit>& assum);
    // create vector of assumptions based on a bitmap masked on init assum.
    //INVARIANT: the assum is a subset of init assum!
    void         litBitMapToVec(vec<Lit>& assum);
    // create bit map of literals based on the vector assum.
    //INVARIANT: same as previous method + assum is created by minisat!
    void 		 vecToLitBitMap(const vec<Lit>& assum);

    bool         isAssum(Lit l) {
    	foreach(i, initAssum.size()) if (initAssum[i] == l) return true; return false;
    }
    bool         isConfWithAssum(Lit l) { return isAssum(~l); }


public:

    AssumMinimiser(Solver& s, vec<Lit>& assum) : s(s), initAssum(), isSatWith(l_Undef),
                                                 isSatWo(l_Undef) {
#define X(s) curr_##s = 0, total_##s = 0
    	SOLVER_STATS_TABLE;
#undef X
    	curr_cpu_time = total_cpu_time = 0;
        initAssum.copyFrom(assum);
        nSAT = nUNSAT = 0;
        isSatWith = isSatWo = l_Undef;
        verbosity = s.verbosity;
        litBitMap.clear();
        TRACE("Init assums are: " << initAssum.toString());
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

    void     rotationAlg   (vec<Lit> &result);

    void     printCurrentStats();

    void     PrintStats    () const;

    bool   tryToRotate   (vec<lbool>& model, Lit assum, Lit& newVital);
};

void     AssumMinimiser::PrintStats    () const {
	printf("\n\n Total Statistics:\n\n");
	printSolverStats(s);
	printf("num of SAT calls      : %d\n", nSAT);
	printf("num of UNSAT calls    : %d\n", nUNSAT);
	printf("total calls           : %d\n", nSolveCalls());
}

lbool    AssumMinimiser::solveWithAssum(vec<Lit>& assum) {
    lbool ret;
    TRACE("Begin Solving");
    if (assum.size() == 0)
    {
    	return isSatWoAssum();
    }
    ret = s.solveLimited(assum);
    TRACE("Solving ended");
    if (ret == l_True) {
        TRACE("SAT");
        nSAT++;
    } else {
        TRACE("UNSAT");
        nUNSAT++;
    }
    printCurrentStats();
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
        printCurrentStats();
        if (isSatWith == l_True) isSatWo = l_True;
    }
    //if (isSatWith) TRACE()
    return isSatWith;
}


lbool AssumMinimiser::isSatWoAssum() {
    if (isSatWo == l_Undef) {
        isSatWo = s.solveLimited(vec<Lit>());
        printCurrentStats();
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

    INIT_ASSUM_BITMAP(litBitMap);

    foreach(i, initAssum.size()) {
        if(litBitMap[initAssum[i]] == false) continue;
        litBitMap[initAssum[i]] = false;
        TRACE("Removing " << initAssum[i].toString() << " from bitMap");
        litBitMapToVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(initAssum[i].toString() << " is essential");
        	TRACE("Added it back to currAssum");
            litBitMap[initAssum[i]] = true;
        } else {
        	TRACE(initAssum[i].toString() << " isn't essential" << std::endl
        			<< "Updating current assumptions");
        	vecToLitBitMap(s.conflict);
        }
        vecAssum.clear(true);
    }
    litBitMapToVec(result);
    return;
}


void AssumMinimiser::rotationAlg(vec<Lit> &result) {
    lbool ret;
    result.clear(false);
    vec<Lit> vecAssum;
    Lit newVitalAssum;

    TRACE_START_FUNC;

    if (isSatWithAssum() == l_True) return;

    INIT_ASSUM_BITMAP(litBitMap);

    foreach(i, initAssum.size()) {
        if(litBitMap[initAssum[i]] == false) continue;
        litBitMap[initAssum[i]] = false;
        TRACE("Removing " << initAssum[i].toString() << " from currAssum");
        // TODO: Maybe it would be more efficient to send in ~initAssum[i]?
        litBitMapToVec(vecAssum);
        ret = solveWithAssum(vecAssum);
        if (ret == l_True) {
        	TRACE(initAssum[i].toString() << " is vital");
        	TRACE("Added it back to currAssum");
            litBitMap[initAssum[i]] = true;
            newVitalAssum = lit_Undef;
        	if (tryToRotate(this->s.model, initAssum[i], newVitalAssum))
        	{
        		litBitMap[newVitalAssum] = false;
        	}
        } else {
        	TRACE(initAssum[i].toString() << " isn't essential" << std::endl
        			<< "Updating current assumptions");
        	vecToLitBitMap(s.conflict);
        }
        vecAssum.clear(true);
    }
    litBitMapToVec(result);
    TRACE_END_FUNC;
    return;
}


void AssumMinimiser::iterativeIns(vec<Lit> &result) {
	lbool res;
	vec<Lit> tmp;
	Lit *lit;
    if (isSatWithAssum() == l_True) return;
    result.clear(true);
    INIT_ASSUM_BITMAP(litBitMap);

    while (solveWithAssum(result) == l_True)
    {
    	tmp.copyFrom(result);
    	MAP_FOREACH(lit, litBitMap)
    	{
    		tmp.push(*lit);
    		if (solveWithAssum(result) == l_False)
    		{
    			litBitMap.remove(*lit);
    			result.push(*lit);
    			break;
    		}
    	}
    }
	return;
}

void flipVarInModel(vec<lbool>& model, int var)
{
	model[var] = ~model[var];
}




/*
 * The primary function in the rotation algorithm.
 * Parameters:
 *    * model - a vector of literals, defines an assignment to all of the variables
 *    * vitalAssum - the vital assumption, it must NOT hold under the current given model
 *    * newVital - output:
 * */
bool   AssumMinimiser::tryToRotate   (vec<lbool>& model, Lit vitalAssum, Lit& newVital)
{
	// Local variables
	vec<vec<Lit>*>  clausesContainingVitAssum;
	vec<vec<Lit>*>  clausesContainingBroker;
	vec<Lit>*       pMutualLiterals             = NULL;
	vec<Lit>*       pBrokerMutualLiterals       = NULL;
	bool            res                         = false;
	Lit             l                           = lit_Undef;
	Lit             k                           = lit_Undef;

	TRACE_START_FUNC;
	// Initializing
	newVital = lit_Undef;
	s.getWeakClausesContaining(~vitalAssum, clausesContainingVitAssum);
	pMutualLiterals =
			getMutualLiteralsInClauses(clausesContainingVitAssum); // dynamic alloc
	/* original vital assumption must be flipped back
	 * to the assignment in order to find new vitals */
	/*flipOut*/flipVarInModel(model, var(vitalAssum));

	/* if there's only 1, then it's the original vital assumption*/
	if (1 == pMutualLiterals->size())
	{
		goto CLEANUP;
	}
	assert(pMutualLiterals->size() > 1);

	TRACE("Mutual Literals are: " << pMutualLiterals->toString());

	/* loop through mutualLiterals to find newVital or
	 * find a broker that can lead to newVital */
	foreach(iL, pMutualLiterals->size())
	{
		l = (*pMutualLiterals)[iL];
		// we don't want to mistake the old vital as a new vital
		if (var(l) == var(vitalAssum)) continue;

		if (isConfWithAssum(l))    // l is a potential newVital
		{
			/* flipping the new potential vital assumption out of the model*/
			/*flipOut*/flipVarInModel(model, var(l));
			TRACE("Found a potential vital: " << l.toString());
			if (s.checkIfModel(model))
			{
				/* remember that l is conflicting with
				 * the assumptions, so ~l is an assumption */
				newVital = ~l;
				res = true;
				goto CLEANUP;
			}
		}
		else    // l is a potential broker
		{
			TRACE("Found a potential broker: " << l.toString());

			/* similar to what we did to vitalAssum
			 * l is only a broker */
			s.getWeakClausesContaining(~l, clausesContainingBroker);
			pBrokerMutualLiterals =
					getMutualLiteralsInClauses(clausesContainingBroker); //dynamic alloc
			TRACE("Mutual Literals are: " << pBrokerMutualLiterals->toString());
			/* flipping the new potential vital assumption out of the model*/
			/*flipOut*/flipVarInModel(model, var(l));
			if (pBrokerMutualLiterals->size() > 1)
			{
				TRACE("Broker " << l.toString()
						<< " has mutualLiterals: " << pBrokerMutualLiterals->toString());
				foreach(iV, pBrokerMutualLiterals->size())
				{
					k = (*pBrokerMutualLiterals)[iV];

					if (isConfWithAssum(k))
					/* can't be equal to l since
			     	 * l is not conflicting with assumptions */
					{
						TRACE("Under broker: " << l.toString() <<
								"Found a potential vital: " << k.toString());
						/*flipOut*/flipVarInModel(model, var(k));
						if (s.checkIfModel(model))
						{
							newVital = ~k;
							res = true;
							delete pBrokerMutualLiterals;
							goto CLEANUP;
						}
						/*flipIn*/flipVarInModel(model, var(k));
					}
				}
			}
			assert(pBrokerMutualLiterals->size() >= 1);
			delete pBrokerMutualLiterals;
		}
		/*flipIn*/flipVarInModel(model, var(l));
	}

CLEANUP:
/*flipIn*/flipVarInModel(model, var(vitalAssum));
    delete pMutualLiterals;
    if (res) TRACE("FOUND VITAL ASSUMPTION: " << newVital.toString());
    else     TRACE("NO NEW VITAL FOUND");
    TRACE_END_FUNC;
    return res;
}






void AssumMinimiser::printCurrentStats()
{
	uint64_t starts = s.starts,
			conflicts = s.conflicts,
			decisions = s.decisions,
			rnd_decisions = s.rnd_decisions,
			propagations = s.propagations;

#define X(str)  curr_##str = str - total_##str
    SOLVER_STATS_TABLE;
#undef X

#define X(str)   total_##str = str
    SOLVER_STATS_TABLE;
#undef X

    curr_cpu_time = cpuTime() - total_cpu_time;
    total_cpu_time = cpuTime();
    printf("restarts              : %"PRIu64"\n", curr_starts);
    printf("conflicts             : %-12"PRIu64"   (%.0f /sec)\n", curr_conflicts   , curr_conflicts   /curr_cpu_time);
    printf("decisions             : %-12"PRIu64"   (%4.2f %% random) (%.0f /sec)\n", curr_decisions, (float)curr_rnd_decisions*100 / (float)curr_decisions, curr_decisions   /curr_cpu_time);
    printf("propagations          : %-12"PRIu64"   (%.0f /sec)\n", curr_propagations, curr_propagations/curr_cpu_time);
    printf("CPU time              : %g s\n", curr_cpu_time);
}



/* TODO try to minimize the SAT calls */
////////////////////////////////////////
/*TRACE_START_FUNC;

vec<vec<Lit>*> clausesWithAssum, clausesWithLit;
s.getWeakClausesContaining(~assum, clausesWithAssum);
LitBitMap bm, sbm;
Lit *l, *k;
lbool res;

getMutualLiterals(clausesWithAssum, &bm);
if (bm.size() == 1)
{
	TRACE("No literals mutual for all relevant clauses!");
	TRACE_END_FUNC;
	return false;
}
TRACE("Flipping the vital assumption back to the model " << assum.toString());
flipVarInModel(model, var(assum));
TRACE("Marking the vital assum as false to skip it in iterations");
bm[~assum] = false;
MAP_FOREACH(l, bm)
{
	assert(l);
	if(!bm[*l]) continue;
	TRACE("Flipping: " << l->toString());
	flipVarInModel(model, var(*l));
	if (isConfWithAssum(*l))
	{
		if (s.checkIfModel(model))
		{
			newVital = *l;
			TRACE("found new vital assumption: " << newVital.toString());
			TRACE_END_FUNC;
			return true;
		}
	}
	else // *l is not an assum
	{
        TRACE(initAssum.size() << "initAssum : " << initAssum.toString());
		INIT_ASSUM_BITMAP(sbm);
		TRACE(sbm.size() << " Init Assums are:");
		MAP_FOREACH(k, sbm)
		{
			TRACE("    " << k->toString());
		}
		sbm[assum] = false;
		//TODO: check if ~
		s.getWeakClausesContaining(~*l, clausesWithLit);
		getMutualAssumptions(clausesWithLit, &sbm);
		MAP_FOREACH(k, sbm)
		{
			if (!sbm[assum]) continue;
			TRACE("Flipping: " << k->toString());
			flipVarInModel(model, var(*k));
			if (s.checkIfModel(model))
			{
				newVital = *k;
				TRACE("tryToRotate: found new vital assumption: " << newVital.toString());
				TRACE_END_FUNC;
				return true;
			}
			flipVarInModel(model, var(*k));
		}
	}
	flipVarInModel(model, var(*l));
}
TRACE("tryToRotate: no new vital found");
TRACE_END_FUNC;
return false;
*/

}

#endif /* CORE_ASSUMMINIMISER_H_ */
