# Shellite

## Introduction

Shellite is a simple custom Unix shell developed in C and using POSIX system interfaces. It provides an interface through which users can run commands and manage processes. The shell supports running both background and foreground process execution. It also supports features like built in commands, process chaining with pipes, signal handling and basic job control. 

The main purpose behind shellite was to demonstrate operating system concepts such as process management, interprocess communication, redirecting outputs using pipes, signal handling and job descriptors. Shellite focuses on simplicity, readability and to demonstrate how these concepts work together to make a useful functioning application. 

## Features

- Foreground and background process execution
- Built-in commands: cd, exit, jobs, kill
- Multi-stage pipelines
- Signal handling
- Basic job control

## Requirements

- Linux or POSIX-compatible operating system
- GCC compiler
- GNU Make

## Compilation

Run the following command in the project directory:

```bash
make
```

## Usage

```bash
./shellite
```

## Example Commands

```bash
ls -l
cd ..
ls | wc -l
sleep 100 &
jobs
kill 0
exit
```

## Limitations

- Maximum of 16 commands per pipeline.
- Maximum of 127 arguements per command.
- Maximum of 10 tracked background jobs.
- Limited command parsing and job control.
