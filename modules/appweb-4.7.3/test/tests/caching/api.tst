/*
    api.tst - Test configuration of caching by API
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

if (!global.test || App.config.me_esp) {

    //  Prep and clear the cache
    http.get(HTTP + "/cache/clear")
    assert(http.status == 200)

    //  This will load the controller and configure caching for "api"
    http.get(HTTP + "/cache/api")
    assert(http.status == 200)

    //  This request should now be cached
    http.get(HTTP + "/cache/api")
    assert(http.status == 200)
    let resp = deserialize(http.response)
    let first = resp.number
    assert(resp.uri == "/cache/api")
    assert(resp.query == "null")

    http.get(HTTP + "/cache/api")
    assert(http.status == 200)
    resp = deserialize(http.response)
    assert(resp.number == first)
    assert(resp.uri == "/cache/api")
    assert(resp.query == "null")

    http.close()

} else {
    test.skip("ESP not enabled")
}
