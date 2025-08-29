//
// Created by jonathan on 8/29/25.
//

#include <string>
#include "natives.h"

cell AMX_NATIVE_CALL mqtt_create(AMX *amx, cell *params) {
    enum { arg_count };

    return 0;
}

AMX_NATIVE_INFO g_natives[] =
{
    {"mqtt_create", mqtt_create}
};
