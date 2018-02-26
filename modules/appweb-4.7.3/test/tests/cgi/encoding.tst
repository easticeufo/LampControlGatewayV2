/*
    encoding.tst - CGI URI encoding and decoding
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

let dotcgi = (!global.test || test.hostOs != "VXWORKS")
let vxworks = (!global.test || test.hostOs == "VXWORKS")

load("cgi.es")

http.get(HTTP + "/cgiProgram.cgi/extra%20long/a/../path/a/..?var%201=value%201")
match(http, "QUERY_STRING", "var%201=value%201")
match(http, "SCRIPT_NAME", "/cgiProgram.cgi")
match(http, "QVAR var 1", "value 1")
match(http, "PATH_INFO", "/extra long/path")

let scriptFilename = keyword(http, "SCRIPT_FILENAME")
let path = Path(scriptFilename).dirname.join("extra long/path")
let translated = Path(keyword(http, "PATH_TRANSLATED"))
assert(path == translated)

if (!vxworks) {
    /* VxWorks doesn't have "cgi Program" installed */
    http.get(HTTP + "/cgi-bin/cgi%20Program?var%201=value%201")
    match(http, "QUERY_STRING", "var%201=value%201")
    match(http, "SCRIPT_NAME", "/cgi-bin/cgi Program")
    match(http, "QVAR var 1", "value 1")
}
http.close()
