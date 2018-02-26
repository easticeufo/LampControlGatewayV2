Tyical Server Sample
===

This sample shows how to embed Appweb using the full embedding API.

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
 
     http://localhost:8080/

Code:
---
* [server.c](server.c) - Main program
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [auth.conf](auth.conf) - User/Password/Role authorization file
* [cgi-bin](cgi-bin) - Directory for CGI programs
* [web](web) - Web content to serve
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Configuration Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/configuration.html#directives)
* [Sandbox Limits](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/sandbox.html)
* [Security Considerations](http://embedthis.com/products/appweb/doc/guide/appweb/users/security.html)
* [SSL in Appweb](http://embedthis.com/products/appweb/doc/guide/appweb/users/ssl.html)
* [User Authentication](http://embedthis.com/products/appweb/doc/guide/appweb/users/authentication.html)

See Also:
---
* [min-server - Minimal server configuration](../min-server/README.md)
* [simple-server - Simple one-line embedding API](../simple-server/README.md)
