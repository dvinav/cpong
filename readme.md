# cPong

A simple **terminal Pong game written in C**, created as a practice project.
The program is tested on MacOS, and won't work on Windows.
By the way, I didn’t realize the original game was horizontal, not vertical, until I was done.

## Controls

### Bottom paddle

- **← Left Arrow**: move left  
- **→ Right Arrow**: move right  

### Top paddle

- **A**: move left  
- **D**: move right  

## Build Requirements

- A C compiler

## Build With GCC

```sh
gcc -O3 -o cpong cpong.c
