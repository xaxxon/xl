
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "template.h"

using namespace xl;


TEST(template, EmptyTemplate) {
    EXPECT_EQ(Template("").fill(Provider()), "");
}
TEST(template, NoSubstitutionTemplate) {
    auto template_string = "there are no substitutions here";
    EXPECT_EQ(Template(template_string).fill(Provider()), template_string);
}
TEST(template, SimpleSubstitutionTemplate) {
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider(Provider::MapT{{"TEST", "REPLACEMENT"}})), "replace: REPLACEMENT");
}TEST(template, SimpleSubstitutionTemplateWithSuffix) {
    EXPECT_EQ(Template("replace: {TEST} and more").fill(Provider(Provider::MapT{{"TEST", "REPLACEMENT"}})), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST} and: {TEST}").fill(Provider(Provider::MapT{{"TEST", "REPLACEMENT"}})),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST1} and: {TEST2}").fill(Provider(Provider::MapT{{"TEST1", "REPLACEMENT1"},{"TEST2", "REPLACEMENT2"}})),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}
TEST(template, CallbackSubstitutionTemplate) {
    Provider::MapT m{{"TEST", [](){return "REPLACEMENT-CALLBACK";}}};
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider(std::move(m))),
              "replace: REPLACEMENT-CALLBACK");
}



