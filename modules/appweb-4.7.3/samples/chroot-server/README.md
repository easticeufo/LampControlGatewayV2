Chroot Sample
===

This sample shows how to run Appweb in a Chroot-jail. This is where Appweb runs with enhanced
security by changing its root directory so that the rest of the operating system is inaccessible.

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To build:
---
    me 
    esp compile

    # Note that the ESP pages must be pre-compiled as the cc compiler wont be available inside the chroot jail.

To run:
---
    sudo me run

The server listens on port 8080. Browse to:

     http://localhost:8080/
     http://localhost:8080/test.esp

Alternatively, run appweb to use the appweb.conf file.

    sudo appweb

Notes:
---
So that the Compiler is not required inside the jail, the ESP pages are pre-compiled before running Appweb.
Appweb is configured to load modules before changing the root directory via the "Chroot" directive in appweb.conf.

Code:
---
* [server.c](server.c) - Main program
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [index.html](index.html) - Web page to serve
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Chroot Directive](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/server.html#chroot)
* [Security Considerations](http://embedthis.com/products/appweb/doc/guide/appweb/users/security.html)

See Also:
---
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
