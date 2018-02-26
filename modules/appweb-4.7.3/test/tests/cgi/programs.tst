/*
    programs.tst - Invoking CGI programs various ways
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let dotcgi = (!global.test || test.hostOs != "VXWORKS")

load("cgi.es")

//  Test various forms to invoke cgi programs
let http: Http = new Http
http.get(HTTP + "/cgi-bin/cgiProgram")
assert(http.status == 200)
contains(http, "cgiProgram: Output")

if (dotcgi) {
    http.get(HTTP + "/cgiProgram.cgi")
    assert(http.status == 200)
    contains(http, "cgiProgram: Output")
}

if (App.test && App.test.os == "WIN") {
    http.get(HTTP + "/cgi-bin/cgiProgram.exe")
    assert(http.status == 200)
    contains(http, "cgiProgram: Output")

    //  Test #!cgiProgram 
    http.get(HTTP + "/cgi-bin/test")
    assert(http.status == 200)
    contains(http, "cgiProgram: Output")

    http.get(HTTP + "/cgi-bin/test.bat")
    assert(http.status == 200)
    contains(http, "cgiProgram: Output")
}

http.get(HTTP + "/cgi-bin/testScript")
assert(http.status == 200)
contains(http, "cgiProgram: Output")
http.close()
