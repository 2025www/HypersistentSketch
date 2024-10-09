# Hypersistent Sketch

* Hypersistent Sketch: Enhanced Persistence Estimation in Web Data Stream via Rapid Item Separation

# Introduction

* The source codes of Hypersistent Sketch and other related algorithms.

# How to run

Suppose you've already cloned the repository.

You just need:

```
$ cmake .
$ make
$ ./bench (-d dataset -m memory -w window's size)
```

**optional** arguments:

- -d: set the path of dataset to run, default dataset is CAIDA "./data/caida.dat"
- -m: set the memory size (KB), default memory is 25KB
- -w: set the size of window, default **w** is 1500
