#include <iostream>
#include "utils/optional.h"
#include <cassert>
#include "Application.h"

void impromptu_optional_unit_test();

int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    impromptu_optional_unit_test();

    return(0);
}

struct Snitch{
    Snitch(int x) : value(x) { std::cout << "Snitch(" << x << ") constructed" << std::endl; }
    ~Snitch() { std::cout << "Snitch(" << value << ") destroyed" << std::endl; }
    int value;

    friend bool operator==(const Snitch& lhs, const Snitch& rhs) {
        return(lhs.value == rhs.value);
    }

    friend bool operator!=(const Snitch& lhs, const Snitch& rhs) {return(!operator==(lhs,rhs));}
};

void impromptu_optional_unit_test() {
    opt::optional<int> x;
    x = 5;
    assert(x.has_value());
    assert(x.value() == 5);
    x.reset();
    assert(!x.has_value());
    try{
        x.value();
        assert(false);
    }catch(opt::bad_optional_access& e){
        printf("Exception caught (this is a good thing)\n");
    }

    opt::optional<Snitch> y(1);
    y.reset();
    y = Snitch(2);
    y = 3; 

    assert(Snitch(3) != Snitch(2));

    assert(opt::optional<Snitch>(3) == y);
    assert(opt::optional<Snitch>(2) != y);
    assert(y == 3); 
}