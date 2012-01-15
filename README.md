idock
=====

idock is a multithreaded [virtual screening] tool for flexible ligand [docking] for computational drug discovery. It is inspired by [AutoDock Vina], and is hosted by GitHub at https://github.com/HongjianLi/idock under [Apache License 2.0].


Features
--------

* idock invents its own thread pool in order to reuse threads and maintain a high CPU utilization throughout the entire screening procedure. The thread pool parallelizes the precalculation of scoring function, the creation of grid maps, and the execution of Monte Carlo tasks.
* idock estimates the capacity of every vector structure and intensively utilizes Rvalue reference, a new feature in the [C++11] standard, to avoid frequent memory reallocation.
* idock flattens Vina's tree-like recursive data structure of ligand into simple linear array structure to ensure a high data cache hit rate and easy coding.
* idock accelerates the assignment of atom types by making use of residue information for receptor and branch information for ligand.
* idock can be used as a backend docking engine for [igrow], a multithreaded ligand growing tool for structure-based molecule design.


Supported operating systems and compilers
-----------------------------------------

* Ubuntu 11.10 x86_64 and GCC 4.6.1
* Ubuntu 11.10 x86_64 and CLANG 2.9
* Ubuntu 11.10 x86_64 and Intel C++ Compiler 12.0.5.220
* Arch Linux 3.1.6 x86_64 and GCC 4.6.2
* Arch Linux 3.1.6 x86_64 and CLANG 3.0
* Oracle Solaris 11 11/11 and GCC 4.5.2
* Windows 7 SP1 x64 and Windows SDK 7.1
* Windows 7 SP1 x64 and Visual Studio 2010
* Windows 7 SP1 x64 and Intel C++ Compiler XE 12.1.1.258


Unsupported operating systems and compilers
-------------------------------------------

* Windows 8 Developer Preview and Visual Studio 11 Developer Preview
* Solaris 11 Express 2010.11 x86_64 and GCC 3.4.3
* Mac OS X Lion 10.7.0 and Xcode 4.2
* Mac OS X Lion 10.7.0 and GCC 4.6.1
* Mac OS X Lion 10.7.0 and CLANG 3.0


Compilation
-----------

idock depends on [Boost C++ Libraries]. Boost 1.46.0, 1.46.1, 1.47.0, and 1.48.0 are tested. The Boost libraries required by idock are `System`, `Thread`, `Filesystem`, and `Program Options`.

### Compilation on Linux

The Makefile uses GCC as the default compiler. To compile, simply run

    make -j

CLANG is also supported.

    make -j TOOLSET=clang

Intel C++ Compiler is also supported.

    make -j TOOLSET=intel-linux

One may modify the Makefile to use a different compiler or different compilation options.

The generated objects will be placed in the `obj` folder, and the generated executable will be placed in the `bin` folder.

### Compilation on Windows

Visual Studio 2010 solution and project files are provided in the `msvc` folder. The project file uses Windows 7.1 SDK as platform toolset by default. One may revert it to vc100. To compile, simply run

    msbuild /t:Build /p:Configuration=Release /m

Or one may open `idock.sln` in Visual Studio 2010 and do a full rebuild.

The generated objects will be placed in the `obj` folder, and the generated executable will be placed in the `bin` folder.


Usage
-----

First add idock to your PATH environment variable.

To display a full list of available options, simply run the program without arguments

    idock

The `examples` folder contains several use cases. For example, to dock the ligand TMC278 against HIV-1 RT of PDB ID 2ZD1,

    cd examples/2ZD1/T27

One can supply the options from command line arguments

    idock --receptor ../../../receptors/2ZD1.pdbqt --ligand_folder ../../../ligands/T27 --output_folder output --center_x 49.712 --center_y -28.923 --center_z 36.824 --size_x 18 --size_y 18 --size_z 20

Or one can instruct idock to load the options from a configuration file

    idock --config idock.cfg

For comparison against [AutoDock Vina]

    vina --config vina.cfg


Documentation Creation
----------------------

Documentations in both HTML and LaTeX formats can be esaily created by running [doxygen]

    doxygen doxygen

The created documents will be placed in `doc` folder. To compile LaTeX files into PDF, one must have `pdflatex` installed.

    cd doc/latex
    make

The generated PDF will be `refman.pdf`.


Change Log
----------

### 1.0 (2011-07-20)

* Initial release at [CodePlex].

### 1.1 (2011-12-20)

* Changed the version control system from TFS to Git.
* Project migrated from CodePlex to GitHub.
* Tested Solaris 11, clang 3.0, and Intel C++ Compiler v11.
* Provided Visual C++ solution, project and bat files to ease recompilation on Windows.
* Added precompiled executables for both 32-bit and 64-bit Linux and Windows.
* Added program option `config` to allow users to specify a configuration file.
* Added thread-safe progress bar.
* Output predicted free energy of the top 5 conformations.
* Reverted the evaluation of intra-molecular free energy to Vina's implementation to obtain better RMSD for certain cases.

### 1.2 (2012-01-20)

* Added program option --csv for dumping docking summary sorted in the ascending of predicted free energy.
* Profiled by the Valgrind tool suite to ensure zero memory leak.
* Replaced a switch statement by table lookup to decrease indirect branch misprediction rate.
* Added move constructors for several classes to boost performance.
* Revised the precision of coordinates to be 3 digits.
* Revised the precision of free energy to be 2 digits.
* Parallelized the precalculation of scoring function.
* Fixed a numerical bug when docking a ligand of only one single heavy atom.
* Added support for docking ligands created by igrow.

Contact Author
--------------

[Jacky Lee]


Logo
----

![idock logo](https://github.com/HongjianLi/idock/raw/master/logo.png)

Green grape is chosen as the logo for idock because it is one of the author's favorite fruit. The logo image is collected from [Open Clip Art].


[virtual screening]: http://en.wikipedia.org/wiki/Virtual_screening
[docking]: http://en.wikipedia.org/wiki/Docking_(molecular)
[AutoDock Vina]: http://vina.scripps.edu
[Apache License 2.0]: http://www.apache.org/licenses/LICENSE-2.0.html
[C++11]: http://en.wikipedia.org/wiki/C++11
[igrow]: https://github.com/HongjianLi/igrow
[Boost C++ Libraries]: http://www.boost.org
[doxygen]: http://www.doxygen.org
[CodePlex]: http://idock.codeplex.com
[Jacky Lee]: http://www.cse.cuhk.edu.hk/~hjli
[Open Clip Art]: http://www.openclipart.org

