# Why are learned indexes so effective?

This repository contains the code to reproduce the experiments in the papers:

> Paolo Ferragina, Fabrizio Lillo, and Giorgio Vinciguerra. On the performance of learned data structures. Theoretical Computer Science, 2021. https://doi.org/10.1016/j.tcs.2021.04.015.

> Paolo Ferragina, Fabrizio Lillo, and Giorgio Vinciguerra. Why are learned indexes so effective?. In Proceedings of the 37th International Conference on Machine Learning (ICML). PMLR, 2020.

In brief, these papers give the first mathematical proof that, under certain general assumptions on the input data, the **PGM-index** ([website](https://pgm.di.unipi.it) | [repository](https://github.com/gvinciguerra/PGM-index)) is orders of magnitude more compressed than [traditional indexes](https://en.wikipedia.org/wiki/B-tree).
This result is important because it gives solid theoretical grounds to the excellent practical performance of [learned indexes](http://learned.di.unipi.it/publication/learned-data-structures/), pushing forward a new generation of data systems based on them.

## Build and run

To run the experiments you need CMake 3.8+, and a compiler with support for C++17 and OpenMP.
To compile the executables, issue the following commands:

    cmake . -DCMAKE_BUILD_TYPE=Release
    make

Then, the experiments can be run with these three scripts, which will populate a `result` directory with csv files:

    bash run_main.sh
    bash run_assumption_tests.sh
    bash run_segments_count.sh
    bash run_real_gaps.sh
    
The experiments may take quite some time to finish (approximately one week on our machine, whose specs are detailed below). 

## Analyse the results

The output files can be analysed in the Jupyter notebook `Figures and tables.ipynb`.
Other than the usual Python modules (numpy, pandas, matplotlib), [tikzplotlib](https://github.com/nschloe/tikzplotlib) is needed to export the figures in TikZ/PGFPlots.

## Test environment

The code was tested on the following machine:

| Component | Specs                          |
|-----------|--------------------------------|
| CPU       | Intel Xeon Gold 6132 @ 2.60GHz |
| RAM       | 376 GB                         |
| OS        | CentOS Linux 7                 |
| Compiler  | gcc 9.2.0                      |
| Python    | version 3.6.8                  |
| CMake     | version 3.16.2                 |

The output of `pip freeze | grep -E 'numpy|matplotlib|pandas|tikzplotlib'` was the following:

    matplotlib==3.0.3
    numpy==1.18.1
    pandas==0.24.1
    tikzplotlib==0.9.0
    
## License

This project is licensed under the terms of the GNU General Public License v3.0.

If you use this code for your research, please cite:

```
@article{Ferragina:2021tcs,
	author = {Paolo Ferragina and Fabrizio Lillo and Giorgio Vinciguerra},
	doi = {https://doi.org/10.1016/j.tcs.2021.04.015},
	issn = {0304-3975},
	journal = {Theoretical Computer Science},
	keywords = {Learned indexes, Data structures, B-trees, Predecessor search},
	title = {On the performance of learned data structures},
	year = {2021}}

@inproceedings{Ferragina:2020icml,
	author = {Ferragina, Paolo and Lillo, Fabrizio and Vinciguerra, Giorgio},
	booktitle = {Proceedings of the 37th International Conference on Machine Learning (ICML)},
	month = jul,
	publisher = {PMLR},
	series = {Proceedings of Machine Learning Research},
	title = {Why are learned indexes so effective?},
	volume = {119},
	year = {2020}}
```