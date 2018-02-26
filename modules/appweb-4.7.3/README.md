Embedthis Appweb 4.X
===

The fast, little web server for embedded applications. 

Branches
---
The repository has three branches:

* master - Most recent release of the software.
* lts - Most recent LTS release of the software.
* dev - Current ongoing development. Least stable and may not be suitable for
development.
 
Licensing
---
See [LICENSE.md](LICENSE.md) for details.

### To read documentation:

See https://embedthis.com/appweb/doc/index.html

### Building
    You can build with make or with MakeMe. MakeMe is quicker and more flexible.
    To install MakeMe, download MakeMe from
https://embedthis.com/makeme/download.html

### To build with make:

    make

You can pass make variables to tailor the build. For a list of variables:

	make help

### To build with MakeMe:

    ./configure
    me

For a list of configure options:

	./configure --help

### To run

	make run

or

    me run

### To install:

    sudo make install

or 

    sudo me install

### To uninstall

    sudo make uninstall

or 

    sudo me uninstall

### To test:

    me test

Resources
---
  - [Appweb web site](http://appwebserver.org/)
  - [Appweb GitHub repository](http://github.com/embedthis/appweb)
  - [Embedthis web site](https://embedthis.com/)
