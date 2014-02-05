#include "gtest/gtest.h"

#include "fastjson.hpp"

#if _MSC_VER
#pragma warning (disable : 4611) // interation between '_setjmp' and C++ object destruction is non-portable
#endif

using namespace fastjson;

static jmp_buf env;

template<class Ch = char> class test_parse_failure
{
    const char* what;
    void* where;

    struct error_handler
    {
        test_parse_failure<Ch>* owner;
        error_handler(test_parse_failure<Ch>* owner_ = 0) : owner(owner_) {}

        void operator () (const char* what_, void* where_)
        {
            owner->what = what_;
            owner->where = where_;
            longjmp(env, 1);
        }
    };

    json_document<Ch, error_handler> doc;

public:
    template<int Flags> void test(const Ch* data, const char* errorString, int offset, bool expectSuccess = false)
    {
        Ch buffer[1024];
        const Ch* inp = data;
        Ch* outp = buffer;
        // Make a copy
        while (*inp) *outp++ = *inp++;
        *outp = Ch(0);

        error_handler handler(this);
        if (::setjmp(env) == 0) // "try"
        {
            doc.parse<Flags>(buffer, sizeof(Ch)*(outp - buffer), json_document<Ch, error_handler>::unknown, handler);
            EXPECT_TRUE(expectSuccess) << "Parse succeeded unexpectedly for text: " << data;
        }
        else // "catch"
        {
            EXPECT_FALSE(expectSuccess) << "Parse failed unexpectedly for text: " << data;
            EXPECT_EQ(offset, (Ch*)where - buffer) << "For error (" << errorString << ") and text: " << data;
            EXPECT_STREQ(errorString, what);
        }
    }
};

