#ifndef WEMU_CTRL_HWSIM_CTRL_EVENT_H
#define WEMU_CTRL_HWSIM_CTRL_EVENT_H

#include "hwsim_ctrl_cli.h"

#define UNUSED(x) (void)(x)

int register_event(hwsim_cli_ctx *ctx);

int wait_for_event();

#endif //WEMU_CTRL_HWSIM_CTRL_EVENT_H
