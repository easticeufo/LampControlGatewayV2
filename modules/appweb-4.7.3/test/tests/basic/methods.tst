/*
    methods.tst - Test misc Http methods
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

/*
//  Test methods are caseless
http.connect("GeT", HTTP + "/index.html")
assert(http.status == 200)
*/

//  Put a file
data = Path("test.dat").readString()
http.put(HTTP + "/tmp/test.dat", data)
assert(http.status == 201 || http.status == 204)

//  Delete
http.connect("DELETE", HTTP + "/tmp/test.dat")
if (http.status != 204) {
    print("STATUS IS " + http.status)
}
assert(http.status == 204)

//  Post
http.post(HTTP + "/index.html", "Some data")
assert(http.status == 200)

//  Options
http.connect("OPTIONS", HTTP + "/trace/index.html")
assert(http.header("Allow").split(',').sort() == "GET,OPTIONS,POST,TRACE")

http.connect("OPTIONS", HTTP + "/tmp/index.html")
assert(http.header("Allow").split(',').sort() == "DELETE,GET,OPTIONS,POST,PUT")

//  Trace - should be disabled by default
http.connect("TRACE", HTTP + "/index.html")
assert(http.status == 405)
http.connect("TRACE", HTTP + "/trace/index.html")
assert(http.status == 200)
assert(http.contentType == 'message/http')
assert(http.response.contains('HTTP/1.1 200 OK'))
assert(http.response.contains('Date'))
assert(http.response.contains('Content-Type: text/html'))
assert(!http.response.contains('Content-Length'))

//  Head 
http.connect("HEAD", HTTP + "/index.html")
assert(http.status == 200)
assert(http.header("Content-Length") > 0)
assert(http.response == "")
http.close()
