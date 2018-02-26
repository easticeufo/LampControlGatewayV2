/*
    Documentation for esp-resource.js

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
module esp.angular {
    /**
        The EspResource supports RESTful HTTP requests to ESP applications.

        The EspResource factory is used to create resource objects that interact with the server via
        RESTful HTTP requests. EspResource is typically used to create a resource object for each data model. 
        Consider the example below that creates a resource group for users. This creates routes for the standard restful 
        routes of: create, edit, init, list, remove and update, as well as creating custom routes for forgot, login and logout.
        Each of these routes is represented by a function that can be called to issue the corresponding HTTP RESTful request.
        <pre>
angular.module('app').factory('User', function (EspResource) {
&nbsp;    return <b>EspResource.group</b>("user", {}, {
&nbsp;        forgot: { method: 'POST', url: '/:prefix/:controller/forgot' },
&nbsp;        login:  { method: 'POST', url: '/:prefix/:controller/login' },
&nbsp;        logout: { method: 'POST', url: '/:prefix/:controller/logout' },
&nbsp;    }); 
});
</pre>
        Once EspResource.group creates the resource object and it is registered as an Angular service via factory() above, then 
        controllers may use the service to issue RESTful HTTP requests on the server. For example, to issue a HTTP request 
        to list the users on the server:
        <pre>User.list(null, $scope);</pre>
        or to logout:
        <pre>User.logout(null, $scope);</pre>

        This simple function call does more than you might expect.
        The resource route function formats request data using JSON from the supplied $scope. It issues the request, and 
        then processes the response extracting results and error messages. This makes communication with ESP simple,
        efficient, and reduces the number of lines of code required to interact with the server. 

        The server-side ESP framework accepts parameters in JSON format and generates responses back to the client 
        that include out-of-band feedback that describes error messages and informational feedback. The feedback
        is stored in the $rootScope.feedback object which is typically databound for display.

        The routes created for group and for singleton resources are listed below.
        For each route, a service method of the same name is created to call the server. The routes for a group are: 
        <pre>
            create: POST,   /:prefix/:controller,
            edit:   GET,    /:prefix/:controller/:id/edit,
            get:    GET,    /:prefix/:controller/:id,
            init:   GET,    /:prefix/:controller/init,
            list:   GET,    /:prefix/:controller/list,
            remove: DELETE, /:prefix/:controller/:id,
            update: POST,   /:prefix/:controller/:id,
        </pre>

        The routes for a singleton resource are:
        <pre>
            create: POST,   /:prefix/:controller,
            edit:   GET,    /:prefix/:controller/edit,
            get:    GET,    /:prefix/:controller,
            init:   GET,    /:prefix/:controller/init,
            remove: DELETE, /:prefix/:controller,
            update: POST,   /:prefix/:controller,
        </pre>
        The prefix is determined automatically by ESP and may be overridden by the request params. The controller is
        provided as the first parameter to the group/solo function. The "id" if required should be supplied to the
        resource function.

        The calling sequence for resource functions is:
        <pre>Resource.action(params, [scope], [mappings], [successCallback], [failureCallback]</pre>

        The request params provide POST data for the request and may be used to override the the URL "prefix", 
        "controller" and "id". The params are also passed to the underlying $

        The scope is an optional output object to receive the response data. The mappings parameter is an object hash
        of mappings to rename response items to a more convenient local name. The success and failure callbacks are invoked
        as appropriate when the request completes.


        @stability prototype
 */
    class EspResource {
        /**
            Create a set of routes for a resource group. This should be used when there are more than one instance
            of a resource. For example: a Users resource group has one or more users.
            @param controller String Name of the controller field in the route URL.
            @param params Object Request parameters to supply as POST data. This is JSON encoded before transmission.
            @param options Hash of options to qualify request. These are passed to the underlying 
                Angular $http service object and permit setting HTTP headers, timeouts and HTTP credentials.
                Some of the key options are:
            @option method HTTP method to use.
            @option params Additional request parameters to pass via POST data
            @option timeout Timeout in milliseconds
            @option rest All other options are passed through to the $http service
            @return An EspResource object which may be used to issue requests on the server. 
            The object contains methods for create, edit, init, list, remove and update.
         */
        public static function group(controller: Object, params: Object, options: Object): EspResource { return null }
    
        /**
            Create a set of routes for a single resource. This should be used when there is only one instance
            of a resource. For example: an Administrator resource has only one administrator.
            @param controller String Name of the controller field in the route URL.
            @param params Object Request parameters to supply as POST data. This is JSON encoded before transmission.
            @param options Hash of options to qualify request. These are passed to the underlying 
                Angular $http service object and permit setting HTTP headers, timeouts and HTTP credentials.
                Some of the key options are:
            @option method HTTP method to use.
            @option params Additional request parameters to pass via POST data
            @option timeout Timeout in milliseconds
            @return The a EspResource object which may be used to issue requests on the server. 
                The object contains methods for create, edit, init, remove and update.
            @example:
                Port.get({id: $scope.id}, $scope, {}, function(response) {
                    // Complete
                });

                //  Get a list of ports and map the response data to the "ports" property.
                Port.list(null, $scope, {ports: "data"});
         */
        public static function solo(controller: Object, params: Object, options: Object): EspResource { return null }
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
