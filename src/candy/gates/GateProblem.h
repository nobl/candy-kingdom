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

#ifndef GATE_PROBLEM
#define GATE_PROBLEM

#include <cstdlib>
#include <algorithm>
#include <vector>
#include <set>

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

namespace Candy {
    
struct Gate {
    Lit out = lit_Undef;
    For fwd, bwd;
    bool notMono = false;
    std::vector<Lit> inp;

    inline bool isDefined() const { return out != lit_Undef; }
    inline Lit getOutput() const { return out; }

    
    inline const For& getForwardClauses() const { return fwd; }
    inline const For& getBackwardClauses() const { return bwd; }
    inline const std::vector<Lit>& getInputs() const  { return inp; }
    
    inline For& getForwardClauses() {
        return const_cast<For&>(static_cast<const Gate*>(this)->getForwardClauses());
    }

    inline For& getBackwardClauses() {
        return const_cast<For&>(static_cast<const Gate*>(this)->getBackwardClauses());
    }

    inline std::vector<Lit>& getInputs() {
        return const_cast<std::vector<Lit>&>(static_cast<const Gate*>(this)->getInputs());
    }
    
    inline bool hasNonMonotonousParent() const { return notMono; }
};


class GateProblem {
public:
    const CNFProblem& problem;

    // analyzer output:
    std::vector<Cl> roots; // top-level clauses
    std::vector<Gate> gates; // stores gate-struct for every output
    unsigned int gate_count = 0;
    Cl artificialRoot; // top-level unit-clause that can be generated by normalizeRoots()

    GateProblem(const CNFProblem& p) : 
        problem(p), roots(), gates(), gate_count(0), artificialRoot() 
    {
        gates.resize(problem.nVars());
    }

    ~GateProblem() {
    }

    void addGate(Lit o, For& fwd, For& bwd, std::set<Lit>& inp, bool notMono) {
        gate_count++;
        gates[o.var()].out = o;
        gates[o.var()].notMono = notMono;
        gates[o.var()].fwd.insert(gates[o.var()].fwd.end(), fwd.begin(), fwd.end());
        gates[o.var()].bwd.insert(gates[o.var()].bwd.end(), bwd.begin(), bwd.end());
        gates[o.var()].inp.insert(gates[o.var()].inp.end(), inp.begin(), inp.end());
    }

    // public getters
    int getGateCount() const { 
        return gate_count; 
    }
    
    Gate& getGate(Lit output) { 
        return gates[output.var()]; 
    }

    typedef std::vector<Gate>::const_iterator const_iterator;
    
    inline const_iterator begin() const {
        return gates.begin();
    }

    inline const_iterator end() const {
        return gates.end();
    }

    inline const Gate& operator [](Var var) const {
        return gates[var];
    }

    inline size_t nVars() const {
        return problem.nVars();
    }

    inline size_t nClauses() const {
        return problem.nClauses();
    }

    inline size_t nGates() const {
        return gate_count;
    }

    const std::vector<Cl>& getRoots() const {
        return roots;
    }

    // create unique list of root literals base on root clauses
    std::vector<Lit> getRootLiterals() {
        std::vector<Lit> literals;

        for (const Cl& c : getRoots()) {
            literals.insert(literals.end(), c.begin(), c.end());
        }
        std::sort(literals.begin(), literals.end());
        auto last = std::unique(literals.begin(), literals.end());
        literals.erase(last, literals.end());

        return literals;
    }

    /**
     * @brief GateAnalyzer::getPrunedProblem
     * @param model
     * @return clauses of all satisfied branches
     */
    Formula getPrunedProblem(Cl model) {
        Formula result(roots.begin(), roots.end());

        std::vector<Lit> literals = getRootLiterals();
        std::vector<int> visited(model.size(), -1);

        while (literals.size() > 0) {
            Lit o = literals.back();
            literals.pop_back();

            if (model[o.var()] == o && visited[o.var()] != o) {
                Gate gate = gates[o.var()];
                for (Cl* clause : gate.fwd) result.push_back(*clause);
                for (Cl* clause : gate.bwd) result.push_back(*clause);
                literals.insert(literals.end(), gate.inp.begin(), gate.inp.end());
                visited[o.var()] = o;
            }
        }

        return result;
    }

    bool hasArtificialRoot() const {
        return artificialRoot.size() > 0;
    }

    Cl getArtificialRoot() const {
        return artificialRoot;
    }

    void printGates() {
        std::vector<Lit> outputs;
        std::vector<bool> done(problem.nVars());
        for (Cl& root : roots) {
            outputs.insert(outputs.end(), root.begin(), root.end());
        }
        for (size_t i = 0; i < outputs.size(); ++i) {
            Gate& gate = getGate(outputs[i]);

            if (gate.isDefined() && !done[outputs[i].var()]) {
                done[outputs[i].var()] = true;
                printf("Gate with output ");
                printLiteral(gate.getOutput());
                printf("Is defined by clauses ");
                printFormula(gate.getForwardClauses());
                printFormula(gate.getBackwardClauses());
                outputs.insert(outputs.end(), gate.getInputs().begin(), gate.getInputs().end());
            }
        }
    }

    Lit getRoot() const {
        assert(roots.size() == 1 && roots.front().size() == 1);
        return roots.front().front(); 
    }

    /**
     * Execute after analysis in order to
     * tronsform many roots to one big and gate with one output
     * Side-effect: introduces a fresh variable
     */
    void normalizeRoots() {
        Var root = problem.nVars();
        // grow data-structures
        gates.resize(problem.nVars() + 1);
        // inputs.resize(2 * problem.nVars() + 2, false);
        // create gate
        gate_count++;
        gates[root].out = Lit(root, false);
        gates[root].notMono = false;
        std::set<Lit> inp;
        for (Cl& c : roots) {
            inp.insert(c.begin(), c.end());
            c.push_back(Lit(root, true));
            gates[root].fwd.push_back(new Cl(c));
        }
        gates[root].inp.insert(gates[root].inp.end(), inp.begin(), inp.end());
        this->roots.clear();
        artificialRoot.push_back(gates[root].out);
        this->roots.push_back(artificialRoot);
    }

};

}

#endif