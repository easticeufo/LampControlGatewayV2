ESP Upload Sample
===

This sample shows how to configure ESP for file upload.

The sample includes an upload web form: web/upload/upload-form.html. This form will
post the uploaded file to the web/upload/upload.esp page.

For security, file upload is restricted to URIs starting with /upload.

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To run:
---
    me run

The server listens on port 8080. Browse to: 
 
     http://localhost:8080/upload/upload-form.html

Code:
---
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [web/upload/upload-form.html](web/upload/upload-form.html) - File upload form
* [web/upload/upload.esp](web/upload/upload.esp) - ESP page to receive the uploaded file
* [start.me](start.me) - MakeMe build instructions
* [cache](cache) - Compiled ESP modules

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [File Upload)(http://embedthis.com/products/appweb/doc/guide/appweb/users/uploading.html)
* [ESP Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/esp.html)
* [ESP APIs](http://embedthis.com/products/appweb/doc/api/esp.html)
* [ESP Guide](http://embedthis.com/products/appweb/doc/guide/esp/users/index.html)
* [ESP Overview](http://embedthis.com/products/appweb/doc/guide/esp/users/using.html)

See Also:
---
* [esp-angular-mvc - ESP Angular MVC Application](../esp-angular-mvc/README.md)
* [esp-controller - Creating ESP controllers](../esp-controller/README.md)
* [esp-html-mvc - ESP MVC Application](../esp-html-mvc/README.md)
* [esp-page - Serving ESP pages](../esp-page/README.md)
* [secure-server - Secure server](../secure-server/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
