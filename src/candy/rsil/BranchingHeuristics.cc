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

#include "BranchingHeuristics.h"
#include <candy/core/Trail.h>

#include <algorithm>

namespace Candy {

    template <>
    RSILBranchingHeuristic3::Parameters RSILBranchingHeuristic3::defaultParameters = RSILBranchingHeuristic3::Parameters();

    template <>
    Lit RSILBranchingHeuristic3::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
    
    template <>
    Lit RSILVanishingBranchingHeuristic3::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
    
    template <>
    Lit RSILBudgetBranchingHeuristic3::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
    
    template <>
    Lit RSILBranchingHeuristic2::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
    
    template <>
    Lit RSILVanishingBranchingHeuristic2::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
    
    template <>
    Lit RSILBudgetBranchingHeuristic2::pickBranchLit(Trail& trail) {
        Lit rsilAdvice = getAdvice(trail.trail, trail.trail_size, trail.trail_lim, trail.assigns, defaultBranchingHeuristic.decision);
        if (rsilAdvice != lit_Undef) {
            return rsilAdvice;
        }
        
        Lit candidate = defaultBranchingHeuristic.pickBranchLit(trail);
        
        if (rsilAdvice != lit_Undef || candidate == lit_Undef) {
            return candidate;
        }
        return getSignAdvice(candidate);
    }
}
