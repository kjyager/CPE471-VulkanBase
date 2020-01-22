#include "catch.hpp"
#include "utils/optional.h"
#include <iostream>
#include <unordered_map>
#include <memory>


struct Snitch{
    Snitch(int x) : value(x) {
        // snitch = std::make_shared<int>(x);
    }
    
    int value;
    // std::shared_ptr<int> snitch = nullptr; // Will 'snitch' by not automatically destroying if optional doesn't call destructors properly. 

    friend bool operator==(const Snitch& lhs, const Snitch& rhs) {
        return(lhs.value == rhs.value);
    }

    friend bool operator!=(const Snitch& lhs, const Snitch& rhs) {return(!operator==(lhs,rhs));}
};

TEST_CASE("std::optional replica basic tests"){

    SECTION(""){
        {
            opt::optional<int> x;
            x = 5;
            REQUIRE(x.has_value());
            REQUIRE(x.value() == 5);
            x.reset();
            REQUIRE(!x.has_value());


            REQUIRE_THROWS(x.value());

            opt::optional<Snitch> y(1);
            y.reset();
            y = Snitch(2);
            y = 3; 

            REQUIRE(Snitch(3) != Snitch(2));

            REQUIRE(opt::optional<Snitch>(3) == y);
            REQUIRE(opt::optional<Snitch>(2) != y);
            REQUIRE(y == 3); 
        }
    }
}