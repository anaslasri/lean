/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <iostream>
#include <fstream>
#include <signal.h>
#include <cstdlib>
#include <getopt.h>
#include "util/stackinfo.h"
#include "util/debug.h"
#include "util/interrupt.h"
#include "util/script_state.h"
#include "kernel/printer.h"
#include "frontends/lean/parser.h"
#include "frontends/lean/frontend.h"
#include "frontends/lua/register_modules.h"
#include "version.h"

using lean::shell;
using lean::frontend;
using lean::parser;
using lean::script_state;
using lean::unreachable_reached;
using lean::invoke_debugger;
using lean::notify_assertion_violation;

enum class input_kind { Unspecified, Lean, Lua };

static void on_ctrl_c(int ) {
    lean::request_interrupt();
}

static void display_header(std::ostream & out) {
    out << "Lean (version " << LEAN_VERSION_MAJOR << "." << LEAN_VERSION_MINOR << ")\n";
}

static void display_help(std::ostream & out) {
    display_header(out);
    std::cout << "Input format:\n";
    std::cout << "  --lean         use parser for Lean default input format for files,\n";
    std::cout << "                 with unknown extension (default)\n";
    std::cout << "  --lua          use Lua parser for files with unknown extension\n";
    std::cout << "Miscellaneous:\n";
    std::cout << "  --help -h      display this message\n";
    std::cout << "  --version -v   display version number\n";
    std::cout << "  --luahook=num  how often the Lua interpreter checks the interrupted flag,\n";
    std::cout << "                 it is useful for interrupting non-terminating user scripts,\n";
    std::cout << "                 0 means 'do not check'.\n";
}

static char const * get_file_extension(char const * fname) {
    if (fname == 0)
        return 0;
    char const * last_dot = 0;
    while (true) {
        char const * tmp = strchr(fname, '.');
        if (tmp == 0) {
            return last_dot;
        }
        last_dot  = tmp + 1;
        fname = last_dot;
    }
}

static struct option g_long_options[] = {
    {"version",    no_argument,       0, 'v'},
    {"help",       no_argument,       0, 'h'},
    {"lean",       no_argument,       0, 'l'},
    {"lua",        no_argument,       0, 'u'},
    {"luahook",    required_argument, 0, 'c'},
    {0, 0, 0, 0}
};

int main(int argc, char ** argv) {
    lean::save_stack_info();
    lean::register_modules();
    input_kind default_k = input_kind::Lean; // default
    while (true) {
        int c = getopt_long(argc, argv, "vhc:012", g_long_options, &optind);
        if (c == -1)
            break; // end of command line
        switch (c) {
        case 'v':
            display_header(std::cout);
            return 0;
        case 'h':
            display_help(std::cout);
            return 0;
        case 'l':
            default_k = input_kind::Lean;
            break;
        case 'u':
            default_k = input_kind::Lua;
            break;
        case 'c':
            script_state::set_check_interrupt_freq(atoi(optarg));
            break;
        default:
            std::cerr << "Unknown command line option\n";
            display_help(std::cerr);
            return 1;
        }
    }
    frontend f;
    script_state S;
    if (optind >= argc) {
        display_header(std::cout);
        std::cout << "Type Ctrl-D to exit or 'Help.' for help."<< std::endl;
        shell sh(f, &S);
        signal(SIGINT, on_ctrl_c);
        return sh();
    } else {
        bool ok = true;
        for (int i = optind; i < argc; i++) {
            char const * ext = get_file_extension(argv[i]);
            input_kind k     = default_k;
            if (ext) {
                if (strcmp(ext, "lean") == 0) {
                    k = input_kind::Lean;
                } else if (strcmp(ext, "lua") == 0) {
                    k = input_kind::Lua;
                }
            }
            if (k == input_kind::Lean) {
                std::ifstream in(argv[i]);
                parser p(f, in, &S, false, false);
                if (!p())
                    ok = false;
            } else if (k == input_kind::Lua) {
                try {
                    S.dofile(argv[i]);
                } catch (lean::exception & ex) {
                    std::cerr << ex.what() << std::endl;
                    ok = false;
                }
            } else {
                lean_unreachable(); // LCOV_EXCL_LINE
            }
        }
        return ok ? 0 : 1;
    }
}
