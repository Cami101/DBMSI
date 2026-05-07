## COMP 530: Database System Implementation

### Introduction

This repo is for the course COMP 530: Database System Implementation. The whole work of these assignments are about database management system architecture, buffer management, query processing and optimization, transaction processing, concurrency control and recovery, data storage, indexing structures, and related topics. They're all about C++ and DBMS.

### Contents

- Buffer and file management
- Record manager
- Sorted file implementation
- B+-tree implementation
- SQL type checking
- Relational operators
- Multi-threading

### Usage

In the home directory, follow the following steps:

1. Download SCons: `wget --no-check-certificate https://pypi.python.org/packages/source/S/SCons/scons-3.1.2.tar.gz`
2. Unpack it: `gunzip scons-3.1.2.tar.gz` `tar xvf scons-3.1.2.tar`
3. Build it: `mkdir scons` `cd scons-3.1.2/` `python setup.py install --prefix=../scons` `cd ..` `rm -r scons-3.1.2/`
4. Run it: `~/scons/bin/scons-3.1.2`

As for testing the projects, in the directory of project, follow the following steps:

1. Enter directory ./Build: `cd ./Build/`
2. Compile and Build it: `~/scons/bin/scons-3.1.2`
3. Select the module(s) you want to build or clean.
4. Test it: `./bin/stackUnitTest`
