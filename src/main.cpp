#include "app.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

App app;

void main_loop() {
    app.Frame();
}

int main() {
    if (!app.Initialize()) {
        return -1;
    }

    emscripten_set_main_loop(main_loop, 0, 1);
}
#else
int main() {
    App app;
    if (!app.Initialize()) {
        return -1;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}
#endif