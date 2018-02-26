/*
    args.tst - CGI program command line args
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

let dotcgi = (!global.test || test.hostOs != "VXWORKS")

load("cgi.es")

//
//  Note: args are split at '+' characters and are then shell character encoded. Typical use is "?a+b+c+d
//
http.get(HTTP + "/cgi-bin/cgiProgram")
assert(keyword(http, "ARG[0]").contains("cgiProgram"))
assert(!http.response.contains("ARG[1]"))

if (dotcgi) {
    http.get(HTTP + "/cgiProgram.cgi/extra/path")
    assert(keyword(http, "ARG[0]").contains("cgiProgram"))
    assert(!http.response.contains("ARG[1]"))

    http.get(HTTP + "/cgiProgram.cgi/extra/path?a+b+c")
    match(http, "QUERY_STRING", "a+b+c")
    assert(keyword(http, "ARG[0]").contains("cgiProgram"))
    match(http, "ARG.1.", "a")
    match(http, "ARG.2.", "b")
    match(http, "ARG.3.", "c")
}
http.get(HTTP + "/cgi-bin/cgiProgram?var1=a+a&var2=b%20b&var3=c")
match(http, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c")
http.close()
