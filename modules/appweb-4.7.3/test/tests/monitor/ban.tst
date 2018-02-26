/*
    ban.tst - Monitor and ban defense tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

if (!global.test || test.depth >= 5) {
    /*
        Trigger the ban with > 190 requests in a 5 sec period
     */
    for (i in 200) {
        http.get(HTTP + "/unknown.html")
        assert(http.status == 404)
        http.close()
    }
    /*
        Wait for the ban to come into effect. Should happen in 0-5 secs.
     */
    for (i in 10) {
        http.get(HTTP + "/unknown.html")
        if (http.status == 404) {
            http.close()
            App.sleep(1000)
            continue
        }
        assert(http.status == 406)
        assert(http.response.contains("Client temporarily banned due to monitored limit exceeded"))
        break
    }
    /*
        Should be banned
     */
    assert(http.status == 406)
    http.close()

    /* 
        A valid URI should now fail 
     */
    http.get(HTTP + "/index.html")
    assert(http.status == 406)
    http.close()

    /*
        Wait for the ban to be lifted. Should be 0-5 secs.
     */
    for (i in 10) {
        http.get(HTTP + "/index.html")
        if (http.status == 406) {
            assert(http.response.contains("Client temporarily banned due to monitored limit exceeded"))
            http.close()
            App.sleep(1000)
            continue
        }
        assert(http.status == 200) 
        break
    }
    assert(http.status == 200) 
    http.close()
    
    /* 
        A valid URI should now work 
     */
    http.get(HTTP + "/index.html")
    assert(http.status == 200)
    http.close()

} else {
    test.skip("Runs at depth 3")
}
