/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef GATE_ANALYZER
#define GATE_ANALYZER

#include <cstdlib>
#include <algorithm>
#include <memory>

#include <vector>
#include <set>

#include <climits>
#include <chrono>

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"
#include "candy/gates/GateProblem.h"

#include "candy/utils/Runtime.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

class GateAnalyzer {

public:
    GateAnalyzer(const CNFProblem& dimacs, 
        double timeout = GateRecognitionOptions::opt_gr_timeout, 
        int tries = GateRecognitionOptions::opt_gr_tries,
        bool patterns = GateRecognitionOptions::opt_gr_patterns, 
        bool semantic = GateRecognitionOptions::opt_gr_semantic, 
        bool holistic = GateRecognitionOptions::opt_gr_holistic,
        bool lookahead = GateRecognitionOptions::opt_gr_lookahead, 
        bool intensify = GateRecognitionOptions::opt_gr_intensify, 
        int lookahead_threshold = GateRecognitionOptions::opt_gr_lookahead_threshold,
        unsigned int conflict_budget = GateRecognitionOptions::opt_gr_semantic_budget);

    ~GateAnalyzer();

    GateProblem& getResult() const {
        return gate_problem;
    }

    GateProblem& getGateProblem() const {
        return gate_problem;
    }

    const CNFProblem& getCNFProblem() const {
        return problem;
    }

    // main analysis routine
    void analyze();


    bool hasTimeout() const;

    const CNFProblem& problem;
    GateProblem& gate_problem;
    Runtime runtime;

private:
    // problem to analyze:
    CandySolverInterface* solver;

    // control structures:
    std::vector<For> index; // occurrence lists
    std::vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

    // heuristic configuration:
    int maxTries = 0;
    bool usePatterns = false;
    bool useSemantic = false;
    bool useHolistic = false;
    bool useLookahead = false;
    bool useIntensification = false;
    int lookaheadThreshold = 10;
    unsigned int semanticConflictBudget = 0;
    unsigned int assumptionCounter = 0;

    // main analysis routines
    void analyze(std::vector<Lit>& candidates);
    std::vector<Lit> analyze(std::vector<Lit>& candidates, bool pat, bool sem, bool dec);

    // clause selection heuristic
    std::vector<Lit> getRarestLiterals(std::vector<For>& index);
    std::vector<Cl> getBestRoots();

    // clause patterns of full encoding
    bool patternCheck(Lit o, For& fwd, For& bwd, std::set<Lit>& inputs);
    bool semanticCheck(Var o, For& fwd, For& bwd);

    // work in progress:
    bool isBlockedAfterVE(Lit o, For& f, For& g);

    // some helpers:
    bool isBlocked(Lit o, Cl& a, Cl& b) { // assert ~o \in a and o \in b
        for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
        return false;
    }

    bool isBlocked(Lit o, For& f, For& g) { // assert ~o \in f[i] and o \in g[j]
        for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
        return true;
    }

    bool isBlocked(Lit o, Cl* c, For& f) { // assert ~o \in c and o \in f[i]
        for (Cl* a : f) if (!isBlocked(o, *c, *a)) return false;
        return true;
    }

    bool fixedClauseSize(For& f, unsigned int n) {
        for (Cl* c : f) if (c->size() != n) return false;
        return true;
    }

    void removeFromIndex(std::vector<For>& index, Cl* clause) {
        for (Lit l : *clause) {
            For& h = index[l];
            h.erase(remove(h.begin(), h.end(), clause), h.end());
        }
    }

    void removeFromIndex(std::vector<For>& index, For& clauses) {
        For copy(clauses.begin(), clauses.end());
        for (Cl* c : copy) {
            removeFromIndex(index, c);
        }
    }

};

}
#endif
