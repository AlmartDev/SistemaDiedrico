#include "app.h"

// TODO: (in order of priority)
// - Points should be deleted, not hidden, this makes everything more complex and buggy (revised 1 time)
// - Refactor app class and check for memory leaks and bad practices
// - Quality check on renderer class
// - Better plane drawing and dihedral plane drawing
//      - More plane presets
// - Save/load current workspace
//      - Better JSON handling (json.h refactor)
// - Web Assembly build
//      - SDL2 for web?
// - FUTURE: lenguage support

int main() {
    App app;

    if (!app.Initialize()) {
        return -1;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}