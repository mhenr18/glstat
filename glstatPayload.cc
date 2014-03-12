//
// glstatPayload.cc
// Copyright (c) 2014 Matthew Henry.
// MIT licensed - refer to LICENSE.txt for details.
//

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <OpenGL/OpenGL.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "mach_override/mach_override.h"
using namespace std;
using namespace std::chrono;

struct GLContext {
    map<string, vector<steady_clock::time_point>> callTimes;
};

FILE *fin, *fout, *ferr;
CGLError (*CGLFlushDrawableReentry)(CGLContextObj);
mutex mu;
mach_timebase_info_data_t timebaseInfo;
steady_clock::time_point start;
thread outputter;
map<CGLContextObj, GLContext> contexts;
milliseconds outputRate(250);
set<string> functionsToTrack = { "CGLFlushDrawable" };

extern "C" CGLError CGLFlushDrawableOverride(CGLContextObj ctx)
{
    {
        lock_guard<mutex> lock(mu);
        contexts[ctx].callTimes["CGLFlushDrawable"].push_back(steady_clock::now());
    }
    
    return CGLFlushDrawableReentry(ctx);
}

extern "C" void payload_main(int in, int out, int err)
{
    fin = fdopen(in, "r");
    fout = fdopen(out, "w");
    ferr = fdopen(out, "w");

    setvbuf(fin, NULL, _IONBF, 0);
    setvbuf(fout, NULL, _IONBF, 0);
    setvbuf(ferr, NULL, _IONBF, 0);

    mach_timebase_info(&timebaseInfo);
    start = steady_clock::now();

    if (mach_override_ptr((void *)CGLFlushDrawable, 
        (void *)CGLFlushDrawableOverride, (void **)&CGLFlushDrawableReentry))
    {
        fprintf(ferr, "glstat: failed to override CGLFlushDrawable\n");
        return;
    }

    fprintf(fout, "context, function, time, call rate\n");

    outputter = thread([&] {
        int i = 0;

        while (true) {
            {
                lock_guard<mutex> lock(mu);
                auto now = steady_clock::now();
                double dnow = duration_cast<microseconds>(now - start).count() / 1e6;

                for (auto& p : contexts) {
                    auto ctx_addr = p.first;
                    auto& callTimes = p.second.callTimes;

                    for (auto& p : callTimes) {
                        auto& funcName = p.first;
                        auto& times = p.second;

                        // get rid of everything older than a second
                        times.erase(std::remove_if(times.begin(), times.end(),
                            [&](steady_clock::time_point t) {
                                return now - t > seconds(1);
                            }),
                            times.end());

                        // and print our frames in the current second if we're
                        // tracking this function
                        if (functionsToTrack.count(funcName)) {
                            fprintf(fout, "%p, %s, %.6f, %d\n",
                                (void*)ctx_addr, funcName.c_str(), dnow, 
                                (int)times.size());
                        }
                    }
                }
            }

            ++i;
            this_thread::sleep_until(start + (i * outputRate));
        }
    });
}
