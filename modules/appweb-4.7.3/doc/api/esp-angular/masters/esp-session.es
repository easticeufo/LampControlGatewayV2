/*
    Documentation for esp-session.js

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
module esp.angular {
    /**
        The SessionStore service object that provides access to client session storage using the
        window.sessionStorage API. When the browser window is closed, the values stored are removed.

        Values are converted to JSON strings before storing and are converted back from JSON to objects when retrieved.
        @stability prototype
     */
    class SessionStore {
        /**
            Get an item from session storage using the given key.
            @param key Unique item key identifier used when the item was put to the session storage.
            @return The previously stored object.
         */
        public static function get(key: String): Object { return null }
    
        /**
            Put an item to the session storage using the given key.
            @param key Unique item key identifier to associate with the item value.
            @param value Any Javascript value. These values are converted to strings via JSON before storing.
         */
        public static function put(key: String, value: Object): Void {}

        /**
            Remove an item from the session storage.
            @param key Unique item key identifier used when the item was put to the session storage.
         */
        public static function remove(key: String): Void {}
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
