/*
    ${UCONTROLLER} Controller (esp-mvc)
 */
#include "esp.h"

${ACTIONS}

ESP_EXPORT int esp_controller_${APP}_${CONTROLLER}(HttpRoute *route, MprModule *module) {
    ${DEFINE_ACTIONS}    
    return 0;
}
