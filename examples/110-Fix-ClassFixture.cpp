// 110-Fix-ClassFixture.cpp

// Catch has two ways to express fixtures:
// - Sections
// - Traditional class-based fixtures (this file)

// Let Catch provide main():
#define CATCH_CONFIG_MAIN

#include "catch.hpp"

class DBConnection
{
public:
    static DBConnection createConnection( std::string const & /*dbName*/ ) {
        return DBConnection();
    }

    bool executeSQL( std::string const & /*query*/, int const /*id*/, std::string const & arg ) {
        if ( arg.length() == 0 ) {
            throw std::logic_error("empty SQL query argument");
        }
        return true; // ok
    }
};

class UniqueTestsFixture
{
private:
    static int uniqueID;

protected:
    DBConnection conn;

public:
    UniqueTestsFixture()
    : conn( DBConnection::createConnection( "myDB" ) )
    {}

protected:
    int getID() {
        return ++uniqueID;
    }
};

int UniqueTestsFixture::uniqueID = 0;

TEST_CASE_METHOD( UniqueTestsFixture, "Create Employee/No Name", "[create]" ) {
    REQUIRE_THROWS( conn.executeSQL( "INSERT INTO employee (id, name) VALUES (?, ?)", getID(), "") );
}

TEST_CASE_METHOD( UniqueTestsFixture, "Create Employee/Normal", "[create]" ) {
    REQUIRE( conn.executeSQL( "INSERT INTO employee (id, name) VALUES (?, ?)", getID(), "Joe Bloggs" ) );
}

// Compile & run:
// - g++ -std=c++11 -Wall -I$(CATCH_SINGLE_INCLUDE) -o 110-Fix-ClassFixture 110-Fix-ClassFixture.cpp && 110-Fix-ClassFixture --success
// - cl -EHsc -I%CATCH_SINGLE_INCLUDE% 110-Fix-ClassFixture.cpp && 110-Fix-ClassFixture --success

// Expected compact output (all assertions):
//
// prompt> 110-Fix-ClassFixture.exe --reporter compact --success
// 110-Fix-ClassFixture.cpp:49: passed: conn.executeSQL( "INSERT INTO employee (id, name) VALUES (?, ?)", getID(), "")
// 110-Fix-ClassFixture.cpp:53: passed: conn.executeSQL( "INSERT INTO employee (id, name) VALUES (?, ?)", getID(), "Joe Bloggs" ) for: true
// Passed both 2 test cases with 2 assertions.
