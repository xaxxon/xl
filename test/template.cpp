
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "template.h"

using namespace xl;
using namespace std;


TEST(template, EmptyTemplate) {
    EXPECT_EQ(Template("").fill(), "");
}
TEST(template, NoSubstitutionTemplate) {
    auto template_string = "there are no substitutions here";
    EXPECT_EQ(Template(template_string).fill(), template_string);
}
TEST(template, SimpleSubstitutionTemplate) {
    EXPECT_EQ(Template("replace: {TEST}").fill(MapProvider(xl::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST}").fill(MapProvider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {TEST }").fill(MapProvider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: { TEST }").fill(MapProvider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
}
TEST(template, MissingNameInProviderSubstitutionTemplate) {
    EXPECT_THROW(Template("replace: {TEST}").fill(MapProvider{xl::pair{"XXX", "REPLACEMENT"}}),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_OpenedButNotClosed_Template) {
    EXPECT_THROW(Template("replace: {TEST").fill(),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_ClosedButNotOpened_Template) {
    EXPECT_THROW(Template("replace: TEST}").fill(),
                 xl::TemplateException);
}

TEST(template, SimpleSubstitutionTemplateWithSuffix) {
    EXPECT_EQ(Template("replace: {TEST} and more").fill(MapProvider{xl::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST} and: {TEST}").fill(MapProvider{xl::pair{"TEST", "REPLACEMENT"}}),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {TEST1} and: {TEST2}").fill(MapProvider{std::pair{"TEST1", "REPLACEMENT1"},std::pair{"TEST2", "REPLACEMENT2"}}),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}
TEST(template, CallbackSubstitutionTemplate) {
    MapProvider m{std::pair{"TEST", std::function<std::string()>([](){return std::string("REPLACEMENT-CALLBACK");})}};
    EXPECT_EQ(Template("replace: {TEST}").fill(m),
              "replace: REPLACEMENT-CALLBACK");
}


struct A {
    int i;

    A(int i) : i(i) {}

    std::unique_ptr<xl::Provider_Interface> get_provider() const {
        return std::make_unique<xl::MapProvider>(std::pair{"I", [this]{return fmt::format("{}", this->i);}}, std::pair{"J", "6"});
    }
};


struct B {
    std::string name = "B name";
    std::vector<A> vec_a{1,2,3,4,5};

    std::vector<A> const & get_vec_a(){return this->vec_a;};
    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return std::make_unique<xl::MapProvider>(std::pair{"NAME", this->name},
                                                 std::pair("GET_VEC_A", this->get_vec_a()));
    }
};

static_assert(xl::is_range_for_loop_able_v<vector<int>>);


TEST(template, UserDefinedTypeArray) {
    B b;

    TemplateMap templates{std::pair{"A1", Template("i: {I} j: {J}")},
                          std::pair{"A2", Template("i2: {I} j2: {J}")}};

    auto fill_result = Template("B: '{NAME}' A1: {GET_VEC_A|A1|, } A2: {GET_VEC_A|A2|, }").fill(b, templates);
    EXPECT_EQ(fill_result, "B: 'B name' A1: i: 1 j: 6, i: 2 j: 6, i: 3 j: 6, i: 4 j: 6, i: 5 j: 6 A2: i2: 1 j2: 6, i2: 2 j2: 6, i2: 3 j2: 6, i2: 4 j2: 6, i2: 5 j2: 6");
}
























struct Finger {
    enum class Name{Thumb = 0, Pointer, Middle, Ring, Pinky};
    static inline std::vector<std::string> names{"thumb", "pointer", "middle", "ring", "pinky"};

    Name name;
    Finger(Name name) : name(name) {}
    std::string & get_name(){
        return Finger::names[(int)this->name];
    }

    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return std::make_unique<xl::MapProvider>(std::pair{"NAME", this->get_name()});
    }
};


struct Hand {
private:
    std::vector<Finger> fingers{Finger::Name::Thumb, Finger::Name::Pointer, Finger::Name::Middle, Finger::Name::Ring, Finger::Name::Pinky};

public:
    auto get_finger_count(){return fingers.size();}
    auto const & get_fingers() {return this->fingers;}

    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return std::make_unique<xl::MapProvider>(std::pair{"FINGERS", "BOGUS"});
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
    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return std::make_unique<xl::MapProvider>(std::pair{"HANDS","BOGUS"});
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

    std::unique_ptr<xl::Provider_Interface> get_provider() {
        std::cerr << fmt::format("Getting Person provider") << std::endl;
        return std::make_unique<xl::MapProvider>(
                std::pair("NAME", this->name)//,
            );
    }

};



