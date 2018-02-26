/*
    cert.tst - Test SSL certificates
 */

if (!Config.SSL) {
    test.skip("SSL not enabled in ejs")

} else if (!global.test || App.config.me_ssl) {
    let http: Http

    for each (let provider in Http.providers) {
        if (provider == 'matrixssl') {
            //  MatrixSSL doesn't support certificate state yet
            continue
        }
        http = new Http
        http.provider = provider;
        http.ca = '../../crt/ca.crt'
        http.verify = true
        http.key = null
        http.certificate = null

        //  Verify the server (without a client cert)
        let endpoint = App.config.uris.ssl || "https://127.0.0.1:4443"
        assert(http.verify == true)
        assert(http.verifyIssuer == true)
        http.get(endpoint + '/index.html')
        assert(http.status == 200) 
        assert(http.info.SERVER_S_CN == 'localhost')
        assert(http.info.SERVER_I_EMAIL == 'licensing@example.com')
        assert(http.info.SERVER_I_OU != http.info.SERVER_S_OU)
        assert(!http.info.CLIENT_S_CN)
        http.close()

        //  Without verifying the server
        let endpoint = App.config.uris.ssl || "https://127.0.0.1:4443"
        http.verify = false
        assert(http.verify == false)
        assert(http.verifyIssuer == false)
        http.get(endpoint + '/index.html')
        assert(http.status == 200) 
        assert(http.info.SERVER_S_CN == 'localhost')
        assert(http.info.SERVER_I_EMAIL == 'licensing@example.com')
        assert(http.info.SERVER_I_OU != http.info.SERVER_S_OU)
        assert(!http.info.CLIENT_S_CN)
        http.close()

        //  Test a server self-signed cert. Verify but not the issuer.
        //  Note in a self-signed cert the subject == issuer
        let endpoint = App.config.uris.selfcert || "https://127.0.0.1:5443"
        http.verify = true
        http.verifyIssuer = false
        http.get(endpoint + '/index.html')
        assert(http.status == 200) 
        assert(http.info.SERVER_S_CN == 'localhost')
        assert(http.info.SERVER_I_OU == http.info.SERVER_S_OU)
        assert(http.info.SERVER_I_EMAIL == 'dev@example.com')
        assert(!http.info.CLIENT_S_CN)
        http.close()

        //  Test SSL with a client cert 
        endpoint = App.config.uris.clientcert || "https://127.0.0.1:6443"
        http.key = '../../crt/test.key'
        http.certificate = '../../crt/test.crt'
        // http.verify = false
        http.get(endpoint + '/index.html')
        assert(http.status == 200) 
        // assert(info.PROVIDER == provider)
        assert(http.info.CLIENT_S_CN == 'localhost')
        assert(http.info.SERVER_S_CN == 'localhost')
        assert(http.info.SERVER_I_OU != http.info.SERVER_S_OU)
        assert(http.info.SERVER_I_EMAIL == 'licensing@example.com')
        http.close()
    }

} else {
    test.skip("SSL not enabled")
}
