/*
    Support functions for Embedthis Appweb

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

require ejs.tar
require ejs.unix

public function startService() {
    if (Config.OS != 'windows' && App.uid != 0) {
        trace('Skip', 'Must be root to install Appweb daemon')
        return
    }
    if (!me.cross) {
        stopService(true)
        trace('Run', 'appman install enable start')
        Cmd([me.prefixes.bin.join('appman' + me.globals.EXE), 'install', 'enable', 'start'])
        if (me.platform.os == 'windows') {
            Cmd([me.prefixes.bin.join('appwebMonitor' + me.globals.EXE)], {detach: true})
        }
    }
}


public function stopService(quiet: Boolean = false) {
    if (Config.OS != 'windows' && App.uid != 0) {
        return
    }
    if (!me.cross) {
        if (!quiet) {
            trace('Stop', me.settings.title)                                                     
        }
        if (me.platform.os == 'windows') {
            try {
                Cmd([me.prefixes.bin.join('appwebMonitor'), '--stop'])
            } catch {}
        }
        let appman = Cmd.locate(me.prefixes.bin.join('appman'))
        if (appman) {
            try {
                Cmd([appman, '--continue', 'stop', 'disable', 'uninstall'])
            } catch {}
        }
    }
}


/*
    @copy   default
  
    Copyright (c) Embedthis Software. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2014. All Rights Reserved.
  
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
