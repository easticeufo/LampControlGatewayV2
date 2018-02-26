/*
    form.tst - Post form tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"

let http: Http = new Http

if (App.config.me_esp) {

    //  Empty form
    http.post(HTTP + "/form.esp", "")
    assert(http.status == 200)
    http.close()

    //  Simple string
    http.post(HTTP + "/form.esp", "Some data")
    assert(http.status == 200)
    http.close()

    //  Two keys
    http.form(HTTP + "/form.esp", {name: "John", address: "700 Park Ave"})
    assert(http.response.contains('name=John'))
    assert(http.response.contains('address=700 Park Ave'))
    http.close()

} else {
    test.skip("ESP not enabled")
}
