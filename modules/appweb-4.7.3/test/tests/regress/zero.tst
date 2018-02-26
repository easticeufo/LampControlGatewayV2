/*
    zero.tst - Test zero conent length form
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"

let http: Http = new Http

http.setHeader('Content-Length', 0)
http.setHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8')
http.post(HTTP + '/test.esp')
assert(http.status == 200)
http.finalize()
http.close()
