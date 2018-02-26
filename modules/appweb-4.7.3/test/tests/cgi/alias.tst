/*
    alias.tst - CGI via aliased routes
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

let dotcgi = (!global.test || test.hostOs != "VXWORKS")

load("cgi.es")

http.get(HTTP + "/MyScripts/cgiProgram")
assert(http.status == 200)
contains(http, "cgiProgram: Output")
match(http, "SCRIPT_NAME", "/MyScripts/cgiProgram")
match(http, "PATH_INFO", "")
match(http, "PATH_TRANSLATED", "")

if (dotcgi) {
    http.get(HTTP + "/YourScripts/cgiProgram.cgi")
    assert(http.status == 200)
    contains(http, "cgiProgram: Output")
}
http.close()
