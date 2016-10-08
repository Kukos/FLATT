# FLATT

Repo contains MY ( student's ) code for FLATT course.

Project: Compiler for lang and machine described by Maciek Gębala

###### Course:
    Formal languages and translation techniques
###### University:
    Wroclaw University of Science and Technology
###### Faculty:
    Faculty of Fundamental Problems of Technology ( Computer Science )

All code in C is written in std GNU99 and compiled using gcc -std=gnu99 -Wall -pedantic -Werror -O3 on linux

## Project structure

| Directory  | Content                            		  |
| ---------- | ------------------------------------------ |
| `include`  | headers 									  |
| `src`      | sources                   				  |
| `libs`     | external libraries (non-standart)          |
| `obj`      | generated files from srcs                  |
| `main`     | main compiler file						  |
| `tests`    | tests									  |
| `external` | external files ( libs and interpreter)     |


### To build compiler
make compiler

### To build compiler with debugs
make compiler_dbg

### To build and run tests
make test

### To build interpreter
make interpreter

### To build mylibs
make libs

### To build all
make all or make

### To clean object and executable files
make clean

### To clean mylibs objects
make clean_libs
