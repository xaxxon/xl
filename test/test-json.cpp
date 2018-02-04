
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "json.h"

using namespace xl;
using namespace xl::json;

TEST(json, CopyAndAssignmentTests) {
    Json j;
    Json j2(j);
    Json j3((Json()));

    j2 = j;
    j2 = std::move(j);
}

TEST(json, empty) {
    EXPECT_FALSE(Json("").is_valid());
}

TEST(json, invalid) {
    EXPECT_FALSE(Json("asdfasdf").is_valid());
    EXPECT_THROW(Json("asdfasdf").get_string(), JsonException);
    EXPECT_EQ(*Json("asdfasdf").get_string("alternate string"), "alternate string");

    EXPECT_FALSE(Json("\"asdfasdf").is_valid());
    EXPECT_THROW(Json("\"asdfasdf").get_string(), JsonException);

    EXPECT_FALSE(Json("asdfasdf\"").is_valid());
    EXPECT_THROW(Json("asdfasdf\"").get_string(), JsonException);

    EXPECT_FALSE(Json("{\"a\"; 4}").is_valid());
    EXPECT_THROW(Json("{\"a\"; 4}").get_string(), JsonException);
    EXPECT_FALSE(Json("[0, 1, 2, 3, 4").is_valid());

    EXPECT_FALSE(Json("{\"a\":5 \"b\":6}").is_valid());
    EXPECT_FALSE(Json("[1 2 3]").is_valid());
}


TEST(json, number) {

    EXPECT_EQ(*Json("1").get_number(), 1);
    EXPECT_EQ(*Json("1.0").get_number(), 1);
    EXPECT_EQ(*Json(".5").get_number(), 0.5);
    EXPECT_EQ(*Json("0.5").get_number(), 0.5);
    EXPECT_EQ(*Json("-.5").get_number(), -0.5);
    EXPECT_EQ(*Json("-0.5").get_number(), -0.5);
    EXPECT_EQ(*Json("-1").get_number(), -1);
    EXPECT_EQ(*Json("-1.0").get_number(), -1);
    EXPECT_EQ(*Json("-1.0e2").get_number(), -100);
    EXPECT_EQ(*Json("-1.0e+2").get_number(), -100);
    EXPECT_EQ(*Json("-1.0e-2").get_number(), -0.01);
    EXPECT_EQ(*Json("-1.0E2").get_number(), -100);
    EXPECT_EQ(*Json("-1.0E+2").get_number(), -100);
    EXPECT_EQ(*Json("-1.0E-2").get_number(), -0.01);
    EXPECT_FALSE(Json("\"asdf\"").get_number());
    EXPECT_FALSE(Json("true").get_number());
    EXPECT_FALSE(Json("[1,2,3]").get_number());
    EXPECT_FALSE(Json("{\"a\": 4}").get_number());
    EXPECT_EQ(*Json("{\"a\": 4}").get_number(5), 5);
}

TEST(json, boolean) {
    EXPECT_TRUE(*Json("true").get_boolean());
    EXPECT_FALSE(*Json("false").get_boolean());
    EXPECT_TRUE(Json("false").get_boolean().has_value());
    EXPECT_FALSE(Json("\"asdf\"").get_boolean().has_value());
    EXPECT_TRUE(*Json("\"asdf\"").get_boolean(true));
}

TEST(json, string) {
    EXPECT_EQ(*Json("\"string\"").get_string(), "string");
    EXPECT_FALSE(Json("5").get_string());
    EXPECT_FALSE(Json("\"string\"")["bogus"].get_string());
    EXPECT_EQ(*Json("\"string\"")["bogus"].get_string("alternate string"), "alternate string");

    // test escaped quotation marks in strings
    EXPECT_EQ(*Json("\"\\\"string\\\"\"").get_string(), "\"string\"");
    EXPECT_EQ(*Json("\"str'ing\"").get_string(), "str'ing");
    EXPECT_EQ(*Json("'string'").get_string(), "string");
    EXPECT_EQ(*Json("'str\'ing'").get_string(), "str'ing");
    EXPECT_EQ(*Json("'str\"ing'").get_string(), "str\"ing");
}


TEST(json, object) {
    {
        auto result = Json("{\"a\": 4}").get_object();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ((*result).size(), 1);
    }
    { // test escaped quotations in object keys
        auto result = Json("{\"\\\"a\": 4}").get_object();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ((*result).size(), 1);
        EXPECT_TRUE(result->find("\"a") != result->end());
    }

    {
        auto result = Json("{\"a\": 1, \"b\": 2, \"c\": 3}").get_object();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ((*result).size(), 3);
        EXPECT_EQ((*result)["a"].get_number(), 1);
        EXPECT_EQ((*result)["b"].get_number(), 2);
        EXPECT_EQ((*result)["c"].get_number(), 3);
    }
}


TEST(json, array) {
    {
        auto result = Json("[]").get_array();
        EXPECT_TRUE(result);
        EXPECT_EQ(result->size(), 0);
    }
    {
        auto result = Json("[0, 1, 2, 3, 4]").get_array();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ((*result).size(), 5);
        EXPECT_EQ((*result)[0].get_number(), 0);
        EXPECT_EQ((*result)[4].get_number(), 4);
    }
    {
        auto result = Json("[0, 1, 2, 3, 4, ]").get_array();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ((*result).size(), 5);
        EXPECT_EQ((*result)[0].get_number(), 0);
        EXPECT_EQ((*result)[4].get_number(), 4);

    }
}


TEST(Json, WalkingNonexistantElements) {
    EXPECT_FALSE(Json()["Foo"]);
    EXPECT_FALSE(Json()[100]);

    EXPECT_FALSE(Json()["Foo"][100]["Foo"][100]);

    EXPECT_EQ(*Json("{\"key\": 1}")["key"].get_number(), 1);
    EXPECT_FALSE(Json("{\"key\": 1}")["not_key"].get_number());

    EXPECT_EQ(*Json("[0, 1, 2]")[1].get_number(), 1);
    EXPECT_FALSE(Json("[0, 1, 2]")[3].get_number());

}

TEST(Json, Comments) {
    EXPECT_EQ(Json("//test\n4").get_number(), 4);
    EXPECT_EQ((*Json("[1\n// test\n]//another test\n").get_array())[0].get_number(), 1);
    EXPECT_EQ((*Json("// foo\n"
                       "{ // test\n"
                       "// test\n"
                       "\"4\": 4 // test\n"
                       "// test\n"
                       "} // test\n"
                       "// test").get_object())["4"].get_number(), 4);
}

