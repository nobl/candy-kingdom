/* Copyright (c) 2017 Felix Kutzner
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#include "TestUtils.h"
#include <core/CNFProblem.h>
#include <randomsimulation/Conjectures.h>

namespace randsim {
    void assertContainsVariable(const std::unordered_set<Glucose::Var>& variables, Glucose::Var forbidden) {
        assert(variables.find(forbidden) != variables.end()); // TODO refactor
    }
    
    void assertDoesNotContainVariable(const std::unordered_set<Glucose::Var>& variables, Glucose::Var forbidden) {
        assert(variables.find(forbidden) == variables.end()); // TODO refactor
    }
    
    void deleteClauses(Candy::CNFProblem* formula) {
        
        // TODO: This is a weird workaround. CNFProblem ought to own the formula and take care of its destruction.
        
        if (formula == nullptr) {
            return;
        }
        
        for (auto clause : formula->getProblem()) {
            delete clause;
        }
        formula->getProblem().clear();
    }
    
    Candy::Cl negatedLits(const Candy::Cl& clause) {
        Candy::Cl result;
        for (auto lit : clause) {
            result.push_back(~lit);
        }
        return result;
    }
    
    void insertVariables(const std::vector<Glucose::Lit>& lits, std::unordered_set<Glucose::Var>& target) {
        for (auto lit : lits) {
            target.insert(Glucose::var(lit));
        }
    }
    
    bool hasBackboneConj(Conjectures &c, Glucose::Lit lit) {
        bool found = false;
        for (auto bb : c.getBackbones()) {
            found |= (bb.getLit() == lit);
        }
        return found;
    }
    
    bool isEquivalenceConjEq(EquivalenceConjecture &conj, std::vector<Glucose::Lit> lits) {
        if (conj.getLits().size() != lits.size()) {
            return false;
        }
        else {
            for (auto lit : lits) {
                auto begin = conj.getLits().begin();
                auto end = conj.getLits().end();
                if (std::find(begin, end, lit) == end) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    bool hasEquivalenceConj(Conjectures &c, std::vector<Glucose::Lit> lits) {
        auto invertedLits = negatedLits(lits);
        for (auto eqconj : c.getEquivalences()) {
            if (isEquivalenceConjEq(eqconj, lits) || isEquivalenceConjEq(eqconj, invertedLits)) {
                return true;
            }
        }
        
        return false;
    }

    bool containsClause(const Candy::For& formula, const Candy::Cl& clause) {
        bool found = false;
        for (auto fcl : formula) {
            found |= (*fcl == clause); // TODO: make this independent of literal positions
        }
        return found;
    }
}
