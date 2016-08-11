#ifndef MGA
#define MGA

#include <cstdlib>
#include <algorithm>

#include <vector>
#include <set>

#include <climits>

#include "../core/CNFProblem.h"
using namespace Glucose;

using namespace std;

#include "core/SolverTypes.h"
#include "core/Utilities.h"
#include "core/Solver.h"

class GateAnalyzer {

public:
  GateAnalyzer(CNFProblem& dimacs);

  // main analysis routines:
  void analyze();
  void analyze(set<Lit>& roots);

  // public getters:
  int getNGates() { return nGates; }
  For* getGates() { return gates; }


  void enumerateBlockedPairs() {
    for (Cl* c : problem.getProblem())
      if (c->size() == 1) roots.push_back(c);
      else for (Lit l : *c) index[l].push_back(c);
    int n = problem.nClauses() * 100;
    bool* blocks = (bool*)calloc(n, sizeof(bool));
    for (int v = 0; v < problem.nVars(); v++) {
      Lit l = mkLit(v, false);
      for (Cl* c1 : index[l]) {
        for (Cl* c2 : index[~l]) {
          int addr = ((long)c1 + (long)c2) % n;
          blocks[addr] = blocks[addr] || isBlocked(l, *c1, *c2);
        }
      }
    }
    int c = 0;
    for (int i = 0; i < n; i++) {
      if (blocks[i]) c++;
    }
    printf("%i of %i bits are set (%i * %i pairs)", c, n, problem.nClauses(), problem.nClauses());
  }

private:
  // problem to analyze:
  CNFProblem problem;
  Solver solver;

  // control structures:
  vector<Cl*> roots;
  vector<For> index; // occurrence lists
  vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

  // heuristic configuration:
  int maxTries = 0;
  bool usePatterns = false;
  bool useSemantic = false;
  bool useHolistic = false;
  bool useDecomposition = false;

  // statistics:
  int nGates = 0;
  For* gates;

  // debugging
  bool verbose = false;

  // clause selection heuristic
  vector<Cl*>& selectClauses();

  // clause patterns of full encoding
  bool fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs);
  bool semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o);
  bool isFullGate(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs, Lit output);

  // some helpers:
  bool isBlocked(Lit o, Cl& a, Cl& b) {
    for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
    return false;
  }

  bool isBlocked(Lit o, For& f, For& g) {
    for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
    return true;
  }

  bool isBlocked(Lit o, Cl& c, For& f) {
    for (Cl* a : f) if (!isBlocked(o, c, *a)) return false;
    return true;
  }

  void saturate(For& source, For& target, For& blocked, Lit o) {
    for (unsigned int i = 0; i < source.size(); i++) {
      int n = source.size();
      if (isBlocked(o, *source[i], blocked)) {
        target.push_back(source[i]);
        std::swap(source[i], source[n-1]);
        source.pop_back();
      }
    }
  }

  bool isBlockedGreedyDecompose(Lit o, For f, For g) {
    vector<For> left, right;
    if (verbose) {
      printf("Decomposition with Output-literal %s%i\n", sign(o)?"-":"", var(o)+1);
    }
    while (!f.empty()) {
      left.push_back(For()); right.push_back(For());
      Cl* next = f.back(); f.pop_back();
      left.back().push_back(next);
      saturate(g, right.back(), left.back(), o);
      saturate(f, left.back(), right.back(), o);
    }
    if (verbose) {
      printf("Decomposition into:");
      for (int i = 0; i < left.size(); i++) {
        printf("Block %i\n", i);
        printClauses(left[i]);
        printf("----------\n");
        printClauses(right[i]);
        printf("----------\n----------\n");
      }
    }
    return g.empty();
  }

  bool fixedClauseSize(For& f, unsigned int n) {
    for (Cl* c : f) if (c->size() != n) return false;
    return true;
  }

  void removeFromIndex(vector<Cl*> clauses) {
    for (Cl* c : clauses) for (Lit l : *c) {
      For& h = index[l];
      h.erase(remove(h.begin(), h.end(), c), h.end());
    }
  }

};

#endif