
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "json.h"

using namespace xl;
using namespace xl::json;

TEST(json, empty) {
    EXPECT_FALSE(Json("").is_valid());
}

TEST(json, invalid) {
    EXPECT_FALSE(Json("asdfasdf").is_valid());
    EXPECT_FALSE(Json("\"asdfasdf").is_valid());
    EXPECT_FALSE(Json("asdfasdf\"").is_valid());
    EXPECT_FALSE(Json("{\"a\"; 4}").is_valid());
    EXPECT_FALSE(Json("[0, 1, 2, 3, 4").is_valid());
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
}

TEST(json, boolean) {
    EXPECT_TRUE(*Json("true").get_boolean());
    EXPECT_FALSE(*Json("false").get_boolean());
    EXPECT_TRUE(Json("false").get_boolean().has_value());
    EXPECT_FALSE(Json("\"asdf\"").get_boolean().has_value());
}

TEST(json, string) {
    EXPECT_EQ(*Json("\"string\"").get_string(), "string");

    // test escaped quotation marks in strings
    EXPECT_EQ(*Json("\"\\\"string\\\"\"").get_string(), "\"string\"");
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
    EXPECT_TRUE(Json().get_object_by_key("Foo").empty());
    EXPECT_FALSE(Json().get_array_index(100).is_valid());

    EXPECT_FALSE(Json().
        get_object_by_key("Foo")["bar"].get_array_index(100).
        get_object_by_key("Foo")["bar"].get_array_index(100).is_valid());

    EXPECT_EQ(*Json("{\"key\": 1}").as_object()["key"].get_number(), 1);
    EXPECT_FALSE(Json("{\"key\": 1}").get_object_by_key("not_key")["some_key"].get_number());

    EXPECT_EQ(*Json("[0, 1, 2]").get_array_index(1).get_number(), 1);
    EXPECT_FALSE(Json("[0, 1, 2]").get_array_index(3).get_number());

}

