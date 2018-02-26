/*
    big-response.tst - CGI big response data
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
load("cgi.es")

/* Depths:    0  1  2  3   4   5   6    7    8    9    */
let sizes = [ 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 ]
let depth = global.test ? test.depth : 1
let len = sizes[depth] * 102400
let bytes = len * 11

let http = new Http
http.setHeader("SWITCHES", "-b%20" + len)
http.get(HTTP + "/cgi-bin/cgiProgram")
assert(http.status == 200)
if (bytes != http.response.length) {
    print('\n', bytes, '\n', http.response.length)
}
assert(bytes == http.response.length)
http.close()
