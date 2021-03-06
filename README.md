|        Linux        |       Windows       |       Coverage       |     Metrics      |    Sonarcloud     |
|:-------------------:|:-------------------:|:--------------------:|:----------------:|:-----------------:|
| [![travisCI][1]][2] | [![appveyor][3]][4] | [![coveralls][5]][6] | [![tokei][7]][8] | [![sonar][9]][10] |

[1]: https://travis-ci.org/Udopia/candy-kingdom.svg?branch=master
[2]: https://travis-ci.org/Udopia/candy-kingdom
[3]: https://ci.appveyor.com/api/projects/status/s9w7la4p8pdi5cja?svg=true
[4]: https://ci.appveyor.com/project/Udopia/candy-kingdom/branch/master
[5]: https://coveralls.io/repos/github/Udopia/candy-kingdom/badge.svg?branch=master
[6]: https://coveralls.io/github/Udopia/candy-kingdom?branch=master
[7]: https://tokei.rs/b1/github/udopia/candy-kingdom?category=code
[8]: https://github.com/Aaronepower/tokei#badges
[9]: https://sonarcloud.io/api/project_badges/measure?project=candy&metric=alert_status
[10]: https://sonarcloud.io/dashboard?id=candy

# Candy Kingdom

**Candy Kingdom** is a modular collection of SAT solvers and tools for structure analysis in SAT problems. The original **Candy** solver is a branch of the famous SAT solver **[Glucose](http://www.labri.fr/perso/lsimon/glucose/)**. Several new approaches (e.g. rsar and rsil-variants) in Candy focus on explicit exploitation of structure analysis in SAT solving. The core of Glucose has been completely reworked with a strong focus on the *independence* and *exchangeability* of components in the core solver, while increasing the *readability* and *maintainability* of the code. The allocation model of clauses was revisited with a focus on cache efficient *memory management*. A new *sonification* module provides *[Ear Candy](https://www.youtube.com/watch?v=iupgZGlzMCQ)*, you can now also listen to solver runs. 

## Build

Candy uses the [googletest](https://github.com/google/googletest) API, which is why, in order to build Candy you need to initialize and build the googletest submodule like this:
```bash
git submodule init
git submodule update
cd lib/googletest
cmake .
cmake --build .
```

Then you can build Candy like this:
```bash
mkdir release
cd release
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target candy
```

In order to build and execute unit-tests execute:
```bash
cmake --build .
ctest
```

## Run

In order to run candy and solve a problem `example.cnf` invoke:
```bash
candy example.cnf
```

Candy offers a multitude of options, like paramaters to tune heuristics and thresholds, or parameters to select an alternative solving strategy (e.g. *rsar* or *rsil*). If you have any questions feel free to contact [me](mailto:2.markus.iser@gmail.com).

## Credits

Credits go to Robin Freyler and Felix Kutzner for a multitude of discussions about SAT solving and structured programming, 
we were SAT CLIQUE. The main ideas for the profound structural changes in Candy have been developed in that time. 

Credits go to all the people at ITI and the students.


