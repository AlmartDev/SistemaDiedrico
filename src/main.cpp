#include "app.h"

int main() {
    App app;

    if (!app.Init()) 
        return -1;
    
    app.Run();
    app.Shutdown();
}