#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include "doctest.h"
#include <signal.h>
#include <unistd.h>

// Timeout handler for macOS (30 seconds)
static volatile bool timeout_triggered = false;

void timeout_handler(int sig) {
    (void)sig;
    timeout_triggered = true;
    fprintf(stderr, "\n[TEST TIMEOUT] Test execution exceeded 30 seconds. Aborting.\n");
    fflush(stderr);
    _exit(1);
}

// Include all test files
#include "test_vm_basic.cpp"

// Custom main with timeout for macOS
int main(int argc, char** argv) {
    // Set up 30-second timeout using alarm (macOS compatible)
    signal(SIGALRM, timeout_handler);
    alarm(30);
    
    // Run doctest
    int result = doctest::Context(argc, argv).run();
    
    // Cancel alarm if tests complete
    alarm(0);
    
    if (timeout_triggered) {
        return 1;
    }
    
    return result;
}

