#include "ConnectOptionMgr.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
void Require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}
} // namespace

int main() {
    ConnectOptionMgr manager;
    const int consumed = manager.make();
    Require(consumed > 0, "Options handle is not positive");
    manager.destroy(consumed);
    const int fresh = manager.make();
    Require(fresh > consumed, "Destroyed options handle was reused");
    Require(manager.getOptions(consumed) == nullptr, "Stale handle resolves to new options");
    manager.destroy(consumed);
    Require(manager.getOptions(fresh) != nullptr, "Stale destroy deleted new options");

    manager.reset();
    const int reloaded = manager.make();
    Require(reloaded > fresh, "Options handles were reset during plugin reload");
    Require(manager.getOptions(fresh) == nullptr, "Pre-reload handle resolves to new options");
    manager.destroy(fresh);
    Require(manager.getOptions(reloaded) != nullptr, "Pre-reload destroy deleted new options");
    manager.reset();

    // The limit is on live objects, not total handles ever issued. Unknown
    // lookups must neither insert entries nor consume active capacity.
    for (int i = 0; i < 10001; ++i) {
        Require(manager.getOptions(-i) == nullptr, "Unknown options handle exists");
    }
    std::vector<int> handles;
    for (int i = 0; i < 10000; ++i) {
        handles.push_back(manager.make());
    }
    bool capacityRejected = false;
    try {
        manager.make();
    } catch (const mqtt::exception &) {
        capacityRejected = true;
    }
    Require(capacityRejected, "Live options limit was not enforced");
    manager.destroy(handles.front());
    const int replacement = manager.make();
    Require(replacement > handles.back(), "Freed capacity reused an old handle");
    Require(manager.getOptions(handles.front()) == nullptr, "Destroyed options became valid again");
    manager.destroy(handles.front());
    Require(manager.getOptions(replacement) != nullptr, "Stale destroy removed replacement options");
    manager.reset();
    const int afterReset = manager.make();
    Require(afterReset > replacement, "Reset reused a handle after reaching capacity");
    std::cout << "PASS: stale options, reset isolation, invalid lookups, and live capacity" << std::endl;
}
