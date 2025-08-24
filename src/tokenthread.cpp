#include "tokenthread.hpp"

#include "config.hpp"
#include <curl/curl.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>
#include <exception>
#include <nn/act.h>
#include <thread>
#include "utils/logger.h"
#include "utils/token.hpp"

namespace tokenthread {
    bool running = false;
    bool should_kill = false;
    bool should_run_once = false;
    nn::act::SlotNo lastSlotNo;

    void token_thread() {
        while(true) {
            nn::act::SlotNo res = nn::act::GetSlotNo();
            if (lastSlotNo != res) {
                lastSlotNo = res;
                token::updCurrentToken();
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if(should_kill || should_run_once) {
                should_run_once = false;
                break;
            }
        }
    }

    void CreateTokenThread() {
        try {
            std::jthread tokenThread(token_thread);

            auto threadHandle = (OSThread*) tokenThread.native_handle();
            OSSetThreadName(threadHandle, "rverse Token Account Handler");
            OSSetThreadAffinity(threadHandle, OS_THREAD_ATTRIB_AFFINITY_ANY);

            tokenThread.detach();
        } catch(std::exception &e) {
            DEBUG_FUNCTION_LINE("Couldn't make thread: %s", e.what());
        }
    }
}