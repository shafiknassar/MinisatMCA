/*******************************************************************************************[Map.h]
Copyright (c) 2006-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Minisat_Map_h
#define Minisat_Map_h

#include "mtl/IntTypes.h"
#include "mtl/Vec.h"
#include "mca/global_defs.h"

namespace Minisat {

//=================================================================================================
// Default hash/equals functions
//

template<class K> struct Hash  { uint32_t operator()(const K& k)               const { return hash(k);  } };
template<class K> struct Equal { bool     operator()(const K& k1, const K& k2) const { return k1 == k2; } };

template<class K> struct DeepHash  { uint32_t operator()(const K* k)               const { return hash(*k);  } };
template<class K> struct DeepEqual { bool     operator()(const K* k1, const K* k2) const { return *k1 == *k2; } };

static inline uint32_t hash(uint32_t x){ return x; }
static inline uint32_t hash(uint64_t x){ return (uint32_t)x; }
static inline uint32_t hash(int32_t x) { return (uint32_t)x; }
static inline uint32_t hash(int64_t x) { return (uint32_t)x; }


//=================================================================================================
// Some primes
//

static const int nprimes          = 25;
static const int primes [nprimes] = { 31, 73, 151, 313, 643, 1291, 2593, 5233, 10501, 21013, 42073, 84181, 168451, 337219, 674701, 1349473, 2699299, 5398891, 10798093, 21596719, 43193641, 86387383, 172775299, 345550609, 691101253 };

//=================================================================================================
// Hash table implementation of Maps
//

template<class K, class D, class H = Hash<K>, class E = Equal<K> >
class Map {
 public:
    struct Pair { K key; D data; };

 private:
    H          hash;
    E          equals;

    vec<Pair>* table;
    int        cap;
    int        sz;

    // Don't allow copying (error prone):
    Map<K,D,H,E>&  operator = (Map<K,D,H,E>& other) { assert(0); }
                   Map        (Map<K,D,H,E>& other) { assert(0); }

    bool    checkCap(int new_size) const { return new_size > cap; }

    int32_t index  (const K& k) const { return hash(k) % cap; }
    void   _insert (const K& k, const D& d) { 
        vec<Pair>& ps = table[index(k)];
        ps.push(); ps.last().key = k; ps.last().data = d; }

    void    rehash () {
        const vec<Pair>* old = table;

        int old_cap = cap;
        int newsize = primes[0];
        for (int i = 1; newsize <= cap && i < nprimes; i++)
           newsize = primes[i];

        table = new vec<Pair>[newsize];
        cap   = newsize;

        for (int i = 0; i < old_cap; i++){
            for (int j = 0; j < old[i].size(); j++){
                _insert(old[i][j].key, old[i][j].data); }}

        delete [] old;

        // printf(" --- rehashing, old-cap=%d, new-cap=%d\n", cap, newsize);
    }

    int globalIter, perTableIter;

    
 public:

    Map () : table(NULL), cap(0), sz(0) {}
    Map (const H& h, const E& e) : hash(h), equals(e), table(NULL), cap(0), sz(0){}
    ~Map () { delete [] table; }

    // PRECONDITION: the key must already exist in the map.
    const D& operator [] (const K& k) const
    {
        assert(sz != 0);
        const D*         res = NULL;
        const vec<Pair>& ps  = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (equals(ps[i].key, k))
                res = &ps[i].data;
        assert(res != NULL);
        return *res;
    }

    // PRECONDITION: the key must already exist in the map.
    D& operator [] (const K& k)
    {
        assert(sz != 0);
        D*         res = NULL;
        vec<Pair>& ps  = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (equals(ps[i].key, k))
                res = &ps[i].data;
        assert(res != NULL);
        return *res;
    }

    // PRECONDITION: the key must *NOT* exist in the map.
    void insert (const K& k, const D& d) { if (checkCap(sz+1)) rehash(); _insert(k, d); sz++; }

    bool peek   (const K& k, D& d) const {
        if (sz == 0) return false;
        const vec<Pair>& ps = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (equals(ps[i].key, k)){
                d = ps[i].data;
                return true; } 
        return false;
    }

    bool has   (const K& k) const {
        if (sz == 0) return false;
        const vec<Pair>& ps = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (equals(ps[i].key, k))
                return true;
        return false;
    }

    bool getIfContains(const K& k, D *out) const
    {
        if (sz == 0) return false;
        const vec<Pair>& ps = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (equals(ps[i].key, k))
            {
            	*out = ps[i].data;
                return true;
            }
        return false;
    }

    // PRECONDITION: the key must exist in the map.
    void remove(const K& k) {
        assert(table != NULL);
        vec<Pair>& ps = table[index(k)];
        int j = 0;
        for (; j < ps.size() && !equals(ps[j].key, k); j++);
        assert(j < ps.size());
        ps[j] = ps.last();
        ps.pop();
        sz--;
    }

    void clear  () {
    	//TRACE((void*)table);
        cap = sz = 0;
        delete [] table;
        table = NULL;
    	//TRACE_END_FUNC;
    }

    int  elems()        const { return sz; }
    int  size()         const { return sz; }
    int  bucket_count() const { return cap; }

    // NOTE: the hash and equality objects are not moved by this method:
    void moveTo(Map& other){
        delete [] other.table;

        other.table = table;
        other.cap   = cap;
        other.sz  = sz;

        table = NULL;
        sz = cap = 0;
    }

    // NOTE: given a bit more time, I could make a more C++-style iterator out of this:
    const vec<Pair>& bucket(int i) const { return table[i]; }
    K keyBucket(int i)             const { const Pair& p = table[i]; return p.key; }

    // key iteration methods:


    K* startLoop()
    {
    	globalIter = 0, perTableIter = 0;
    	assert(table);
    	//TRACE("Map full state: ");
    	//TRACE("          size: " << sz);
    	//TRACE("         table: " << table);
    	while (table[globalIter].size() == 0) globalIter++;
    	return &(table[globalIter][perTableIter++].key);
    }
    K* getNext()
    {
    	if (perTableIter < table[globalIter].size())
    	{
    		//TRACE("In same table!");
    		return &(table[globalIter][perTableIter++].key);
    	}
    	while (++globalIter < cap)
    	{
    		perTableIter = 0;
    		if (table[globalIter].size() > 0)
    		    return &(table[globalIter][perTableIter++].key);
    	}
    	TRACE("passed cap - iteration finished");
    	return NULL;
    }

    bool setIfContains(const K& k, const D& d)
    {
    	//D* res = NULL;
        vec<Pair>& ps  = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
        {
            if (equals(ps[i].key, k))
        	{
        	    ps[i].data = d;
       	        return true;
            }
        }
        return false;
    }
};

#define MAP_FOREACH(l, bm)  \
	for (l = bm.startLoop(); l != NULL; l = bm.getNext())


//=================================================================================================
}

#endif
