/*
    jquery.esp.js - ESP jQuery support
    version: 1.1
  
    Copyright (c) 2014 Embedthis Software
    Dual licensed under GPL licenses and the Embedthis commercial license.
    See http://embedthis.com/ for further licensing details.
 */

;(function($) {
    var defaults = {
        "updating": true,               // Enable live updating
        "toggle-updating": 27           // ESC to toggle updating
    }
    $.fn.extend({
        /*
            Load or reload esp support. Can be called after adding new elements to a page.
         */
        esp: function(options) {
            $.extend(defaults, options || {});
            $('[data-esp-sort-order]').each(sort);
            $('[data-esp-refresh]').each(refresh);
            $('span[class~=esp-flash-inform]').animate({opacity: 1.0}, 1000).hide("slow");
            $('div[class~=esp-flash-inform]').animate({opacity: 1.0}, 1000).hide("slow");

            /*
                Attach event handlers
             */
            $(document).on('submit', 'form', function (e) {
                var method = $(this).attr('data-esp-method');
                if (method) {
                    $(this).append('<input name="-http-method-" value="' + method + '" type="hidden" />');
                }
            });

            $(document).on('submit', '[data-esp-feedback]', function (e) {
                var o = getDataAttributes($(this))
                var msg = o["data-esp-feedback"];
                var info = $('<div>' + msg + '</div>').addClass('esp-feedback');
                $('main').append(info);
                anim.call(info, "bold");
                return true;
            });

            /* Click with data-esp-confirm */
            $(document).on('click', 'a[data-esp-confirm],input[data-esp-confirm]', function () {
                var elt = $(this);
                elt.bind('confirm', function (evt) {
                    return confirm(elt.attr('data-esp-confirm'));
                });
                elt.trigger('confirm');
            });

            /* Click in form. Ensure submit buttons are added for remote calls */
            $(document).on('click', 'input[type=submit]', function (e) {
                $(this).attr("checked", true);
            });

            /*
                Remote (background) requests
             */
            /* Click on link foreground with data-esp-method to override method */
            $(document).on('click', 'a[data-esp-method]:not([data-esp-remote])', function (e) {
                request.apply(this)
                e.preventDefault();
            });

            /* Button click data-esp-remote */
            $(document).on('click', 'button[data-esp-remote]', function(e) {
                remote.apply(this);
                e.preventDefault();
            });

            /* Click on form with data-esp-remote (background) */
            $(document).on('submit', 'form[data-esp-remote]', function (e) {
                var method = $(this).attr('data-esp-method');
                if (method) {
                    $(this).append('<input name="-http-method-" value="' + method + '" type="hidden" />');
                }
                remote.apply(this);
                e.preventDefault();
            });

            /* Click on link with data-esp-remote (background) */
            $(document).on('click', 'a[data-esp-remote],input[data-esp-remote]', function (e) {
                remote.apply(this);
                e.preventDefault();
            });
            /* Click on table row data-esp-remote */
            $(document).on('click', 'tr[data-esp-remote]', function(e) {
                var table = $(this).parents("table");
                var url = $(this).attr('data-esp-remote');
                remote.call(table, url);
                e.preventDefault();
            });

            $(document).on('click', '[data-esp-click] a', function (e) {
                request.apply(this);
                return false;
            });

            /* Click on anything with data-esp-click */
            $(document).on('click', '[data-esp-click]', function (e) {
                request.apply(this);
                e.preventDefault();
            });

            /*
                Window Status updates for mouseover
             */
            /* Update status with the URI for data-esp-click */
            $(document).on('mouseover', '[data-esp-click]', function (e) {
                var click = $(this).attr("data-esp-click");
                if (click.indexOf("http://") == 0) {
                    window.status = click;
                } else {
                    var location = window.location;
                    window.status = location.protocol + "//" + location.host + click;
                }
            });

            $(document).on('mouseout', '[data-esp-click]', function (e) {
                window.status = "";
            });

            /* Update status with the URI for data-esp-remote */
            $(document).on('mouseover', '[data-esp-remote]', function (e) {
                var remote = $(this).attr("data-esp-remote");
                if (remote.indexOf("http://") == 0) {
                    window.status = remote;
                } else {
                    var location = window.location;
                    window.status = location.protocol + "//" + location.host + remote;
                }
            });

            $(document).on('mouseout', '[data-esp-remote]', function (e) {
                window.status = "";
            });

            /* Click data-esp-toggle */
            $(document).on('click', 'li[data-esp-toggle]', function(e) {
                var next = $(this).attr('data-esp-toggle');
                $('div[id|=pane]').hide();
                var pane = $('div#' + next);
                pane.fadeIn(500);
                pane.addClass('esp-pane-visible');
                pane.removeClass('esp-pane-hidden');
                e.preventDefault();
                return false
            });

            /* 
                Refresh control indicators with class esp-refresh
                <img src="green.png" data-on="green.png" data-off="red.png" class="ejs-refresh" />
             */
            $(document).on('click', '[class~=esp-refresh]', function (e) {
                $('[data-esp-refresh]').each(function() {
                    refreshToggle.call(this);
                });
                refreshKeyEvent();
                return false;
            });

            $('[data-esp-modal]').each(function() {
                $(this).modal({
                    escClose: false, 
                    overlayId: "esp-modal-background", 
                    containerId: "esp-modal-foreground"
                });
            });
        }
    });

    /*
        Define the defaults property
     */
    $.fn.esp.defaults = defaults;

    /************************************************* Support ***********************************************/
    /*
        Background request using data-esp-remote attributes. Apply the result to the data-esp-apply (or current element).
     */
    function remote(url) {
        var elt    = $(this),
            data   = elt.is('form') ? elt.serializeArray() : [],
            method = elt.attr('data-esp-remote-method') || elt.attr('method') || 'GET';

        if (url === undefined) {
            url = elt.attr('data-esp-remote') || elt.attr('action') || elt.attr('href');
            if (!url) {
                throw "No URL specified for remote call";
            }
        }
        elt.trigger('http:before');
        $.ajax({
            url: url,
            data: data,
            type: method.toUpperCase(),
            beforeSend: function (http) {
                elt.trigger('http:loading', http);
            },
            success: function (data, status, http) {
                elt.trigger('http:success', [data, status, http]);
                var apply = elt.attr('data-esp-apply');
                if (apply) {
                    $(apply).html(data)
                }
            },
            complete: function (http) {
                elt.trigger('http:complete', http);
            },
            error: function (http, status, error) {
                elt.trigger('http:failure', [http, status, error]);
            }
        });
        elt.trigger('http:after');
    }

    /* UNUSED
    function copyDataAttributes(from) {
        this.each(function() {
            var e = $(this);
            $.each($(from)[0].attributes, function(index, att) {
                if (att.name.indexOf("data-esp-") == 0) {
                    if (e.attr(att.name) == null) {
                        e.attr(att.name, att.value);
                    }
                }
            });
        });
    }
    */

    /*
        Get an object containing all data-esp-attributes
     */ 
    function getDataAttributes(elt) {
        var elt = $(elt);
        var result = {};
        $.each(elt[0].attributes, function(index, att) {
            if (att.name.indexOf("data-esp-") == 0) {
                result[att.name] = att.value;
            }
        });
        return result;
    }

    /*
        Foreground request using data-esp-* element attributes. This makes all elements clickable and supports 
        non-GET methods with security tokens to aleviate CSRF.
     */
    function request() {
        var el      = $(this);
        var method  = el.attr('data-esp-click-method') || el.attr('data-esp-method') || 'GET';
        var url     = el.attr('data-esp-click') || el.attr('action') || el.attr('href');
        var params  = el.attr('data-esp-click-params') || el.attr('data-esp-params');

        if (url === undefined) {
            alert("No URL specified");
            return;
        }
        method = method.toUpperCase();

        if (method == "GET") {
            window.location = url;
        } else {
            var parts = document.cookie.split("XSRF-TOKEN=");
            var token = (parts.length == 2) ? parts.pop().split(";").shift() : null;
            if (token) {
                var form = $('<form method="post" action="' + url + '"/>').
                    append('<input name="-http-method-" value="' + method + '" type="hidden" />').
                    append('<input name="-xsrf-" value="' + token + '" type="hidden" />');
                if (params) {
                    params = params.split("&")
                    var k;
                    for (k in params) {
                        var param = params[k];
                        var pair = param.split("=")
                        form.append('<input name="' + pair[0] + '" value="' + pair[1] + '" type="hidden" />');
                    }
                }
                form.hide().appendTo('body').submit();
            } else {
                alert("Missing Security Token");
            }
        }
    }

    /*
        Refresh dynamic elements using a data-esp-refresh attributes
     */
    function refresh(options) {
        function applyData(data, http) {
            var d, id, oldID, newElt;
            var id = this.attr("id");
            var apply = this.attr('data-esp-apply') || id;
            var e = (apply) ? $('#' + apply) : $(this);
            var o = e.data("esp-options");
            var contentType = http.getResponseHeader("Content-Type") || "text/html";

            try {
                if (contentType == "text/html") {
                    if (e[0] && e[0].tagName == "TABLE") {
                        var config = data[0].config = e[0].config;
                        if (config) {
                            elt.tablesorter({ sortList: config.sortList });
                        }
                    }
                    e.html($.trim(data));
                } else {
                    d = e[0];
                    if (d.type == "checkbox") {
                        if (data == e.val()) {
                            e.attr("checked", "yes")
                        } else {
                            e.removeAttr("checked")
                        }
                    } else if (d.type == "radio") {
                        if (data == e.val()) {
                            e.attr("checked", "yes")
                        } else {
                            e.removeAttr("checked")
                        }
                    } else if (d.type == "text" || d.type == "textarea") {
                        e.val(data);
                    } else if (d.tagName == "IMG") {
                        e.attr("src", data)
                    } else {
                        e.text(data);
                    }
                }
            } catch (e) {
                console.debug(e);
            }
            anim.call(e);
            return e;
        }

        function update(options) {
            var elt = $(this);
            var id = elt.attr("id");
            var o = $.extend({}, options, elt.data("esp-options") || {}, getDataAttributes(elt));
            elt.data("esp-options", o);
            if (o.updating) {
                var method = o["data-esp-method"] || "GET";

                $.ajax({
                    url: o['data-esp-refresh'],
                    type: method,
                    success: function (data, status, http) { 
                        if (http.status == 200) {
                            elt = applyData.call(elt, data, http); 
                        } else if (http.status == 0) {
                            log("Error updating control: can't connect to server");
                        } else {
                            log("Error updating control: " + http.statusText + " " + http.responseText); 
                        }
                    },
                    complete: function() {
                        setTimeout(function(elt) { 
                            update.call(elt, o); 
                        }, o["data-esp-period"], elt);
                    },
                    error: function (http, msg, elt) { 
                        log("Error updating control: " + msg); 
                    }
                });
            } else {
                setTimeout(function(elt) { update.call(elt, o);}, o["data-esp-period"], elt);
            }
            if (!o.bound) {
                $(document).bind('keyup.refresh', function (event) {
                    if (event.keyCode == o["toggle-updating"]) {
                        /* Need to refetch as the elements has probably been replaced */
                        refreshToggle.call($('[id=' + id + ']'));
                    }
                });
                o.bound = true;
            }
        }

        /*
            Initialze the refresh
         */
        var elt = $(this);
        var refreshCfg = $.extend({}, defaults, options || {});
        var period = getDataAttributes(elt)["data-esp-period"];
        setTimeout(function(elt) { 
            update.call(elt, refreshCfg); 
        }, period, elt);
        return this;
    } /* end of refresh() */

    function refreshToggle() {
        elt = $(this);
        var o = elt.data("esp-options");
        if (o) {
            o.updating = !o.updating;
            elt.data("esp-options", o);
        }
    }

    /* Used for keyboard on the refresh control */
    function refreshKeyEvent(event) {
        if (event == null || event.keyCode == defaults["toggle-updating"]) {
            var e = $("[class~=esp-refresh]");
            if (e.attr("src") == e.attr("data-esp-off")) {
                e.attr("src", e.attr("data-esp-on"));
            } else {
                e.attr("src", e.attr("data-esp-off"));
            }
        }
    }

    /*
        Apply table sorting. Uses tablesorter plugin.
     */
    function sort(options) {
        var elt = $(this);
        var o = $.extend({}, defaults, options, elt.data("esp-options") || {}, getDataAttributes(elt));
        if (!o.sortConfig) {
            if (o.sort != "false") {
                var items = $("th", elt).filter(':contains("' + o.sort + '")');
                var sortColumn = $("th", elt).filter(':contains("' + o.sort + '")').get(0);
                if (sortColumn) {
                    var order = (o["sort-order"].indexOf("asc") >= 0) ? 0 : 1;
                    o.sortConfig = {sortList: [[sortColumn.cellIndex, order]]};
                } else {
                    o.sortConfig = {sortList: [[0, 0]]};
                }
            }
            elt.tablesorter(o.sortConfig);
        }
        elt.data("esp-options", o);
        return this;
    }

    function anim(effects) {
        e = $(this);
        if (effects == "highlight") {
            e.fadeTo(0, 0.5);
            e.fadeTo(500, 1);
        } else if (effects == "fadein") {
            e.fadeTo(0, 0);
            e.fadeTo(1000, 1);
        } else if (effects == "fadeout") {
            e.fadeTo(0, 1);
            e.fadeTo(1000, 0);
        } else if (effects == "bold") {
            e.animate({"opacity": 1.0}, 1000);
        }
    }

    function log(msg) {
        if (window.console) {
            console.debug(msg);
        } else {
            alert(msg);
        }
    }

    /**************************** Initialization ************************/

    $(document).ready(function() {
        $(document).esp();
        $(document).bind('keyup.refresh', refreshKeyEvent);
    });

})(jQuery);

