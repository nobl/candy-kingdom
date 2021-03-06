/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
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

#include <gtest/gtest.h>
#include <candy/core/CandySolverInterface.h>
#include <candy/utils/FastRand.h>
#include <candy/utils/MemUtils.h>
#include <candy/rsar/ApproximationState.h>
#include <candy/frontend/CandyBuilder.h>

#include "TestUtils.h"
#include "HeuristicsMock.h"

#include <iostream>

namespace Candy {
    
    EquivalencyChecker::EquivalencyChecker() {
    }
    
    EquivalencyChecker::~EquivalencyChecker() {
    }
    
    class EquivalencyCheckerImpl : public EquivalencyChecker {
    public:
        EquivalencyCheckerImpl();
        virtual ~EquivalencyCheckerImpl();
        
        void addClauses(const std::vector<Cl>& clauses) override;
        bool isEquivalent(const std::vector<Lit>& assumptions, Lit a, Lit b) override;
        bool isAllEquivalent(const std::vector<Lit>& assumptions, const std::vector<Lit>& equivalentLits) override;
        bool isBackbones(const std::vector<Lit>& assumptions, const std::vector<Lit>& backboneLits) override;

        Var createVariable() {
            return m_maxVar++;
        }
        
    private:
        bool solve();

        void setAssumptions(const std::vector<Lit>& assumptions) {
            m_solver->getAssignment().setAssumptions(assumptions);
        }
        
        CandySolverInterface* m_solver;
        Var m_maxVar;
    };

    
    
    EquivalencyCheckerImpl::EquivalencyCheckerImpl()
    : EquivalencyChecker(), m_maxVar(0) {
        SolverOptions::opt_preprocessing = false;
        m_solver = createSolver();
    }
    
    EquivalencyCheckerImpl::~EquivalencyCheckerImpl() {
        delete m_solver;
    }
    
    void EquivalencyCheckerImpl::addClauses(const std::vector<Cl>& clauses) {
        CNFProblem problem;
        for (const Cl& clause : clauses) {
            problem.readClause((Cl&)clause);
        }
        m_solver->init(problem);
        m_maxVar = problem.nVars();
    }
    
    bool EquivalencyCheckerImpl::isEquivalent(const std::vector<Lit>& assumptions, Lit a, Lit b) {
        std::vector<Lit> extendedAssumptions {assumptions};
        
        Lit assumption1 = Lit(createVariable(), 1);
        Lit assumption2 = Lit(createVariable(), 1);
        
        addClauses({Cl{assumption1, a}, Cl{assumption1, ~b}, Cl{assumption2, ~a}, Cl{assumption2, b}});
        
        extendedAssumptions.push_back(~assumption1);
        extendedAssumptions.push_back(assumption2);
        
        setAssumptions(extendedAssumptions);
        if (!solve()) {
            extendedAssumptions[extendedAssumptions.size()-2] = assumption1;
            extendedAssumptions[extendedAssumptions.size()-1] = ~assumption2;
            setAssumptions(extendedAssumptions);
            return !solve();
        }
        else {
            return false;
        }
    }
    
    bool EquivalencyCheckerImpl::isAllEquivalent(const std::vector<Lit>& assumptions, const std::vector<Lit>& equivalentLits) {
        assert(equivalentLits.size() > 1);
        
        bool allEquiv = true;
        for (size_t i = 0; i < equivalentLits.size()-1 && allEquiv; ++i) {
            assert(equivalentLits[i].var() <= m_maxVar && equivalentLits[i+1].var() <= m_maxVar);
            allEquiv &= isEquivalent(assumptions, equivalentLits[i], equivalentLits[i+1]);
        }
        
        return allEquiv;
    }
    
    bool EquivalencyCheckerImpl::isBackbones(const std::vector<Lit>& assumptions, const std::vector<Lit>& backboneLits) {
        bool allBackbone = true;
        for (auto lit : backboneLits) {
            Lit assumption = Lit(createVariable(), 1);
            addClauses({Cl{~lit, assumption}});
            std::vector<Lit> extendedAssumptions {assumptions};
            extendedAssumptions.push_back(~assumption);
            setAssumptions(extendedAssumptions);
            allBackbone &= !solve();
        }
        return allBackbone;
    }
    
    bool EquivalencyCheckerImpl::solve() {
        return l_True == m_solver->solve();
    }
    
    std::unique_ptr<EquivalencyChecker> createEquivalencyChecker() {
        return backported_std::make_unique<EquivalencyCheckerImpl>();
    }
    
    
    TEST(RSARTestUtils, EquivalencyChecker_checkEquivalences) {
        auto checker = createEquivalencyChecker();

        checker->addClauses({{Lit(0, 0), Lit(1, 1)},
            {Lit(1, 0), Lit(0, 1)},
            {Lit(2, 0), Lit(3, 1)}});
        
        EXPECT_FALSE(checker->isEquivalent({}, Lit(2, 1), Lit(3,1)));
        EXPECT_TRUE(checker->isEquivalent({}, Lit(0, 1), Lit(1,1)));
        EXPECT_FALSE(checker->isEquivalent({}, Lit(0, 0), Lit(1,1)));
    }
    
