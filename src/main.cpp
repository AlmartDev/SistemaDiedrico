#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <emscripten.h>
#include "app.h"

App app;

void main_loop() {
    app.Frame();
}

int main() {

    if (!app.Initialize()) {
        return -1;
    }

    // 8. Main loop
    emscripten_set_main_loop(main_loop, 0, 1);
}
#else
#include "app.h"
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