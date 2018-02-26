/*
    status.tst - Test CGI program status responses
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
load("cgi.es")

let http = new Http
http.setHeader("SWITCHES", "-s%20711")
http.get(HTTP + "/cgi-bin/cgiProgram")
assert(http.status == 711)
http.close()
