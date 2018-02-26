/*
    session.tst - ESP session tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

//  GET /session/login
http.get(HTTP + "/session/login")
assert(http.status == 200)
assert(http.response.contains("Please Login"))
let securityToken = http.header("X-XSRF-TOKEN")
let cookie = http.header("Set-Cookie")
if (cookie) {
    cookie = cookie.match(/(-http-session-=.*);/)[1]
}

assert(cookie && cookie.contains("-http-session-="))
http.close()

//  POST /session/login
http.setCookie(cookie)
http.setHeader("X-XSRF-TOKEN", securityToken)
http.form(HTTP + "/session/login", { 
    username: "admin", 
    password: "secret", 
    color: "blue" 
})
assert(http.status == 200)
assert(http.response.contains("Valid Login"))
if (http.header("Set-Cookie")) {
    cookie = http.header("Set-Cookie");
    cookie = cookie.match(/(-http-session-=.*);/)[1]
}
http.close()

//  GET /session/login
App.sleep(1000);
http.setCookie(cookie)
http.get(HTTP + "/session/login")
assert(http.status == 200)
assert(http.response.contains("Logged in"))
http.close()
