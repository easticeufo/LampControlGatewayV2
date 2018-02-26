Typical Client Sample
===

This sample shows how to efficiently use the Http library to issue Http client requests.
This is a fuller sample suitable for applications that need to issue multiple HTTP requests.
If you only need to issue one HTTP request, consult the simple-client sample.

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

The client retrieves:
 
     http://www.embedthis.com/index.html

Code:
---
* [client.c](client.c) - Main program
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Configuration Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/configuration.html#directives)
* [Http Client](http://embedthis.com/products/appweb/doc/guide/appweb/users/client.html)
* [Http API](http://embedthis.com/products/appweb/doc/api/http.html)
* [API Library](http://embedthis.com/products/appweb/doc/api/native.html)

See Also:
---
* [simple-client - Simple client and embedding API](../simple-client/README.md)
