#include <iostream>
#include "utils/optional.h"
#include <cassert>
#include "Application.h"

int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    optional<int> x;
    x = 5;
    assert(x.has_value());
    assert(x.value() == 5);
    x.reset();
    assert(!x.has_value());
    try{
        x.value();
        assert(false);
    }catch(bad_optional_access& e){
        printf("Exception caught (this is a good thing)\n");
    }

    struct Snitch{
        Snitch(int x) : value(x) { std::cout << "Snitch(" << x << ") constructed" << std::endl; }
        ~Snitch() { std::cout << "Snitch(" << value << ") destroyed" << std::endl; }
        int value;
    };

    optional<Snitch> y(1);
    y.reset();
    y = Snitch(2);
    y = 3; 

    
    return(0);
}