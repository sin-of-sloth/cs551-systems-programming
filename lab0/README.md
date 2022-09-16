# Homework #1

Homework 1 requirements can be found in **ppmcvt.pdf**.

**ppmcvt.ref** is a reference executable you can test yours against.

## Steps to cross-examine your code

- Run your executable
- Run reference executable
- Do a diff on the output files

*NOTE: I have created a script to run all these three together in .zshrc - source the file after specifying an input image, and run:*

```bash
ppmcvt {options}
```

*e.g. `ppmcvt -i red`*
