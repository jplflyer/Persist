# Project Persist
This is the start of a C++ based persistence generator. From a data model, different generators can create:

* SQL to create tables and define relationships
* POCOs -- plain old C++ objects
* Database Access Methods

Ultimately, it would be great if there was a driver that could point to an existing database and apply updates to it rather than a simple "create from scratch" script. I'm not prepare to write that yet.

# Overall Concept
Imagine a typical web application. There's a client that runs on the browser. Most typically that's JavaScript + HTML + CSS. There's a server that handles requests. And there's some sort of database for persistence.

My view is of this:

* Client: C++ with Qt for WebAssembly
* Server: nginx for serving files plus a REST server in C++
* Database: PostgreSQL

# Requirements
This project is pure C++. Dependence upon external libraries will be kept minimal. Compilation should succeed on both Mac and Linux. No promises for Windows.

This is a Makefile-based app. I don't personally want to mess with CMake. If someone comes along who needs it, they can contribute to the project.

My IDE is Qt Creator. See Qt.io. I check in the project files, but you can ignore them. I do not depend on any Qt libraries in this project.

I am targeting PostgreSQL. If you wish to target a different database, you can subclass the relevant classes for your own generators.

# License
This project uses the MIT license, attached in a separate file.
