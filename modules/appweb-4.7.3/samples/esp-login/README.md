Esp-login Sample
===

This sample shows how to configure a simple form-based login using ESP. 
Note: this does not implement typical UI elements of warning the user, other than a basic alert.

This sample uses:

* SSL for encryption of traffic
* Redirection of all traffic over SSL
* Login authentication 
* Blowfish encryption for secure password hashing

This sample uses a self-signed certificate. In your application, you will need a real certificate.

Notes:
The password database is kept in a flat file called auth.conf. The password was created via:

    authpass --cipher blowfish --password pass5 auth.conf example.com ralph

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

To run:
---
    me run

The server listens on port 8080 for HTTP traffice and 4443 for SSL. Browse to: 
 
     http://localhost:8080/

This will redirect to SSL (you will get a warning due to the self-signed certificate).
Continue and you will be prompted to login. The test username/password is:

    ralph/pass5

Code:
---
* [server.c](server.c) - Main program
* [appweb.conf](appweb.conf) - Appweb server configuration file
* [self.crt](self.crt) - Self-signed test certificate
* [self.key](self.key) - Test private key
* [web](web) - Web content to serve
* [start.me](start.me) - MakeMe build instructions
* [cache](cache) - Directory for cached ESP pages

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Chroot Directive](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/server.html#chroot)
* [Configuration Directives](http://embedthis.com/products/appweb/doc/guide/appweb/users/configuration.html#directives)
* [Sandbox Limits](http://embedthis.com/products/appweb/doc/guide/appweb/users/dir/sandbox.html)
* [Security Considerations](http://embedthis.com/products/appweb/doc/guide/appweb/users/security.html)
* [SSL in Appweb](http://embedthis.com/products/appweb/doc/guide/appweb/users/ssl.html)
* [User Authentication](http://embedthis.com/products/appweb/doc/guide/appweb/users/authentication.html)

See Also:
---
* [min-server - Minimal server configuration](../min-server/README.md)
* [secure-server - Secure server configuration](../secure-server/README.md)
* [simple-server - Simple one-line embedding API](../simple-server/README.md)
* [ssl-server - SSL server](../ssl-server/README.md)
* [typical-server - Typical server configuration](../typical-server/README.md)
