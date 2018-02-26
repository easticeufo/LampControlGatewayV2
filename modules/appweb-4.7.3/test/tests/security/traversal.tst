/*
    Test directory traversal
 */

const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"

http = new Http
http.get(HTTP + "/../../appweb.conf")
assert(http.status == 400)
http.close()

http.get(HTTP + "/../../index.html")
assert(http.status == 400)
http.close()

/* Test windows '\' delimiter */
http.get(HTTP + "/..%5Cappweb.conf")
if (Config.OS == 'windows') {
    /* for windows, the "..\\" is an invalid filename */
    assert(http.status == 400)
} else {
    assert(http.status == 404)
}
http.close()
