
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
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider(xl::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST}").fill(Provider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {TEST }").fill(Provider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST }").fill(Provider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
}
TEST(template, MissingNameInProviderSubstitutionTemplate) {
    EXPECT_THROW(Template("replace: {TEST}").fill(Provider{xl::pair{"XXX", "REPLACEMENT"}}),
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
    EXPECT_EQ(Template("replace: {TEST} and more").fill(Provider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST} and: {TEST}").fill(Provider{xl::pair{"TEST", "REPLACEMENT"}}),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST1} and: {TEST2}").fill(Provider{std::pair{"TEST1", "REPLACEMENT1"},std::pair{"TEST2", "REPLACEMENT2"}}),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}
TEST(template, CallbackSubstitutionTemplate) {
    Provider m{std::pair{"TEST", std::function<std::string()>([](){return std::string("REPLACEMENT-CALLBACK");})}};
    EXPECT_EQ(Template("replace: {TEST}").fill(Provider(std::move(m))),
              "replace: REPLACEMENT-CALLBACK");
}



struct A {
    int i;

    A(int i) : i(i) {}

    std::unique_ptr<xl::Provider> get_provider() const {
        return std::make_unique<xl::Provider>(std::pair("asdf", "asdf"));
    }
};

struct B {
    std::string name = "B name";
    std::vector<A> vec_a{1,2,3,4,5};

    std::vector<A> const & get_vec_a(){return this->vec_a;};
    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>(std::pair{"NAME", this->name},
                                          std::pair("GET_VEC_A", BoundTemplate(Template("{GET_VEC_A}"), static_cast<BoundTemplate::vector<A> const &>(this->get_vec_a()))));
    }
};


TEST(template, UserDefinedTypeArray) {
    B b;
    EXPECT_EQ(BoundTemplate(Template("B: {B_NAME}  As: {GET_VEC_A}"), b)(), "THIS IS WRONG");
}
























struct Finger {
    enum class Name{Thumb = 0, Pointer, Middle, Ring, Pinky};
    static inline std::vector<std::string> names{"thumb", "pointer", "middle", "ring", "pinky"};

    Name name;
    Finger(Name name) : name(name) {}
    std::string & get_name(){
        return Finger::names[(int)this->name];
    }
    static Template get_template() {
        return Template("Finger: Name: {NAME}");
    }
    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>(std::pair{"NAME", this->get_name()});
    }
};

struct Hand {
private:
    std::vector<Finger> fingers{Finger::Name::Thumb, Finger::Name::Pointer, Finger::Name::Middle, Finger::Name::Ring, Finger::Name::Pinky};

public:
    auto get_finger_count(){return fingers.size();}
    auto const & get_fingers() {return this->fingers;}

    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>(std::pair{"FINGERS", "BOGUS"});
    }

    static Template get_template() {
        return Template("Hand: Fingers: {FINGERS:}");
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
    static Template get_template() {
        return Template("Arm: Hands: {HANDS:}");
    }
    std::unique_ptr<xl::Provider> get_provider() {
        return std::make_unique<xl::Provider>(std::pair{"HANDS","BOGUS"});
    }
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
        std::cerr << fmt::format("Getting Person provider") << std::endl;
        return std::make_unique<xl::Provider>(
                std::pair("NAME", this->name)//,
                //std::pair("ARMS", Arm::get_template())
            );
    }

};


TEST(template, MakeProviderForUserDefinedType) {
    Person p{"Joe"};
//    EXPECT_EQ(Template(R"(
//Person {{
//    name: {NAME},
//    Arms: {ARMS:,}
//}})").fill(p), "5");
}


