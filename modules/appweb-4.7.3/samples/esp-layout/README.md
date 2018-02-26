ESP Layout Sample
===

This sample shows how to use ESP layout pages with stand-alone ESP pages.

The page to be served, index.esp specifies the desired layout page, layout.esp.
It does this with the <%@ layout "file" %> directive.

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To run:
---
    me run

The server listens on port 8080. Browse to: 
 
     http://localhost:8080/index.esp

Code:
---
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [index.esp](index.esp) - ESP page to serve. Uses layout.esp as a template.
* [layout.esp](index.esp) - ESP layout template
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
* [esp-angular-mvc - ESP Angular MVC Application](../esp-angular-mvc/README.md)
* [esp-html-mvc - ESP MVC Application](../esp-html-mvc/README.md)
* [esp-page - ESP Page](../esp-page/README.md)
* [secure-server - Secure server](../secure-server/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
