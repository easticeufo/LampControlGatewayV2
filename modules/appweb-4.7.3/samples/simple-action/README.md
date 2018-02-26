Simple Action Sample
===

This sample shows how to create actions. i.e. Simple bindings from URIs to C functions.
The sample contains a main program which initializes the server and registers the required
actions.

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To build:
---
    me 

To run:
---
    me run

The server listens on port 8080. Browse to: 
 
     http://localhost:8080/action/myaction
     http://localhost:8080/action/myaction?name=Peter&address=Park+Lane

Notes:
---
The MyAction will parse the form/query parameters and echo their values back.

Code:
---
* [server.c](server.c) - Main program with action
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [index.html](index.html) - Web page to serve
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Action Handler](http://embedthis.com/products/appweb/doc/guide/appweb/users/frameworks.html#action)
* [Configuration Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/configuration.html#directives)
* [Sandbox Limits](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/sandbox.html)

See Also:
---
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
