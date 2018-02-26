Deploy Appweb Sample
===

This sample demonstrates the commands to use to deploy Appweb files to a staging directory.

Requirements
---
* [Appweb](http://embedthis.com/downloads/appweb/download.ejs)
* [MakeMe Build Tool](http://embedthis.com/downloads/me/download.ejs)

Steps:
---

1. Appweb must be built

    me 

2. Deploy 

    me --sets core,libs,esp --deploy dir

This will copy the required Appweb files to deploy into the nominated directory.

Other sets include: 'web', 'service', 'php', 'utils', 'test', 'dev', 'doc', 'package'

Documentation:
---
* [Appweb Documentation](http://embedthis.com/products/appweb/doc/index.html)
* [Building Appweb with MakeMe](http://embedthis.com/products/appweb/doc/guide/appweb/source/me.html)

See Also:
---
* [min-server - Minimal server configuration](../min-server/README.md)
* [simple-server - Simple one-line embedding API](../simple-server/README.md)
