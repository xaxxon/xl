
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "template.h"

using namespace xl;
using namespace std;


TEST(template, EmptyTemplate) {
    EXPECT_EQ(Template("").fill(Provider()), "");
}
TEST(template, NoSubstitutionTemplate) {
    auto template_string = "there are no substitutions here";
    EXPECT_EQ(Template(template_string).fill(Provider()), template_string);
}
TEST(template, SimpleSubstitutionTemplate) {
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider{{{"TEST", "REPLACEMENT"}}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST}").fill(Provider{{{"TEST", "REPLACEMENT"}}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {TEST }").fill(Provider{{{"TEST", "REPLACEMENT"}}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST }").fill(Provider{{{"TEST", "REPLACEMENT"}}}), "replace: REPLACEMENT");

}
TEST(template, MissingNameInProviderSubstitutionTemplate) {
    EXPECT_THROW(Template("replace: {TEST}").fill(Provider{{{"XXX", "REPLACEMENT"}}}),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_OpenedButNotClosed_Template) {
    EXPECT_THROW(Template("replace: {TEST").fill(Provider()),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_ClosedButNotOpened_Template) {
    EXPECT_THROW(Template("replace: TEST}").fill(Provider()),
                 xl::TemplateException);
}

TEST(template, SimpleSubstitutionTemplateWithSuffix) {
    EXPECT_EQ(Template("replace: {TEST} and more").fill(Provider{{{"TEST", "REPLACEMENT"}}}), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST} and: {TEST}").fill(Provider{{{"TEST", "REPLACEMENT"}}}),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST1} and: {TEST2}").fill(Provider{{{"TEST1", "REPLACEMENT1"},{"TEST2", "REPLACEMENT2"}}}),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}
TEST(template, CallbackSubstitutionTemplate) {
    Provider m{{{"TEST", std::function<std::string()>([](){return std::string("REPLACEMENT-CALLBACK");})}}};
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider(std::move(m))),
              "replace: REPLACEMENT-CALLBACK");
}
TEST(template, SubTemplateSubstitutionTemplate) {
//    Provider p{Provider::MapT{{"TEST", Stringable(Template("{INNER-TEST}"))}, {"INNER-TEST", Stringable("INNER-REPLACEMENT")}}};
    Provider p{Provider::MapT{{{"TEST", Stringable(Template("{INNER-TEST}"))}, {"INNER-TEST", "INNER-REPLACEMENT"}}}};
    EXPECT_EQ(Template("replace: {TEST}").fill(std::move(p)), "replace: INNER-REPLACEMENT");
}

TEST(template, ArraySubstitution) {
    Provider p(Provider::MapT{{std::string("TEST"), Stringable(std::vector<std::string>{"test1", "test2", "test3"})}});
    EXPECT_EQ(Template("replace: {TEST:}").fill(std::move(p)), "replace: test1, test2, test3");
}


// Not supported yet
//TEST(template, ArraySubstitutionWithCustomSeparator) {
//    Provider p(Provider::MapT{{std::string("TEST"), Stringable(std::vector<std::string>{"test1", "test2", "test3"})}});
//    EXPECT_EQ(Template("replace: {TEST:,,}").fill(std::move(p)), "replace: test1,,test2,,test3");
//}



struct Finger {
    enum class Name{Thumb = 0, Pointer, Middle, Ring, Pinky};
    static inline std::vector<std::string> names{"thumb", "pointer", "middle", "ring", "pinky"};

    Name name;
    Finger(Name name) : name(name) {}
    std::string & get_name(){
        return Finger::names[(int)this->name];
    }

};

struct Hand {
private:
    std::vector<Finger> fingers{Finger::Name::Thumb, Finger::Name::Pointer, Finger::Name::Middle, Finger::Name::Ring, Finger::Name::Pinky};

public:
    auto get_finger_count(){return fingers.size();}
    auto const & get_fingers() {return this->fingers;}

    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>(Provider::MapT{{"finger_count", Stringable([this]{return fmt::format("{}", this->get_finger_count());})}});
    }
};

struct Arm {
    enum class Side{Left = 0, Right};
private:
    Side side;
    vector<string> const side_names{"left", "right"};
public:
    Arm(Side side) : side(side) {}
    Hand get_hand() const {return Hand();}
};


struct Person {
private:
    std::vector<Arm> arms{Arm(Arm::Side::Left), Arm(Arm::Side::Right)};
    string name;
public:
    Person(string const & name) : name(name) {}

    string const & get_name() const {return this->name;}
    std::vector<Arm> const & get_arms() const {return this->arms;}
    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>();
    }

};

//
//TEST(template, MakeProviderForUserDefinedType) {
//    Person p{"Joe"};
//    EXPECT_EQ(Template(R"(
//Person {{
//    name: {NAME},
//    Arms: {ARMS:,}
//}})").fill(p), "5");
//}


