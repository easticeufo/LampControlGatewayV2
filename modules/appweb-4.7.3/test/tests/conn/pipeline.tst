/*
    Test pipelining
 */ 
const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"
const DELAY  = 500

let s = new Socket
let response = new ByteArray
s.connect(HTTP.address)
//  Test pipelined GET requests
let request =  "GET /alive.html HTTP/1.1\r\n\r\n"
    request += "GET /end.html HTTP/1.1\r\n\r\n"
s.write(request)
while ((n = s.read(response, -1)) != null) {
    if (response.toString().contains("END")) break
}
let r = response.toString()
if (!r.contains('ALIVE')) {
    print("TEST DIAGNOSTIC")
    print("RESPONSE: \"" + response.toString() + '"')
}
assert(r.contains('ALIVE'))
assert(r.contains('END'))
assert(r.match(/^HTTP\/1.1 200 OK/mg).length == 2)
s.close()


//  Test pipelined POST requests 
let s = new Socket
s.connect(HTTP.address)
let response = new ByteArray
request =  "POST /alive.html HTTP/1.1\r\nContent-Length: 3\r\n\r\n"
request += "a=b"
request += "GET /end.html HTTP/1.1\r\n\r\n"
s.write(request)
while ((n = s.read(response, -1)) != null) {
    if (response.toString().contains("END")) break
}
let r = response.toString()
assert(r.contains('ALIVE'))
assert(r.contains('END'))
assert(r.match(/^HTTP\/1.1 200 OK/mg).length == 2)
s.close()


//  Test pipelined POST form requests. The pipeline starts later for form data
let s = new Socket
s.connect(HTTP.address)
let response = new ByteArray
request =  "POST /alive.html HTTP/1.1\r\nContent-Length: 3\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"
request += "a=b"
request += "GET /end.html HTTP/1.1\r\n\r\n"
s.write(request)
while ((n = s.read(response, -1)) != null) {
    if (response.toString().contains("END")) break
}
let r = response.toString()
assert(r.contains('ALIVE'))
assert(r.contains('END'))
assert(r.match(/^HTTP\/1.1 200 OK/mg).length == 2)
s.close()


//  Test pipelined HTTP/1.0 final request.
let s = new Socket
let response = new ByteArray
s.connect(HTTP.address)
//  Test pipelined GET requests
let request =  "GET /alive.html HTTP/1.1\r\n\r\n"
    request += "GET /end.html HTTP/1.0\r\n\r\n"
s.write(request)
while ((n = s.read(response, -1)) != null) {
    if (response.toString().contains("END")) break
}
let r = response.toString()
assert(r.contains('ALIVE'))
assert(r.contains('END'))
assert(r.match(/^HTTP\/1.1 200 OK/mg).length == 1)
assert(r.match(/^HTTP\/1.0 200 OK/mg).length == 1)
s.close()
