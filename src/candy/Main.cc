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

#include <errno.h>
#include <iostream>
#include <csignal>
#include <zlib.h>
#include <string>
#include <stdexcept>
#include <regex>
#include <functional>
#include <type_traits>
#include <chrono>

#if !defined(WIN32) || defined(CYGWIN)
#include <execinfo.h>
#endif

#include "candy/utils/Memory.h"
#include "candy/utils/Options.h"

#include "candy/core/CNFProblem.h"
#include "candy/core/Statistics.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/minimizer/Minimizer.h"

#include "candy/frontend/SolverFactory.h"
#include "candy/frontend/CandyBuilder.h"
#include "candy/frontend/Exceptions.h"

#include "candy/gates/GateAnalyzer.h"
#include "candy/rsar/ARSolver.h"
#include "candy/rsar/Heuristics.h"


#include "candy/systems/branching/rsil/BranchingHeuristics.h"

using namespace Candy;

static std::vector<CandySolverInterface*> solvers;

static bool interrupted = false;
static unsigned int start_time = 0;
static int interrupted_callback(void* state) {
    if (SolverOptions::time_limit > 0) {
        if (start_time == 0) {
            start_time = get_wall_time();
        }
        else if (get_wall_time() - start_time > SolverOptions::time_limit) {
            interrupted = true;
        }
    }
    if (SolverOptions::memory_limit > 0 && static_cast<int>(getCurrentRSS()/(1024*1024)) > SolverOptions::memory_limit) {
        interrupted = true;
    }
    return interrupted ? 1 : 0;
}

static void set_interrupted(int signum) {
    std::cout << "c Informing solver about desire to interrupt." << std::endl;
    interrupted = true;
}

static void just_quit(int signum) {
    std::cout << std::endl << "*** INTERRUPTED ***" << std::endl;
    _exit(1);
}

static void print_stacktrace(int signum) {
#if !defined(WIN32) || defined(CYGWIN)
    void *array[10];
    size_t size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "c Error: signal %d:\n", signum);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    std::raise(signum);
#endif
    _exit(1);
}

static void installSignalHandlers(bool handleInterruptsBySolver) {
#if defined(WIN32) && !defined(CYGWIN)
#if defined(_MSC_VER)
#pragma message ("Warning: setting signal handlers not yet implemented for Win32")
#else
#warning "setting signal handlers not yet implemented for Win32"
#endif
#else
    if (handleInterruptsBySolver) {
        signal(SIGINT, set_interrupted);
        signal(SIGXCPU, set_interrupted);
        signal(SIGSEGV, print_stacktrace);
    } else {
        signal(SIGINT, just_quit);
        signal(SIGXCPU, just_quit);
        signal(SIGSEGV, print_stacktrace);
    }
#endif
}

static void printProblemStatistics(CNFProblem& problem) {
    std::cout << "Variables: " << problem.nVars() << std::endl;
    std::cout << "Clauses: " << problem.nClauses() << std::endl;
}

static void printGateStatistics(CNFProblem& problem) {
    GateAnalyzer analyzer { problem };
    Runtime runtime;
    
    runtime.start();
    analyzer.analyze();
    runtime.stop();
    
    GateProblem gates = analyzer.getResult();

    std::cout << "c Variables: " << problem.nVars() << std::endl;
    std::cout << "c Clauses: " << problem.nClauses() << std::endl;
    std::cout << "c RemainingClauses: " << gates.remainder.size() << std::endl;
    std::cout << "c Gates: " << gates.nGates() << std::endl;
    std::cout << "c Monoton: " << gates.nMonotonGates() << std::endl;
    std::cout << "c Roots: " << gates.nRoots() << std::endl; 
    std::cout << "c StatsPatterns: " << gates.stat_patterns << std::endl; 
    std::cout << "c StatsSemantic: " << gates.stat_semantic << std::endl; 
    std::cout << "c StatsHolistic: " << gates.stat_holistic << std::endl; 

    std::cout << "c HistoConflicts: "; 
    unsigned int index = 0;
    for (unsigned int count : gates.histogram_conflicts) std::cout << " " << index++ << ":" << count;
    std::cout << std::endl;

    std::cout << "c HistoConflictsUnsuccessful: "; 
    index = 0;
    for (unsigned int count : gates.unsuccessful_histogram_conflicts) std::cout << " " << index++ << ":" << count;
    std::cout << std::endl;
    std::cout << "c Runtime: " << runtime << std::endl;
}
    
CandySolverInterface* solver = nullptr;
lbool result = l_Undef;

static void runSolverThread(lbool& result, CandySolverInterface*& solver, CNFProblem& problem, ClauseAllocator*& global_allocator) {
    std::cout << "c Sort Watches: " << SolverOptions::opt_sort_watches << std::endl;
    std::cout << "c Sort Variables: " << SolverOptions::opt_sort_variables << std::endl;
    std::cout << "c Preprocessing: " << SolverOptions::opt_preprocessing << std::endl;
    std::cout << "c Inprocessing: " << SolverOptions::opt_inprocessing << std::endl;

    CandySolverInterface* solver_ = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable);

    solver_->init(problem, global_allocator);

    if (ParallelOptions::opt_static_database && global_allocator == nullptr) {
        global_allocator = solver_->setupGlobalAllocator();
    }

    solvers.push_back(solver_);
    solver_->setTermCallback(solver_, interrupted_callback);

    lbool result_ = solver_->solve();

    if (result == l_Undef) {
        result = result_;
        solver = solver_;
    }
}

