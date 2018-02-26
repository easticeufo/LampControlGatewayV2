/*
    npg.tst - CGI non-parsed header tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let vxworks = (!global.test || test.hostOs == "VXWORKS")
load("cgi.es")
let http = new Http

if (!vxworks) {
    /* VxWorks doesn't have "nph-cgiProgram" */
    http.setHeader("SWITCHES", "-n")
    http.get(HTTP + "/cgi-bin/nph-cgiProgram")
    assert(http.status == 200)
    assert(http.header("X-CGI-CustomHeader") == "Any value at all")

    //  Do another to make sure it handles the content correctly
    http.reset()
    http.setHeader("SWITCHES", "-n")
    http.get(HTTP + "/cgi-bin/nph-cgiProgram")
    assert(http.status == 200)
    assert(http.header("X-CGI-CustomHeader") == "Any value at all")
}
http.close()
