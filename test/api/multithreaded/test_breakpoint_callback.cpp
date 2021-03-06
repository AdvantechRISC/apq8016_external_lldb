
// LLDB C++ API Test: verify that the function registered with
// SBBreakpoint.SetCallback() is invoked when a breakpoint is hit.

#include <mutex>
#include <iostream>
#include <vector>
#include <string>

#include "lldb-headers.h"

#include "common.h"

using namespace std;
using namespace lldb;

mutex g_mutex;
condition_variable g_condition;
int g_breakpoint_hit_count = 0;

bool BPCallback (void *baton,
                 SBProcess &process,
                 SBThread &thread,
                 SBBreakpointLocation &location) {
  lock_guard<mutex> lock(g_mutex);
  g_breakpoint_hit_count += 1;
  g_condition.notify_all();
  return true;
}

void test(SBDebugger &dbg, vector<string> args) {
  SBTarget target = dbg.CreateTarget(args.at(0).c_str());
  if (!target.IsValid()) throw Exception("invalid target");

  SBBreakpoint breakpoint = target.BreakpointCreateByName("next");
  if (!breakpoint.IsValid()) throw Exception("invalid breakpoint");
  breakpoint.SetCallback(BPCallback, 0);

  std::unique_ptr<char> working_dir(get_working_dir());
  SBProcess process = target.LaunchSimple(0, 0, working_dir.get());

  {
    unique_lock<mutex> lock(g_mutex);
    g_condition.wait_for(lock, chrono::seconds(5));
    if (g_breakpoint_hit_count != 1)
      throw Exception("Breakpoint hit count expected to be 1");
  }
}
