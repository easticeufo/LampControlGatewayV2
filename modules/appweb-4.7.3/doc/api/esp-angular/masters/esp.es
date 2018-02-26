/*
    Documentation for esp.js

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
module esp.angular {
    /**
        Esp Angular service object that provides support for ESP applications.
        As an Angular service, there is only one instance of "Esp". 
        The Esp service may be injected (imported) via the Angular injector. It is also aliased as $$rootScope.Esp.
        
        The Esp object provides utility function and state properties that may be accessed directly from 
        the Esp service object. The Esp service performs background activities to smooth the execution of Esp 
        applications. Specifically:
        <ul>
            <li>The Esp service will track the browser referrer and store it in Esp.referrer. </li>
            <li>The Esp service will assist in managing the user feedback service. Whenever a browser click event is received the transient feedback message area will be cleared.</li>
            <li>The Esp service will manage client side sessions. While the ultimate arbiter of user sessions is 
                done on the server side, the Esp service will mirror a client side understanding of the server 
                side service. The Esp service will warn the user when a session is about to expire and will 
                provide details of the authenticated user via the Esp.user.name and the Esp.user.abilities 
                properties.</li>
            <li>The Esp service wraps the Angular $http service and intercepts responses to provide 
                uniform error handling. Responses with a 401 HTTP_UNAUTHORIZED status code are handled with 
                the browser redirected to the login page if required. Responses are monitored and feedback 
                messages are intercepted and sent to the feedback area for display.</li>
            <li>The Esp service provides a checkAuth routine that can be called when resolving routes. This 
                routine tests if the authenticated user has the requisite abilities to be granted access.</li>
            </ul>
        @stability prototype
     */
    class Esp {
        /**
            Determine if the user authorized to perform the given task.
            The user's abilities are defined by the server and passed to the client when the user logs in.
            @note: this is advisory only to provide hints in the UI. It is the server's repsonsibility to
            restrict user abilities as appropriate. We allow !user because auto-login will not set these.
            @param task Required task string
            @return True if the user has the ability to perform the requested task.
         */
        public static function can(task: String): Boolean { return true }
    
        /**
            Set an error feedback message.
            By default, error messages are persistent and are only cleared when the user performs a 
            new operation in the application.
            @param msg Message to display
         */
        public static function error(msg: String): Void {}

        /**
            Set an information feedback message.
            By default, informational messages are cleared wherenver a user clicks or navigates in the application.
            @param msg Message to display
         */
        public static function inform(msg: String): Void {}

        /**
            Load a script.
            This routine loads the requested script and executes it in the browser by appending a script DOM
            element to the document body.
            @param script URL of script to load
            @param callback Optional callback to invoke when the script is loaded.
         */
        public static function loadScript(script: Uri, callback: Function): Void {}

        /**
            Login as a user.
            This performs a client-side registration of a server-authenticated user.
            The user details are saved in Esp.user. After this routine, Esp will monitor the user session 
            and warn when the session is due to expire.
            @param user Object containing a 'name' property for the username.
         */
        public static function login(user): Void {}

        /**
            Logout the current user.
            This removes the client-side user registration. The local session is discarded.
            Note: this does not logout the user from the server. Rather, after logging out from the server, the 
            application should call Esp.logout to allow the client to provide visual feedback to the user.
         */
        public static function logout(): Void {}

        /**
            Map a string to TitleCase.
            @param str String to convert
            @return A titlecase string where the first letter of each word is capitalized.
         */
        public static function titlecase(str: String): String  { return "" }

        /**
            Return a qualified client URI.
            This routine supports hosting an application using an application URI prefix.
            This returns a fully qualified URI including the application URI prefix by prepending the
            application prefix from Esp.config.appPrefix.
            @param uri The client URI for a template or script without the application URI prefix.
         */
         public static function url(uri: Uri): String { return ""}

        /*
            Contents of the package.json configuration file
         */
        public static var config: Object

        /**
            Authenticated user information
            @options name Authenticated username
         */
        public static var user: Object

        /**
            Prior client URL
         */
        public static var referrer: String
    }
}


/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2014. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
