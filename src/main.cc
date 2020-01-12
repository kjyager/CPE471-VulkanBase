#include <iostream>
#include "utils/optional.h"
#include <cassert>
#include "Application.h"

int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    return(0);
}