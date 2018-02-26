/*
    location.tst - CGI redirection responses
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
load("cgi.es")

let http = new Http
http.setHeader("SWITCHES", "-l%20/index.html")
http.followRedirects = false
http.get(HTTP + "/cgi-bin/cgiProgram")
assert(http.status == 302)
http.close()
