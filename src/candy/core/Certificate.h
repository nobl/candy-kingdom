/*
 * Certificate.h
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CERTIFICATE_H_
#define SRC_CANDY_CORE_CERTIFICATE_H_

#include <vector>
#include <cstring>
#include <fstream>
#include <memory>
#include "candy/core/SolverTypes.h"
#include "candy/core/Clause.h"
#include "candy/core/SolverTypes.h"

namespace Candy {

class Clause;

class Certificate {
private:
    bool active;
    std::unique_ptr<std::ofstream> out;

    template<typename Iterator>
    void printLiterals(Iterator it, Iterator end) {
        for(; it != end; it++) {
            *out << (var(*it) + 1) * (sign(*it) ? -1 : 1) << " ";
        }
        *out << "0\n";
    }

public:
    Certificate(const char* _out);
    ~Certificate();

    bool isActive() {
        return active;
    }

    void reset(const char* target);

    void proof() {
        if (active) {
            *out << "0" << std::endl;
        }
    }

    void added(std::initializer_list<Lit> list) {
        added(list.begin(), list.end());
    }

    void removed(std::initializer_list<Lit> list) {
        removed(list.begin(), list.end());
    }

    template<typename Iterator>
    void added(Iterator it, Iterator end) {
        if (active) {
            printLiterals(it, end);
        }
    }

    template<typename Iterator>
    void removed(Iterator it, Iterator end) {
        if (active) {
            *out << "d ";
            printLiterals(it, end);
        }
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CERTIFICATE_H_ */
