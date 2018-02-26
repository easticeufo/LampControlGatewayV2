ESP Controller Sample
===

This sample shows how to create and configure ESP controllers. The controller is in 
service.c. It registers one action that is run in response to the URI: /test/hello.

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
 
     http://localhost:8080/test/hello

This then returns "Hello World" to the client.

If you modify the service.c it will be automatically recompiled and reloaded when 
next accessed.

Code:
---
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [service.c](service.c) - ESP controller source
* [start.me](start.me) - MakeMe build instructions

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [ESP Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/esp.html)
* [ESP Tour](http://embedthis.com/products/appweb/doc/guide/esp/users/tour.html)
* [ESP Controllers](http://embedthis.com/products/appweb/doc/guide/esp/users/controllers.html)
* [ESP APIs](http://embedthis.com/products/appweb/doc/api/esp.html)
* [ESP Guide](http://embedthis.com/products/appweb/doc/guide/esp/users/index.html)
* [ESP Overview](http://embedthis.com/products/appweb/doc/guide/esp/users/using.html)

See Also:
---
* [esp-page - Serving ESP pages](../esp-page/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [secure-server - Secure server](../secure-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
