/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


--------------- Original Minisat Copyrights

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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
*************************************************************************************************/

#ifndef SolverTypes_h
#define SolverTypes_h

#include <assert.h>
#include <stdint.h>

#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>

namespace Candy {

//=================================================================================================
// Variables, literals, lifted booleans:

// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int Var;
#define var_Undef (-1)

struct Lit {
	int32_t x;

	operator int() const {
		return x;
	}

	// Use this as a constructor:
	friend Lit mkLit(Var var, bool sign);

	bool operator ==(Lit p) const {
		return x == p.x;
	}
	bool operator !=(Lit p) const {
		return x != p.x;
	}
	bool operator <(Lit p) const {
		return x < p.x;
	} // '<' makes p, ~p adjacent in the ordering.
};

inline Lit mkLit(Var var, bool sign = false) {
	Lit p;
    p.x = var + var + (sign ? 1 : 0);
	return p;
}

inline Var operator"" _V(unsigned long long n) {
    assert(n > 0);
    return static_cast<Var>(n-1);
}

inline Lit operator"" _L(unsigned long long n) {
    assert(n > 0);
    return mkLit(static_cast<int>(n-1), false);
}

inline Lit operator ~(Lit p) {
	Lit q;
	q.x = p.x ^ 1;
	return q;
}

inline Lit operator ^(Lit p, bool b) {
	Lit q;
    q.x = p.x ^ (b ? 1 : 0);
	return q;
}

inline bool sign(Lit p) {
	return p.x & 1;
}
inline int var(Lit p) {
	return p.x >> 1;
}

// Mapping Literals to and from compact integers suitable for array indexing:
inline int toInt(Var v) {
	return v;
}
inline int toInt(Lit p) {
	return p.x;
}
inline Lit toLit(int i) {
	Lit p;
	p.x = i;
	return p;
}

const Lit lit_Undef = { -2 };  // }- Useful special constants.
const Lit lit_Error = { -1 };  // }


//=================================================================================================
// Lifted booleans:
//
// NOTE: this implementation is optimized for the case when comparisons between values are mostly
//       between one variable and one constant. Some care had to be taken to make sure that gcc
//       does enough constant propagation to produce sensible code, and this appears to be somewhat
//       fragile unfortunately.

#define l_True  (lbool((uint8_t)0)) // gcc does not do constant propagation if these are real constants.
#define l_False (lbool((uint8_t)1))
#define l_Undef (lbool((uint8_t)2))

class lbool {
	uint8_t value;

public:
	explicit lbool(uint8_t v) :
			value(v) {
	}

	lbool() :
			value(0) {
	}
	explicit lbool(bool x) :
			value(!x) {
	}

	bool operator ==(lbool b) const {
		return (((b.value & 2) & (value & 2))
				| (!(b.value & 2) & (value == b.value))) != 0;
	}
	bool operator !=(lbool b) const {
		return !(*this == b);
	}
	lbool operator ^(bool b) const {
		return lbool((uint8_t) (value ^ (uint8_t) b));
	}

	lbool operator &&(lbool b) const {
		uint8_t sel = (this->value << 1) | (b.value << 3);
		uint8_t v = (0xF7F755F4 >> sel) & 3;
		return lbool(v);
	}

	lbool operator ||(lbool b) const {
		uint8_t sel = (this->value << 1) | (b.value << 3);
		uint8_t v = (0xFCFCF400 >> sel) & 3;
		return lbool(v);
	}

	friend int toInt(lbool l);
	friend lbool toLbool(int v);
};
inline int toInt(lbool l) {
	return l.value;
}
inline lbool toLbool(int v) {
	return lbool((uint8_t) v);
}

typedef std::vector<Lit> Cl;
typedef std::vector<Cl*> For;

inline std::ostream& operator <<(std::ostream& stream, Lit const& lit) {
	if (sign(lit)) stream << "-";
	stream << var(lit)+1;
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, lbool const& value) {
	stream << (value == l_True ? '1' : (value == l_False ? '0' : 'X'));
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, Cl const& clause) {
    for (Lit lit : clause) {
        stream << lit << " ";
    }
    return stream;
}

inline void printLiteral(Lit lit) {
    std::cout << lit;
}

inline void printLiteral(Lit lit, std::vector<lbool> values) {
    lbool value = values[var(lit)] ^ sign(lit);
	std::cout << lit << ":" << value;
}

inline void printClause(Cl clause) {
    std::cout << clause << std::endl; ;
}

inline void printDIMACS(Cl clause) {
    std::cout << clause << "0" << std::endl;
}

//=================================================================================================
// OccLists -- a class for maintaining occurence lists with lazy deletion:

template<class Idx, class Elem, class Deleted>
class OccLists {
	std::vector<std::vector<Elem>> occs;
	std::vector<char> dirty;
	std::vector<Idx> dirties;
	Deleted deleted;

public:
	OccLists() { }
	OccLists(const Deleted& d) : deleted(d) { }

	void init(const Idx& idx) {
		if ((int)size() < toInt(idx) + 1) {
			occs.resize(toInt(idx) + 1);
			dirty.resize(toInt(idx) + 1, 0);
		}
	}

	size_t size() {
	    return occs.size();
	}

	std::vector<Elem>& operator[](const Idx& idx) {
		return occs[toInt(idx)];
	}

	std::vector<Elem>& lookup(const Idx& idx) {
		clean(idx);
		return occs[toInt(idx)];
	}

	void removeAll(const Idx& idx) {
		occs[toInt(idx)].clear();
	}

	void remove(const Idx& idx, Elem elem) {
		occs[toInt(idx)].erase(std::remove(occs[toInt(idx)].begin(), occs[toInt(idx)].end(), elem), occs[toInt(idx)].end());
	}

	void smudge(const Idx& idx) {
		if (!dirty[toInt(idx)]) {
			dirty[toInt(idx)] = true;
			dirties.push_back(idx);
		}
	}

	void cleanAll() {
		for (Idx dirty : dirties) {
			clean(dirty);
		}
		dirties.clear();
	}

	void clean(const Idx& idx) {
		if (dirty[toInt(idx)]) {
			std::vector<Elem>& vec = occs[toInt(idx)];
			auto end = std::remove_if(vec.begin(), vec.end(), [this](Elem e) {return deleted(e);});
			vec.erase(end, vec.end());
			// vec.shrink_to_fit();
			dirty[toInt(idx)] = false;
		}
	}

	void clear() {
		for (auto& v : occs) {
			v.clear();
		}
		std::fill(dirty.begin(), dirty.end(), false);
		dirties.clear();
	}
};

}

// legacy
namespace Glucose {
using Var = Candy::Var;
using Lit = Candy::Lit;
}

// add std::hash template specialization
namespace std {

template<>
struct hash<Candy::Lit> {
	std::size_t operator()(const Candy::Lit& key) const {
		Candy::Var hashedVar = Candy::var(key);
		if (Candy::sign(key) == 0) {
			hashedVar = ~hashedVar;
		}
		return std::hash<Candy::Var>()(hashedVar);
	}
};

}

#endif
