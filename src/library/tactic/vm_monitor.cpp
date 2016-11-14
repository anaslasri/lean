/*
Copyright (c) 2016 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "library/vm/vm.h"
#include "library/vm/vm_name.h"
#include "library/tactic/tactic_state.h"

namespace lean {
static vm_obj _vm_monitor_register(vm_obj const & vm_n, vm_obj const & vm_s) {
    auto const & s = to_tactic_state(vm_s);
    auto const & n = to_name(vm_n);
    LEAN_TACTIC_TRY;
    return mk_tactic_success(set_env(s, vm_monitor_register(s.env(), n)));
    LEAN_TACTIC_CATCH(s);
}

void initialize_vm_monitor() {
    DECLARE_VM_BUILTIN(name({"vm_monitor", "register"}),  _vm_monitor_register);
}

void finalize_vm_monitor() {
}
}