int main(int argc, char** argv) {
    setUsageHelp("c USAGE: %s [options] <input-file>\n\nc where input may be either in plain or gzipped DIMACS.\n");
    parseOptions(argc, argv, true);
    
    if (SolverOptions::verb > 1) {
        std::cout << "c Candy 0.7 is made of Glucose (Many thanks to the Glucose and MiniSAT teams)" << std::endl;
    }

    CNFProblem problem{};
    try {
        if (argc == 1) {
            std::cout << "c Reading from standard input ... " << std::endl; 
            problem.readDimacsFromStdin();
        } else {
            std::cout << "c Reading file: " << argv[1] << std::endl; 
            const char* inputFilename = argv[1];
            problem.readDimacsFromFile(inputFilename);
        }
    }
    catch (ParserException& e) {
		printf("c Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    if (SolverOptions::gate_stats) {
        printGateStatistics(problem);
        return 0;
    }

    if (SolverOptions::verb == 1) {
        printProblemStatistics(problem);
    }

    ClauseAllocator* global_allocator = nullptr;
    std::vector<std::thread> threads;

    if (ParallelOptions::opt_threads == 1) {
        if (RSAROptions::opt_rsar_enable) {
            solver = createRSARSolver(problem);
        }
        else {
            solver = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable, RSILOptions::opt_rsil_advice_size);
        }
        solvers.push_back(solver); 

        solver->init(problem);
        solver->setTermCallback(solver, interrupted_callback);

        installSignalHandlers(true);
        result = solver->solve();
        installSignalHandlers(false);
    }
    else {
        if (ParallelOptions::opt_static_database) {
            std::cout << "Parallel mode with static database is disabled in this version due to a bug. If you want to test it, please checkout the branch 'release_sat_2019'." << std::endl;
            exit(0);
        }
        for (unsigned int count = 0; count < (unsigned int)ParallelOptions::opt_threads && result == l_Undef; count++) {
            std::cout << "c Initializing Solver " << count << std::endl;
            ClauseDatabaseOptions::opt_recalculate_lbd = false;
            SolverOptions::opt_sort_watches = ((count % 2) == 0);
            SolverOptions::opt_preprocessing = (count == 0);
            SolverOptions::opt_inprocessing = count + SolverOptions::opt_inprocessing;
            VariableEliminationOptions::opt_use_elim = !ParallelOptions::opt_static_database;
            VariableEliminationOptions::opt_use_asymm = (count == 6) || (count == 7) || (count == 14) || (count == 15);
            switch (count) {
                case 0 : case 1 : //vsids
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 2 : case 3 : //lrb
                    SolverOptions::opt_use_lrb = true;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 4 : case 5 : //rsil
                    SolverOptions::opt_vsids_var_decay = 0.75;
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = true;
                    break;
                case 6 : case 7 : //vsids
                    SolverOptions::opt_vsids_var_decay = 0.7;
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 8 : case 9 : //vsids
                    SolverOptions::opt_vsids_var_decay = 0.6;
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 10 : case 11 : //lrb
                    SolverOptions::opt_lrb_step_size = 0.7;
                    SolverOptions::opt_lrb_min_step_size = 0.02;
                    SolverOptions::opt_use_lrb = true;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 12 : case 13 : //rsil
                    GateRecognitionOptions::method = static_cast<int>(GateRecognitionMethod::IntensifyPS); 
                    RandomSimulationOptions::opt_rs_nrounds = 1048576 * 2;
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = true;
                    break;
                case 14 : case 15 : //vsids
                    SolverOptions::opt_vsids_var_decay = 0.5;
                    SolverOptions::opt_vsids_max_var_decay = 0.99;
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
            }
            
            threads.push_back(std::thread(runSolverThread, std::ref(result), std::ref(solver), std::ref(problem), std::ref(global_allocator)));

            do { 
                // wait till solver is initialized (and wait a bit longer)
                std::this_thread::sleep_for(std::chrono::milliseconds(ParallelOptions::opt_thread_initialization_delay)); 
            } while (solvers.size() <= count);
        }

        installSignalHandlers(true);
        // problem.clear();
        while (result == l_Undef && !interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        installSignalHandlers(false);
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    if (solver != nullptr) {
        CandySolverResult& model = solver->getCandySolverResult();

        if (result == l_True && MinimizerOptions::do_minimize) {
            Minimizer minimizer(problem, model);
            Runtime runtime;
            std::cout << "c Minimizing Model" << std::endl;
            runtime.start();
            minimizer.mimimizeModel(MinimizerOptions::minimize_pruned, MinimizerOptions::minimize_minimal, MinimizerOptions::minimize_project);
            runtime.stop();
            std::cout << "c Minimized-Model-Size: " << model.getMinimizedModelLiterals().size() << std::endl; 
            std::cout << "c Minimizer-Runtime: " << std::fixed << std::setprecision(2) << runtime.getRuntime() << std::endl;
        }
        if (result == l_True && SolverOptions::mod) {
            if (MinimizerOptions::do_minimize) {
                std::cout << "c Minimized-Model: " << model.getMinimizedModelLiterals() << std::endl;
            } 
            else {
                // problem.checkResult(result);
                std::cout << "v";
                for (Var v = 0; v < (Var)problem.nVars(); v++) {
                    std::cout << " " << model.value(v);
                }
                std::cout << " 0" << std::endl;
            }
        }

        if (SolverOptions::verb > 0) {
            solver->getStatistics().printFinalStats();
        }

        solver->getStatistics().printRuntimes();
        std::cout << "c Peak Memory (MB): " << getPeakRSS()/(1024*1024) << std::endl;
    }

    #ifndef __SANITIZE_ADDRESS__
        exit((result == l_True ? 10 : result == l_False ? 20 : 0));
    #endif
    return (result == l_True ? 10 : result == l_False ? 20 : 0);
}

