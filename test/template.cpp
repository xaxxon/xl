
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
    EXPECT_EQ(Template("replace: {{TEST}}").fill(Provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ TEST}}").fill(Provider{std::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{TEST }}").fill(Provider{std::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ TEST }}").fill(Provider{std::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT");
}
TEST(template, EscapedCurlyBraceTemplate) {
    EXPECT_EQ(Template("replace: \\{{{TEST}}").fill(Provider(std::pair{"TEST", "REPLACEMENT"})), "replace: {REPLACEMENT");
    EXPECT_EQ(Template("replace: {{TEST}}\\}").fill(Provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT}");
}
TEST(template, MissingNameInProviderSubstitutionTemplate) {
    EXPECT_THROW(Template("replace: {{TEST}}").fill(Provider{std::pair{"XXX", "REPLACEMENT"}}),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_OpenedButNotClosed_Template) {
    EXPECT_THROW(Template("replace: {{TEST").fill(),
                 xl::TemplateException);
}
TEST(template, InvalidTemplateSyntax_ClosedButNotOpened_Template) {
    EXPECT_THROW(Template("replace: TEST}}").fill(),
                 xl::TemplateException);
}

TEST(template, SimpleSubstitutionTemplateWithSuffix) {
    EXPECT_EQ(Template("replace: {{TEST}} and more").fill(Provider{std::pair{"TEST", "REPLACEMENT"}}), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {{TEST}} and: {{TEST}}").fill(Provider{std::pair{"TEST", "REPLACEMENT"}}),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {{TEST1}} and: {{TEST2}}").fill(Provider{std::pair{"TEST1", "REPLACEMENT1"},std::pair{"TEST2", "REPLACEMENT2"}}),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}
TEST(template, CallbackSubstitutionTemplate) {
    Provider m{std::pair{"TEST", std::function<std::string()>([](){return std::string("REPLACEMENT-CALLBACK");})}};
    EXPECT_EQ(Template("replace: {{TEST}}").fill(m),
              "replace: REPLACEMENT-CALLBACK");
}



struct A {
    int i;

    A(int i) : i(i) {}

};



std::unique_ptr<xl::Provider_Interface> get_provider(A const & a) {
    return std::unique_ptr<xl::Provider_Interface>(new Provider(std::pair{"I", [a]{return fmt::format("{}", a.i);}}, std::pair{"J", "6"}));
}

static_assert(can_get_provider_for_type_v<A>);
struct B {
    std::string name = "B name";
    std::vector<A> vec_a{1,2,3,4,5};

    std::vector<A> const & get_vec_a(){return this->vec_a;};
    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return make_provider(std::pair{"NAME", this->name},
                             std::pair("GET_VEC_A", std::bind(&B::get_vec_a, this)));
    }
};
static_assert(can_get_provider_for_type_v<B>);

static_assert(xl::is_range_for_loop_able_v<vector<int>>);


TEST(template, UserDefinedTypeArray) {
    B b;

    TemplateMap templates{std::pair{"A1", Template("{i: {{I}} j: {{J}}}")},
                          std::pair{"A2", Template("{i2: {{I}} j2: {{J}}}")}};

    auto fill_result = Template("B: '{{NAME}}' A1: {{GET_VEC_A|A1|, }} A2: {{GET_VEC_A|A2|, }}").fill(b, templates);
    EXPECT_EQ(fill_result, "B: 'B name' A1: {i: 1 j: 6}, {i: 2 j: 6}, {i: 3 j: 6}, {i: 4 j: 6}, {i: 5 j: 6} A2: {i2: 1 j2: 6}, {i2: 2 j2: 6}, {i2: 3 j2: 6}, {i2: 4 j2: 6}, {i2: 5 j2: 6}");
}



vector<A> vector_object_callback() {
    return {10, 11, 12};
}
TEST(template, VectorCallbackTemplate) {
    TemplateMap templates{std::pair{"A1", Template("{i: {{I}} j: {{J}}}")},
                          std::pair{"A2", Template("{i2: {{I}} j2: {{J}}}")}};

    EXPECT_EQ(Template("replace: {{TEST1|A1|, }}").fill(Provider{std::pair{"TEST1", make_provider(vector_object_callback)}}, templates),
    "replace: {i: 10 j: 6}, {i: 11 j: 6}, {i: 12 j: 6}");
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
        return make_provider(std::pair{"NAME", this->get_name()});
    }
};


struct Hand {
private:
    std::vector<Finger> fingers{Finger::Name::Thumb, Finger::Name::Pointer, Finger::Name::Middle, Finger::Name::Ring, Finger::Name::Pinky};

public:
    auto get_finger_count(){return fingers.size();}
    auto const & get_fingers() {return this->fingers;}

    std::unique_ptr<xl::Provider_Interface> get_provider() {
        return make_provider(std::pair{"FINGERS", "BOGUS"});
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
        return make_provider(std::pair{"HANDS","BOGUS"});
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
        return make_provider(
                std::pair("NAME", this->name)//,
            );
    }

};



TEST(template, InlineSubtemplate) {
    Template t(R"(
This is a normal template {{SUBSTITUTE}}
{{VECTOR|!!
 * This is \{{{SUPERLATIVE}}\} inline template{{EXCLAMATION}}}}
This is more normal template
)");


    vector<std::map<std::string, std::string>> vector_provider{
        {std::pair("SUPERLATIVE"s, "an awesome"s), std::pair("EXCLAMATION"s, "!"s)},
        {{std::pair("SUPERLATIVE", "a cool"), std::pair("EXCLAMATION", "!!")}},
        {{std::pair("SUPERLATIVE", "a super"), std::pair("EXCLAMATION", "!!!")}}};

    auto result = t.fill(make_provider(
        std::pair("SUBSTITUTE", "REPLACEMENT"),
        std::make_pair("VECTOR", std::move(vector_provider)) // this creates a copy of vector_provider
    ));

    std::string expected_result = R"(
This is a normal template REPLACEMENT
 * This is {an awesome} inline template!
 * This is {a cool} inline template!!
 * This is {a super} inline template!!!
This is more normal template
)";

    EXPECT_EQ(result, expected_result);
}



TEST(template, LoadDirectoryOfTemplates) {
    auto templates = load_templates("templates");

    EXPECT_EQ(templates.size(), 2);
    EXPECT_NE(templates.find("a"), templates.end());
    EXPECT_NE(templates.find("b"), templates.end());

    EXPECT_EQ(templates["a"].fill(),"a.template contents");
    EXPECT_EQ(templates["b"].fill(),"b.template contents");

    templates = load_templates("templates/a.template");
    EXPECT_EQ(templates.size(), 1);
    EXPECT_NE(templates.find("a"), templates.end());
    EXPECT_EQ(templates.find("b"), templates.end());
    EXPECT_EQ(templates["a"].fill(),"a.template contents");


}
