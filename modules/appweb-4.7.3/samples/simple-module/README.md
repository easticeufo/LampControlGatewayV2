SimpleModule Sample
===

This sample shows how to create an Appweb loadable module.  A module may provide an Appweb handler, filter, 
custom configuration directives or any functionality you wish to integrate into Appweb. Appweb modules are 
compiled into shared libraries and are dynamically loaded in response to appweb.conf LoadModule directives. 
If your main program is statically linked, the same module, without change may be included in the main
program executable link, provided the module entry point is manually invoked from the main program.

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

You will see trace in the console for the custom directive:

    CustomConfig = color=red

Code:
---
* [simpleModule.c](simpleModule.c) - Simple module
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Creating Handlers](http://embedthis.com/products/appweb/doc/guide/appweb/programmers/handlers.html)
* [Creating Modules](http://embedthis.com/products/appweb/doc/guide/appweb/programmers/modules.html)
* [API Library](http://embedthis.com/products/appweb/doc/api/native.html)

See Also:
---
* [simple-server - Simple one-line embedding API](../simple-server/README.md)
