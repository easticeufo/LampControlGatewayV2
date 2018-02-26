ESP LEGACY MVC Sample
===

WARNING: Do not use this sample for any new applications. The legacy-mvc ESP component is deprecated
and will be removed in Appweb 5. Instead, see the esp-html-mvc or esp-angular-mvc samples as an alternative.

This sample demonstrates a Legacy ESP MVC application. This was the old ESP MVC application layout. 

The app is a trivial blogging application. Posts with a title and body can be created, listed and deleted.

The app contains:
* blog database with post table
* post controller to manage posts
* post views to create, list and display posts
* master view layout under the layouts directory

This app was generated, then edited for simplicity via:

    esp --name blog install esp-legacy-mvc
    esp generate scaffold post title:string body:text

For the new style ESP MVC application, see the esp-html-mvc sample:

* [esp-html-mvc](../esp-html-mvc/README.md)

Differences
---
The key changes in the new style html-mvc vs the legacy-mvc are:
* The "static" directory of public client-side content is renamed to "client"
* The "views" are distributed under "client/app/CONTROLLER" directories
* The "layouts" directory is moved under "client/layouts"
* The base URL to access the application uses a server prefix "/do"

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To build:
---
    me 
or

    esp compile

To run:
---
    me run
or

    esp run

The server listens on port 8080. Browse to: 
 
     http://localhost:8080/post/

Notes:
---
If you modify the controller.c it will be automatically recompiled and reloaded when next accessed.

Code:
---
* [controllers](controllers/post.c) - Post controller
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [cache](cache) - Directory of compiled ESP modules
* [db](db) - Database directory for the blog application
* [db/blog.mdb](db/blog.mdb) - Blog database 
* [db/migrations](db/migrations) - Database base migrations to create / destroy the database schema
* [layouts](layouts) - Master view layout templates 
* [static](static) - Static web content
* [start.me](start.me) - MakeMe build instructions
* [views](views) - Web views

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
* [esp-page - Serving ESP pages](../esp-page/README.md)
* [secure-server - Secure server](../secure-server/README.md)
* [simple-server - Simple server and embedding API](../simple-server/README.md)
* [typical-server - Fully featured server and embedding API](../typical-server/README.md)