TEST(fastjson_no_except, parser)
{
    test_parse_failure<> tester;
    // Arrays
    tester.test<0>("", "Expected '{' or '['", 0);
    tester.test<0>(" ", "Expected '{' or '['", 1);
    tester.test<0>(" [ ", "Expected value", 3);
    tester.test<0>(" [\n] ", "", 0, true);
    tester.test<0>(" [ \"", "Expected end-of-string '\"'", 4);
    tester.test<0>(" [ \"\"", "Expected value-separator ',' or end-of-array ']'", 5);
    tester.test<0>(" [ \"\"   \t \n", "Expected value-separator ',' or end-of-array ']'", 11);
    tester.test<0>(" [ 0,     \t", "Expected value", 11);
    tester.test<0>(" [ 0, ] ", "Expected value", 6);
    tester.test<0>(" [\t\n[\t\n]\t\n] ", "", 0, true);
    tester.test<0>(" [[[[[[[[[[[[[]]]]]]]]]]]]] ", "", 0, true);
    tester.test<0>(" [ [], [], [], [], [  ], [], [], [], [] ] \t\n", "", 0, true);
    tester.test<0>(" [] [] ", "Expected end of document", 4);
    // Value parsing
    tester.test<0>(" [ t ]", "Expected value", 3);
    tester.test<0>(" [ true ] ", "", 0, true);
    tester.test<0>(" [ TRUE ] ", "Expected value", 3);
    tester.test<0>(" [ fal ]", "Expected value", 3);
    tester.test<0>(" [ false ] ", "", 0, true);
    tester.test<0>(" [ FALSE ] ", "Expected value", 3);
    tester.test<0>(" [ n ] ", "Expected value", 3);
    tester.test<0>(" [ null ] ", "", 0, true);
    tester.test<0>(" [ NULL ] ", "Expected value", 3);
    // Number parsing
    tester.test<0>(" [ Inf ] ", "Expected value", 3);
    tester.test<0>(" [ -Inf ] ", "Expected digit", 4);
    tester.test<0>(" [ NaN ] ", "Expected value", 3);
    tester.test<0>(" [ 0", "Expected value-separator ',' or end-of-array ']'", 4);
    tester.test<0>(" [ -0", "Expected value-separator ',' or end-of-array ']'", 5);
    tester.test<0>(" [ 0 ] ", "", 0, true);
    tester.test<0>(" [ -0 ] ", "", 0, true);
    tester.test<0>(" [ 01 ] ", "Expected value-separator ',' or end-of-array ']'", 4);
    tester.test<0>(" [ 01.123 ] ", "Expected value-separator ',' or end-of-array ']'", 4);
    tester.test<0>(" [ .132 ] ", "Expected digit", 3);
    tester.test<0>(" [ -.123 ] ", "Expected digit", 4);
    tester.test<0>(" [ 123", "Expected value-separator ',' or end-of-array ']'", 6);
    tester.test<0>(" [ -123", "Expected value-separator ',' or end-of-array ']'", 7);
    tester.test<0>(" [ 123 ] ", "", 0, true);
    tester.test<0>(" [ -123 ] ", "", 0, true);
    tester.test<0>(" [ - 123 ] ", "Expected digit", 4);
    tester.test<0>(" [ 123d ] ", "Expected value-separator ',' or end-of-array ']'", 6);
    tester.test<0>(" [ 123.", "Expected fractional digits", 7);
    tester.test<0>(" [ 123. ] ", "Expected fractional digits", 7);
    tester.test<0>(" [ -123.", "Expected fractional digits", 8);
    tester.test<0>(" [ -123. ] ", "Expected fractional digits", 8);
    tester.test<0>(" [ 0.", "Expected fractional digits", 5);
    tester.test<0>(" [ -0.", "Expected fractional digits", 6);
    tester.test<0>(" [ 0. ]", "Expected fractional digits", 5);
    tester.test<0>(" [ -0. ]", "Expected fractional digits", 6);
    tester.test<0>(" [ 0.0 ] ", "", 0, true);
    tester.test<0>(" [ -0.0 ] ", "", 0, true);
    tester.test<0>(" [ 123e", "Expected exponent digits", 7);
    tester.test<0>(" [ 123e+", "Expected exponent digits", 8);
    tester.test<0>(" [ 123e-", "Expected exponent digits", 8);
    tester.test<0>(" [ -123e+", "Expected exponent digits", 9);
    tester.test<0>(" [ -123e-", "Expected exponent digits", 9);
    tester.test<0>(" [ 123E", "Expected exponent digits", 7);
    tester.test<0>(" [ 123E+", "Expected exponent digits", 8);
    tester.test<0>(" [ 123E-", "Expected exponent digits", 8);
    tester.test<0>(" [ -123E+", "Expected exponent digits", 9);
    tester.test<0>(" [ -123E-", "Expected exponent digits", 9);
    tester.test<0>(" [ 123e0", "Expected value-separator ',' or end-of-array ']'", 8);
    tester.test<0>(" [ 123e+0", "Expected value-separator ',' or end-of-array ']'", 9);
    tester.test<0>(" [ 123e-0", "Expected value-separator ',' or end-of-array ']'", 9);
    tester.test<0>(" [ 123e0 ] ", "", 0, true);
    tester.test<0>(" [ 123e+0 ] ", "", 0, true);
    tester.test<0>(" [ 123e-0 ] ", "", 0, true);
    tester.test<0>(" [ 123e0123 ] ", "", 0, true);
    tester.test<0>(" [ 123e+0123 ] ", "", 0, true);
    tester.test<0>(" [ 123e-0123 ] ", "", 0, true);
    tester.test<0>(" [ 123e0. ] ", "Expected value-separator ',' or end-of-array ']'", 8);
    tester.test<0>(" [ 123e+0. ] ", "Expected value-separator ',' or end-of-array ']'", 9);
    tester.test<0>(" [ 123e-0. ] ", "Expected value-separator ',' or end-of-array ']'", 9);
    // String parsing
    tester.test<0>(" [ \" ]", "Expected end-of-string '\"'", 6);
    tester.test<0>(" [ \"", "Expected end-of-string '\"'", 4);
    tester.test<0>(" [ \"\"\n", "Expected value-separator ',' or end-of-array ']'", 6);
    tester.test<0>(" [ \"\\", "Invalid escaped character", 5);
    tester.test<0>(" [ \"\\a", "Invalid escaped character", 5);
    tester.test<0>(" [ \"\\\"", "Expected end-of-string '\"'", 6);
    tester.test<0>(" [ \"abcdefghijklmnopqrstuvwxyz\\\"\\\\\\/\\b\\f\\n\\r\\t\\u0000\" ] ", "", 0, true);
    tester.test<0>(" [ \"\\u", "Invalid \\u escape sequence", 4);
    tester.test<0>(" [ \"\\u0", "Invalid \\u escape sequence", 4);
    tester.test<0>(" [ \"\\u00", "Invalid \\u escape sequence", 4);
    tester.test<0>(" [ \"\\u000", "Invalid \\u escape sequence", 4);
    tester.test<0>(" [ \"\\ud800", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\u", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\u0", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\u00", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\u000", "Expected UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\u0000", "Invalid UTF-16 surrogate pair", 10);
    tester.test<0>(" [ \"\\ud800\\udc00", "Expected end-of-string '\"'", 16);
    tester.test<0>(" [ \"\\ud800\\udc00\" ] ", "", 0, true);
    tester.test<0>(" [ \"\xc3\xa9\" ] ", "", 0, true); // UTF-8 encoded text

    // Objects
    tester.test<0>(" { ", "Expected end-of-object '}' or name (string)", 3);
    tester.test<0>(" { \"", "Expected end-of-string '\"'", 4);
    tester.test<0>(" { \"\" ", "Expected name separator (:)", 6);
    tester.test<0>(" { : ", "Expected end-of-object '}' or name (string)", 3);
    tester.test<0>(" { \"\" :\t", "Expected value", 8);
    tester.test<0>("\t{\t\"\"\t: t}", "Expected value", 8);
    tester.test<0>(" { \"\" : true } ", "", 0, true);
    tester.test<0>(" { \"\" : f}", "Expected value", 8);
    tester.test<0>(" { \"\" : false } ", "", 0, true);
    tester.test<0>(" { \"\" : n}", "Expected value", 8);
    tester.test<0>(" { \"\" : null } ", "", 0, true);
    tester.test<0>(" { \"\" : }", "Expected value", 8);
    tester.test<0>(" { \"\" : null,\t", "Expected name (string)", 14);
    tester.test<0>(" { \"\" : {", "Expected end-of-object '}' or name (string)", 9);
    tester.test<0>(" { \"\" : {\t} ", "Expected value-separator ',' or end-of-object '}'", 12);
    tester.test<0>(" { } { } ", "Expected end of document", 5);
    tester.test<0>(" { } [ ] ", "Expected end of document", 5);
}