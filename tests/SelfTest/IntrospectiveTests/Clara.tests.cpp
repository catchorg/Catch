//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
    
#include <catch2/catch_test_macros.hpp>
#include <catch2/internal/catch_clara.hpp>


#include <string>

TEST_CASE("Clara::Arg supports single-arg parse the way Opt does", "[clara][arg][compilation]") {
    std::string name;
    auto p = Catch::Clara::Arg(name, "just one arg");
    
    CHECK(name.empty());
    
    p.parse( Catch::Clara::Args{ "UnitTest", "foo" } );
    REQUIRE(name == "foo");
}

TEST_CASE("Clara::Opt supports accept-many lambdas", "[clara][opt]") {
    std::string name;
    using namespace Catch::Clara;
    std::vector<std::string> res;
    const auto push_to_res = [&](std::string const& s) {
        res.push_back(s);
        return ParserResult::ok();
    };

    SECTION("Parsing fails on multiple options without accept_many") {
        auto p = Parser() | Opt(push_to_res, "value")["-o"];
        auto parse_result = p.parse( Args{ "UnitTest", "-o", "aaa", "-o", "bbb" } );
        CHECK_FALSE(parse_result);
    }
    SECTION("Parsing succeeds on multiple options with accept_many") {
        auto p = Parser() | Opt(accept_many, push_to_res, "value")["-o"];
        auto parse_result = p.parse( Args{ "UnitTest", "-o", "aaa", "-o", "bbb" } );
        CHECK(parse_result);
        CHECK(res == std::vector<std::string>{ "aaa", "bbb" });
    }
}
