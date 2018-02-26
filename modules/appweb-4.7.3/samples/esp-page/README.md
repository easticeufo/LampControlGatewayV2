ESP Page Sample
===

This sample shows how to configure Appweb to serve ESP pages.

The ESP test page, index.esp, demonstrates the various ESP directives. These include:

* @@ to access request parameters/session variables
* @! to access local C variables
* <%= %> to emit the result of a C expression
* <%= %,d Number %> to use comma-separated number formatting
* <%= %S String %> to use safe-strings (HTML escape input)
* <%^start %> to emit code at the start of the generated page function
* <%^end %> to emit code at the end of the generated page function

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To run:
---
    me run

The server listens on port 8080. Browse to: 
 
     http://localhost:8080/index.esp
     http://localhost:8080/

Code:
---
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [index.esp](index.esp) - ESP page to serve
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [ESP Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/esp.html)
* [ESP APIs](http://embedthis.com/products/appweb/doc/api/esp.html)
* [ESP Guide](http://embedthis.com/products/appweb/doc/guide/esp/users/index.html)
* [ESP Overview](http://embedthis.com/products/appweb/doc/guide/esp/users/using.html)

See Also:
---
* [secure-server - Secure server](../secure-server/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
