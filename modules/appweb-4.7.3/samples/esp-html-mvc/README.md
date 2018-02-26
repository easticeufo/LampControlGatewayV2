ESP MVC Sample
===

This sample demonstrates a stand-alone ESP MVC application using server-side HTML page views. 
The app is a trivial blogging application.  Posts with a title and body can be created, listed and deleted.

An alternative approach is to use a client-side Javascript framework to generate views on the client.
Te [esp-angular-mvc](../esp-angular-mvc/README.md) sample demonstrates such an approach.

The app contains:
* blog database with post table in the db directory
* post controller to manage posts in the controllers directory
* post views to create, list and display posts in the client/app/post directory
* master view layout under the client/layouts directory

This app was generated then edited via:

    esp --name blog install esp-html-mvc
    esp generate scaffold post title:string body:text

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
 
     http://localhost:8080/do/post/list

Notes:
---
If you modify the controller or web pages they will be automatically recompiled and reloaded when next accessed.
The "/do" prefix is for server-side URIs for the application. So the "client" directory is published as: /blog/.

Code:
---
* [controllers](controllers/post.c) - Post controller
* [appweb.conf](appweb.conf) - Application appweb configuration file
* [cache](cache) - Directory of compiled ESP modules
* [client](client) - Client-side public web content
* [client/app](client/app) - Client-side application per-module pages and scripts
* [client/assets](client/assets) - Client-side media assets
* [client/css](client/css) - Client-side CSS and Less stylesheets
* [client/index.esp](client/index.esp) - Application home page
* [client/layouts](client/layouts) - Master view layout templates 
* [client/lib](client/lib) - Client-side 3rd-party libraries
* [db](db) - Database directory for the blog application
* [db/blog.mdb](db/blog.mdb) - Blog database 
* [db/migrations](db/migrations) - Database base migrations to create / destroy the database schema
* [package.json](package.json) - ESP configuration file
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
* [esp-controller - Creating ESP controllers](../esp-controller/README.md)
* [esp-angular-mvc - ESP Angular MVC sample](../esp-angular-mvc/README.md)
* [esp-page - Serving ESP pages](../esp-page/README.md)
* [secure-server - Secure server](../secure-server/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
