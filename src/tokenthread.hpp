#pragma once
namespace tokenthread {
    extern bool running;
    extern bool should_kill;
    extern bool should_run_once;
    void CreateTokenThread();
}