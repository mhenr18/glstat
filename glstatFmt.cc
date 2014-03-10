//
// glstatFmt.cc
// Copyright (c) 2014 Matthew Henry.
// MIT licensed - refer to LICENSE.txt for details.
//

#include <CoreFoundation/CoreFoundation.h>

#include <iostream>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
using namespace std;

struct gl_context {
    unordered_map<string, vector<double>> function_call_times;
};

int main(int argc, char **argv)
{
    string line;
    regex line_re("(0x[a-z0-9]*) @ (\\d*\\.\\d*): (\\S*)");
    unordered_map<string, gl_context> contexts;
    mutex mu;
    thread outputter;
    atomic<bool> running(true);
    double output_rate = 0.1;

    // use an outputter thread to handle writing out logs at given intervals
    outputter = thread([&] {

    });

    while (cin.good()) {
        smatch m;

        getline(cin, line);

        if (regex_match(line, m, line_re)) {
            lock_guard<mutex> lock(mu);

            auto& context = contexts[m[1]];
            double t = stod(m[2]);
            string function = m[3];

            context.function_call_times[function].push_back(t);
        }
    }

    running = false;
    outputter.join();
    return 0;
}