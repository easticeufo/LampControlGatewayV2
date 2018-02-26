/*
    condition.tst - Test conditional routing based on certificate data
 */

if (!Config.SSL) {
    test.skip("SSL not enabled in ejs")

} else if (App.config.me_ssl) {
    let http: Http

    for each (let provider in Http.providers) {
        if (provider == 'matrixssl') {
            //  MatrixSSL doesn't support certificate state yet
            continue
        }
        http = new Http
        http.provider = provider;
        // http.ca = '../../crt/ca.crt'
        http.verify = false

        //  Should fail if no cert is provided
        endpoint = App.config.uris.clientcert || "https://127.0.0.1:6443"
        let caught
        try {
            /* Should throw */
            http.get(endpoint + '/ssl-match/index.html')
            assert(http.status == 200) 
        } catch {
            caught = true
        }
        assert(caught)
        http.close()

        //  Should pass with a cert
        endpoint = App.config.uris.clientcert || "https://127.0.0.1:6443"
        http.key = '../../crt/test.key'
        http.certificate = '../../crt/test.crt'
        http.get(endpoint + '/ssl-match/index.html')
        assert(http.status == 200) 
        http.close()
    }

} else {
    test.skip("SSL not enabled")
}
