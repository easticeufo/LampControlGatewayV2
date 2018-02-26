/*
    empty-form.tst - Test an missing form
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"

let http: Http = new Http

http.post(HTTP + "/expect-missing", "")
assert(http.status == 404)
http.close()
