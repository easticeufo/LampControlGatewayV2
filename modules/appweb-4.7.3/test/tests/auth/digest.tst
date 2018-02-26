/*
    digest.tst - Digest authentication tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

http.setCredentials("anybody", "PASSWORD WONT MATTER")
http.get(HTTP + "/index.html")
assert(http.status == 200)

//  Digest tests
//  Access to digest/digest.html accepts by any valid user
http.reset()
http.get(HTTP + "/auth/digest/digest.html")
assert(http.status == 401)

http.setCredentials("joshua", "pass1")
http.get(HTTP + "/auth/digest/digest.html")
assert(http.status == 200)

http.setCredentials("mary", "pass2")
http.get(HTTP + "/auth/digest/digest.html")
assert(http.status == 200)

//  Access accepts joshua only
http.setCredentials(null, null)
http.get(HTTP + "/auth/digest/joshua/user.html")
assert(http.status == 401)

http.setCredentials("joshua", "pass1")
http.get(HTTP + "/auth/digest/joshua/user.html")
assert(http.status == 200)

http.setCredentials("mary", "pass2")
http.get(HTTP + "/auth/digest/joshua/user.html")
assert(http.status == 403)
http.close()