    TEST(RSARTestUtils, EquivalencyChecker_checkBackbones) {
        auto checker = createEquivalencyChecker();

        checker->addClauses({{Lit(0, 0), Lit(1, 1)},
            {Lit(3, 0), Lit(0, 1)},
            {Lit(2, 0), Lit(3, 1)}});
        
        
        EXPECT_TRUE(checker->isBackbones({Lit(3, 1)}, {Lit(0, 1)}));
        EXPECT_FALSE(checker->isBackbones({Lit(3, 1)}, {Lit(2, 0)}));
    }
    
    
    bool occursOnlyBefore(const std::vector<SolverMockEvent> &events,
                          SolverMockEvent first,
                          SolverMockEvent second) {
        if (first == second) {
            return false;
        }
        
        bool foundFirst = false;
        bool foundSecond = false;
        
        for (auto event : events) {
            if ((event == first) && foundSecond) {
                return false;
            }
            
            foundFirst |= (event == first);
            foundSecond |= (event == second);
        }
        
        return foundFirst && foundSecond;
    }
    
    bool occursOnlyAfter(const std::vector<SolverMockEvent> &events,
                         SolverMockEvent first,
                         SolverMockEvent second) {
        return occursOnlyBefore(events, second, first);
    }
    
    bool occursBefore(const std::vector<SolverMockEvent> &events,
                      SolverMockEvent first,
                      SolverMockEvent second) {
        return !occursOnlyAfter(events, first, second);
    }
    
    bool occursAfter(const std::vector<SolverMockEvent> &events,
                     SolverMockEvent first,
                     SolverMockEvent second) {
        return !occursOnlyBefore(events, first, second);
    }
    
    bool contains(const Cl& clause, Lit lit) {
        return std::find(clause.begin(), clause.end(), lit) != clause.end();
    }
    
    EquivalenceConjecture createEquivalenceConjecture(const std::vector<Lit> &lits) {
        EquivalenceConjecture result;
        for (auto lit : lits) {
            result.addLit(lit);
        }
        return result;
    }
    
    bool varOccursIn(const std::vector<Cl> &clauses, Var v) {
        bool result = false;
        for (auto&& clause : clauses) {
            result |= (std::find_if(clause.begin(), clause.end(), [v](Lit l) {
                return l.var() == v;
            }) != clause.end());
        }
        return result;
    }
    
    
    std::vector<Lit> pickLiterals(const CNFProblem &f, int n) {
        std::set<Var> chosenVars;
        std::vector<Lit> result;
        
        if (f.nVars() == 0) {
            return {};
        }
        
        fastnextrand_state_t randState = static_cast<fastnextrand_state_t>(f.nVars());
        
        for (int i = 0; i < n; ++i) {
            randState = fastNextRand(randState);
            auto var = randState % f.nVars();
            auto sign = randState % 2;
            
            if (chosenVars.find(var) == chosenVars.end()) {
                result.push_back(Lit(var, sign));
                chosenVars.insert(var);
            }
        }
        
        return result;
    }
    
    std::priority_queue<std::vector<Lit>::size_type> getRandomIndices(const std::vector<Lit> &literals,
                                                                      unsigned int seed) {
        fastnextrand_state_t randState = static_cast<fastnextrand_state_t>(seed);
        std::priority_queue<std::vector<Lit>::size_type> indices;
        
        indices.emplace(0);
        
        for (int i = 0; i < 3; ++i) {
            randState = fastNextRand(randState);
            auto index = randState % literals.size();
            indices.emplace(index);
        }
        
        return indices;
    }
    
    std::vector<std::vector<Lit>> convertToPartition(std::vector<Lit> &literals,
                                                     std::priority_queue<std::vector<Lit>::size_type> &indices) {
        std::vector<std::vector<Lit>> result;
        
        while (!indices.empty()) {
            std::vector<Lit> entry;
            auto currentMinIdx = indices.top();
            indices.pop();
            while (literals.size() > currentMinIdx) {
                entry.push_back(literals.back());
                literals.pop_back();
            }
            result.push_back(entry);
        }
        
        return result;
    }
    
    std::unique_ptr<Conjectures> createRandomConjectures(const std::vector<Lit> &literals,
                                                         const CNFProblem& problem) {
        if (literals.empty()) {
            return std::unique_ptr<Conjectures>(new Conjectures{});
        }
        
        std::vector<Lit> literalsWorkingCpy = literals;
        
        auto indices = getRandomIndices(literals, problem.nVars());
        auto result = std::unique_ptr<Conjectures>(new Conjectures{});
        
        auto backboneLit = literalsWorkingCpy.back();
        literalsWorkingCpy.pop_back();
        
        result->addBackbone(BackboneConjecture{backboneLit});
        
        auto partitionRest = convertToPartition(literalsWorkingCpy, indices);
        for (auto& x : partitionRest) {
            if (x.size() > 1) {
                EquivalenceConjecture conj;
                for (auto lit : x) {
                    conj.addLit(lit);
                }
                result->addEquivalence(conj);
            }
        }
        
        return result;
    }
    
    std::unique_ptr<RefinementHeuristic> createRandomHeuristic(const std::vector<Lit> &literals) {
        auto fakeHeur = std::unique_ptr<FakeHeuristic>(new FakeHeuristic());
        
        auto indices = getRandomIndices(literals, 3ull);
        std::vector<Lit> literalsWorkingCopy = literals;
        
        auto partition = convertToPartition(literalsWorkingCopy, indices);
        
        for (size_t i = 0; i < partition.size(); ++i) {
            std::vector<Var> partitionVars;
            partitionVars.resize(partition[i].size());
            
            std::transform(partition[i].begin(),
                           partition[i].end(),
                           partitionVars.begin(),
                           [](Lit lit) { return lit.var(); });
            
            fakeHeur->inStepNRemove(i, partitionVars);
        }
        
        return std::move(fakeHeur);
    }

}
