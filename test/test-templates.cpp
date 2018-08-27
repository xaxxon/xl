
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "templates.h"
#include "templates/provider.h"
#include "library_extensions.h"

using namespace xl::templates;
using namespace std::string_literals;


TEST(template, VectorOfStrings) {
    EXPECT_EQ(Template("{{v}}").fill(make_provider(std::pair{"v", std::vector<std::string>{"a", "b", "c"}})), "a\nb\nc");
    EXPECT_EQ(Template("{{v|!{{}}}}").fill(make_provider(std::pair{"v", std::vector<std::string>{"a", "b", "c"}})), "a\nb\nc");
}

TEST(template, VectorOfMapOfStrings) {
    EXPECT_EQ(Template("{{v|!{{a}} {{b}}}}").fill(make_provider(std::pair{
        "v", 
        std::vector<std::map<std::string, std::string>>{
            {std::pair{"a"s, "a1"s}, std::pair{"b"s, "b1"s}},
            {std::pair{"a"s, "a2"s}, std::pair{"b"s, "b2"s}},
            {std::pair{"a"s, "a3"s}, std::pair{"b"s, "b3"s}}
        }
    })), "a1 b1\na2 b2\na3 b3");
}


TEST(template, EmptyTemplate) {
    EXPECT_EQ(Template("").fill(), "");
}
TEST(template, NoSubstitutionTemplate) {
    auto template_string = "there are no substitutions here";
    
    EXPECT_EQ(Template(template_string).fill(), template_string);
}
TEST(template, SimpleSubstitutionTemplate) {
    Template t1("replace: {{TEST}}");
    t1.compile();
    EXPECT_TRUE(t1.compiled_template);
    EXPECT_EQ(t1.compiled_template->static_strings.size(), 1);
    EXPECT_EQ(t1.compiled_template->static_strings.front(), "replace: ");
    EXPECT_EQ(t1.compiled_template->substitutions.size(), 1);
    EXPECT_EQ(t1.fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(t1.fill(make_provider(std::pair{"TEST", std::string("REPLACEMENT")})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ TEST}}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{TEST }}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ TEST }}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{NAME WITH SPACE}}").fill(make_provider(std::pair{"NAME WITH SPACE", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ NAME WITH SPACE}}").fill(make_provider(std::pair{"NAME WITH SPACE", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{NAME WITH SPACE }}").fill(make_provider(std::pair{"NAME WITH SPACE", "REPLACEMENT"})), "replace: REPLACEMENT");
    EXPECT_EQ(Template("replace: {{ NAME WITH SPACE }}").fill(make_provider(std::pair{"NAME WITH SPACE", "REPLACEMENT"})), "replace: REPLACEMENT");
}
TEST(template, Callback) {
    auto l = [](){return std::string("foo");};
//    DefaultProviders<void>::Provider<std::string> pstring("s");
//    static_assert(std::is_same_v<xl::remove_reference_wrapper_t<std::string>, std::string>);
//    static_assert(DefaultProviders<void>::is_provider_type_v<std::string>);
//    static_assert(DefaultProviders<void>::is_provider_callback_type_v<decltype(l)>);
    EXPECT_EQ(Template("{{a}}").fill(make_provider(std::pair{"a", l})), "foo");
}
TEST(template, EscapedCurlyBraceTemplate) {
    EXPECT_EQ(Template("replace: \\{{{TEST}}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: {REPLACEMENT");
    EXPECT_EQ(Template("replace: {{TEST}}\\}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT}");
}
TEST(template, MissingNameInProviderSubstitutionTemplate) {
    EXPECT_THROW(Template("replace: {{TEST}}").fill(make_provider(std::pair{"XXX", "REPLACEMENT"})),
                 xl::templates::TemplateException);
}
TEST(template, InvalidTemplateSyntax_OpenedButNotClosed_Template) {
    EXPECT_THROW(Template("replace: {{TEST").fill(),
                 xl::templates::TemplateException);
}
TEST(template, InvalidTemplateSyntax_ClosedButNotOpened_Template) {
    EXPECT_THROW(Template("replace: TEST}}").fill(),
                 xl::templates::TemplateException);
}
TEST(template, SimpleSubstitutionTemplateWithSuffix) {
    EXPECT_EQ(Template("replace: {{TEST}} and more").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})), "replace: REPLACEMENT and more");
}
TEST(template, MultipleSubstitutionsSameNameTemplate) {
    EXPECT_EQ(Template("replace: {{TEST}} and: {{TEST}}").fill(make_provider(std::pair{"TEST", "REPLACEMENT"})),
              "replace: REPLACEMENT and: REPLACEMENT");
}
TEST(template, MultipleSubstitutionsDifferentNameTemplate) {
    EXPECT_EQ(Template("replace: {{TEST1}} and: {{TEST2}}").fill(make_provider(std::pair{"TEST1", "REPLACEMENT1"},std::pair{"TEST2", "REPLACEMENT2"})),
              "replace: REPLACEMENT1 and: REPLACEMENT2" );
}


//TEST(template, CallbackSubstitutionTemplate) {
//    auto m = make_provider(std::pair{"TEST", std::function<std::string()>([](){return std::string("REPLACEMENT-CALLBACK");})});
//    EXPECT_EQ(Template("replace: {{TEST}}").fill(m),
//              "replace: REPLACEMENT-CALLBACK");
//
//    {
//        auto result = Template("{{set|!{{}}}}").fill(
//           make_provider(std::pair("set", [](){return std::set<string>{"a", "b", "c"};})));
//        EXPECT_EQ(result, "a\nb\nc");
//    }
//}


TEST(template, SingleLineIgnoreEmpty) {
    {
        auto result = Template("BEFORE {{name|!{{name}}}}").fill(make_provider(std::pair("name", "")));
        EXPECT_EQ(result, "BEFORE ");
    }
    {
        auto result = Template("BEFORE {{<name|!{{name}}}}").fill(make_provider(std::pair("name", "")));
        EXPECT_EQ(result, "");
    }
    {
        auto result = Template("BEFORE {{<name}} AFTER").fill(make_provider(std::pair("name", "")));
        EXPECT_EQ(result, " AFTER");
    }
    {
        auto result = Template("BEFORE {{name>}} AFTER").fill(make_provider(std::pair("name", "")));
        EXPECT_EQ(result, "BEFORE ");
    }
    {
        auto result = Template("BEFORE {{<name>}} AFTER").fill(make_provider(std::pair("name", "")));
        EXPECT_EQ(result, "");
    }
    {
        auto result = Template("BEFORE {{<name}} AFTER").fill(make_provider(std::pair("name", "X")));
        EXPECT_EQ(result, "BEFORE X AFTER");
    }
    {
        auto result = Template("BEFORE {{name>}} AFTER").fill(make_provider(std::pair("name", "X")));
        EXPECT_EQ(result, "BEFORE X AFTER");
    }
    {
        auto result = Template("BEFORE {{<name>}} AFTER").fill(make_provider(std::pair("name", "X")));
        EXPECT_EQ(result, "BEFORE X AFTER");
    }


    {
        auto result = Template("BEFORE {{<name}}").fill(make_provider(std::pair("name", "content")));
        EXPECT_EQ(result, "BEFORE content");
    }
}


static_assert(std::is_same_v<xl::remove_reference_wrapper_t<std::vector<std::string>>, std::vector<std::string>>);


TEST(template, EmptyVectorReplacementIgnored) {
    {
        std::vector<std::string> v{"a", "", "c"};
        auto result = Template("{{name|!{{}}}}").fill(make_provider(std::pair("name", v)));
        EXPECT_EQ(result, "a\n\nc");
    }
    {
        std::vector<std::string> v{"a", "", "c"};
        Template t("{{<name|!{{<dummyname}}}}");
        auto result = t.fill(make_provider(std::pair("name", v)));
        EXPECT_EQ(result, "a\nc");
    }
    {
        std::vector<std::string> v{"", "", ""};
        auto result = Template("{{<name|!{{<dummyname}}}}").fill(make_provider(std::pair("name", v)));
        EXPECT_EQ(result, "");
    }
    {
        std::vector<std::string> v{"", "", ""};
        auto result = Template("before\n{{<name|!{{<dummyname}}}}\nafter").fill(make_provider(std::pair("name", v)));
        EXPECT_EQ(result, "before\nafter");
    }
    {
        std::vector<std::string> v{"", "", ""};
        auto result = Template("before\n{{<name|!!\n{{<dummyname}}}}\nafter").fill(make_provider(std::pair("name", v)));
        EXPECT_EQ(result, "before\nafter");
    }
}


struct A {
    int i = 444;

    A(int i) : i(i) {
//        std::cerr << fmt::format("Created `A` with i = {}", this->i) << std::endl;
    }
};

class StringCallbackTest{
public:
    std::string operator()(){return std::string();}
};



static_assert(DefaultProviders<void>::is_provider_type_v<std::string>);
static_assert(std::is_same_v<    std::string, std::result_of_t<StringCallbackTest()>>);
static_assert(DefaultProviders<void>::is_provider_type_v<std::remove_reference_t<std::result_of_t<StringCallbackTest()>>>);
static_assert(DefaultProviders<void>::is_provider_type_v<std::result_of_t<StringCallbackTest()>>);

auto l = [](){return StringCallbackTest();};
static_assert(DefaultProviders<void>::is_provider_callback_type_v<StringCallbackTest>);


std::unique_ptr<Provider_Interface> get_provider(A const & a) {
    std::cerr << fmt::format("making provider for A.i: {}", a.i) << std::endl;
    return make_provider(
        std::pair{"I", [a]()->std::string{
            std::cerr << fmt::format("in callback, A.i = {}", a.i) << std::endl;
            return fmt::format("{}", a.i);}
        }, 
        std::pair{"J", "6"});
}

//static_assert(DefaultProviders<void>::can_get_provider_for_type_v<A>);


struct B {
    B(){
//        std::cerr << fmt::format("created B at {}", (void*)this) << std::endl;
    }
    ~B(){
//        std::cerr << fmt::format("B Destructor at {}", (void*)this) << std::endl;
    }
    std::string name = "B name";
    std::vector<A> vec_a{1,2,3,4,5};

    std::vector<A> const & get_vec_a() {
//        std::cerr << fmt::format("get_vec_a  -- this: {}", (void*)this) << std::endl;
        return this->vec_a;}

    B(B&&) = delete;
    
    inline static std::vector<A> static_vec_a = std::vector<A>{1,2,3,4,5};

    std::unique_ptr<Provider_Interface> get_provider() const {
//        std::cerr << fmt::format("B::get_provider called with this: {}", (void*)this) << std::endl;
        return make_provider(std::pair{"NAME", this->name},
            
                            // std::pair("GET_VEC_A", std::bind(&B::get_vec_a, this))
//                             std::pair("GET_VEC_A", [&]()->std::vector<A>{
////                                 std::cerr << fmt::format("B::lambda callback this: {}", (void*)this) << std::endl;
////                                 std::cerr << fmt::format("{}", this->vec_a[0].i) << std::endl;
//                                 return std::vector<A>{1, 2, 3, 4, 5};}
//                             )
                                std::pair("GET_VEC_A", std::ref(static_vec_a))
        
        );
        
    }
};
static_assert(DefaultProviders<void>::can_get_provider_for_type_v<B>);

static_assert(xl::is_range_for_loop_able_v<std::vector<int>>);


TEST(template, UserDefinedTypeArray) {
    B b;

    TemplateMap templates{std::pair{"A1", Template("{i: {{I}} j: {{J}}}")},
                          std::pair{"A2", Template("{i2: {{I}} j2: {{J}}}")}};

//    auto fill_result = Template("A1: {{GET_VEC_A%, |A1}}").fill(std::ref(b), std::move(templates));
    auto fill_result = Template("B: '{{NAME}}' A1: {{GET_VEC_A%, |A1}} A2: {{GET_VEC_A%, |A2}}").fill(std::ref(b), std::move(templates));
    EXPECT_EQ(fill_result, "B: 'B name' A1: {i: 1 j: 6}, {i: 2 j: 6}, {i: 3 j: 6}, {i: 4 j: 6}, {i: 5 j: 6} A2: {i2: 1 j2: 6}, {i2: 2 j2: 6}, {i2: 3 j2: 6}, {i2: 4 j2: 6}, {i2: 5 j2: 6}");
}


//
//vector<A> vector_object_callback() {
//    return {10, 11, 12};
//}
//TEST(template, VectorCallbackTemplate) {
//    TemplateMap templates{std::pair{"A1", Template("{i: {{I}} j: {{J}}}")},
//                          std::pair{"A2", Template("{i2: {{I}} j2: {{J}}}")}};
//
//    EXPECT_EQ(Template("replace: {{TEST1%, |A1}}").fill(make_provider(std::pair{"TEST1", make_provider(vector_object_callback)}), std::move(templates)),
//              "replace: {i: 10 j: 6}, {i: 11 j: 6}, {i: 12 j: 6}");
//}

//
//
//TEST(template, LeadingJoinString) {
//    TemplateMap templates{std::pair{"A1", Template("{i: {{I}} j: {{J}}}")},
//                          std::pair{"A2", Template("{i2: {{I}} j2: {{J}}}")}};
//
//    auto provider = make_provider(std::pair{"TEST1", make_provider(vector_object_callback)});
//    EXPECT_EQ(Template("replace: {{TEST1%%, |A1}}").fill(provider, std::move(templates)),
//              "replace: , {i: 10 j: 6}, {i: 11 j: 6}, {i: 12 j: 6}");
//}





struct Finger {
    enum class Name{Thumb = 0, Pointer, Middle, Ring, Pinky};
    static inline std::vector<std::string> names{"thumb", "pointer", "middle", "ring", "pinky"};

    Name name;
    Finger(Name name) : name(name) {}
    std::string & get_name(){
        return Finger::names[(int)this->name];
    }

    std::unique_ptr<Provider_Interface> get_provider() {
        return make_provider(std::pair{"NAME", this->get_name()});
    }
};


struct Hand {
private:
    std::vector<Finger> fingers{Finger::Name::Thumb, Finger::Name::Pointer, Finger::Name::Middle, Finger::Name::Ring, Finger::Name::Pinky};

public:
    auto get_finger_count(){return fingers.size();}
    auto const & get_fingers() {return this->fingers;}

    std::unique_ptr<Provider_Interface> get_provider() {
        return make_provider(std::pair{"FINGERS", "BOGUS"});
    }

};

struct Arm {
    enum class Side{Left = 0, Right};
private:
    Side side;
    std::vector<std::string> const side_names{"left", "right"};
public:
    Arm(Side side) : side(side) {}
    Hand get_hand() const {return Hand();}
    static Template get_template() {
        return Template("Arm: Hands: {HANDS:}");
    }
    std::unique_ptr<Provider_Interface> get_provider() {
        return make_provider(std::pair{"HANDS","BOGUS"});
    }
    auto get_side() {return this->side;}
};


struct Person {
private:
    std::vector<Arm> arms{Arm(Arm::Side::Left), Arm(Arm::Side::Right)};
    std::string name;
public:
    Person(std::string const & name) : name(name) {}

    std::string const & get_name() const {return this->name;}
    std::vector<Arm> const & get_arms() const {return this->arms;}

    std::unique_ptr<Provider_Interface> get_provider() {
//        std::cerr << fmt::format("Getting Person provider") << std::endl;
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


    std::vector<std::map<std::string, std::string>> vector_provider{
        {
            std::pair(std::string("SUPERLATIVE"), std::string("an awesome")), 
            std::pair(std::string("EXCLAMATION"), std::string("!"))
        },
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


TEST(template, TemplateSubstitutionTemplate) {
    auto templates = load_templates("templates");

    auto result = Template("{{!a}} {{!b}}").fill("", std::move(templates));

    EXPECT_EQ(result, "a.template contents b.template contents");

    EXPECT_THROW(Template("{{!a}} {{!c}}").fill("", std::move(templates)), TemplateException);
}


class HasProvider {
    std::string s;

public:
    HasProvider(std::string s) : s(s) {}
    std::unique_ptr<Provider_Interface> get_provider() const {
        return make_provider(std::pair("string", this->s));
//        return make_provider(std::pair("string", std::ref(this->s)));

    }
    
    HasProvider(HasProvider const &);
    HasProvider(HasProvider &&);
};

HasProvider::HasProvider(HasProvider const &) = default;

HasProvider::HasProvider(HasProvider&&) = default;

/**
 * Class for making sure that uncopyable objects can be used as can_get_provider Providers
 * Substitutes:A => B, C => D, and strings => vector of providers with the key "string" mapped to the parameterized value in its constructor
 */
class Uncopyable {
    std::unique_ptr<int> upi;
    std::vector<HasProvider> strings = {HasProvider("string1"), HasProvider("string2")};
public:
    Uncopyable() {}
    Uncopyable(Uncopyable &&) = default;
    ProviderPtr get_provider() const {
        return make_provider(std::pair("A", "B"),std::pair("C", "D"), std::pair("uncopyable strings", make_provider(strings)));
    }
};


static_assert(DefaultProviders<void>::can_get_provider_for_type_v<xl::remove_refs_and_wrapper_t<std::reference_wrapper<Uncopyable>>>);

/**
 * Provides a vector of Uncopyable objects
 */
class UncopyableHolder {
    std::vector<std::unique_ptr<Uncopyable>> v;

public:
    UncopyableHolder(){
        v.emplace_back(std::make_unique<Uncopyable>());
        v.emplace_back(std::make_unique<Uncopyable>());
    }
    std::unique_ptr<Provider_Interface> get_provider() const {
        return make_provider(std::pair("v", std::ref(v)));
    }
    
    UncopyableHolder(UncopyableHolder const &) = delete;
    UncopyableHolder(UncopyableHolder &&) = default;
};


TEST(template, UncopyableVectorProvider) {
    std::vector<Uncopyable> vups;
    vups.push_back(Uncopyable());
    vups.push_back(Uncopyable());

    auto provider = make_provider(std::pair("uncopyable_vector", std::ref(vups)));

    auto result = Template("{{uncopyable_vector|!{{A}}}}").fill(provider);

    EXPECT_EQ(result, "B\nB");
}


TEST(template, ExpandInline) {
    auto provider = make_provider(std::pair("uncopyable", Uncopyable()));
    auto result = Template("{{uncopyable|!{{A}}{{C}}}}").fill(provider);
    EXPECT_EQ(result, "BD");
}


TEST(template, ExpandVectorInline) {
    
    UncopyableHolder uch;
    std::map<std::string, Template> templates;
    
    //                                                     vector<HasProvider> in Uncopyable
    //                                                     |                     string in HasProvider
    templates.emplace("uncopyable", Template("{{uncopyable strings|!{{string}}}}"));

    // v - vector of Uncopyable objects
    // uncopyable - name of external template to fill with each element in v  
    auto result = Template("{{v|uncopyable}}").fill(UncopyableHolder(), std::move(templates));
    EXPECT_EQ(result, "string1\nstring2\nstring1\nstring2");
}


TEST(template, ExpandEmptyLine) {
    auto result = Template("{{empty_substitution|!!}}").fill("");

    EXPECT_EQ(result, "");
}


// this just needs to compile
TEST(template, VectorOfUniquePointer){
    std::vector<std::unique_ptr<Uncopyable>> vupc;
    vupc.push_back(std::make_unique<Uncopyable>());
    vupc.push_back(std::make_unique<Uncopyable>());

    std::vector<std::unique_ptr<Uncopyable>> const cvupc(std::move(vupc));


    auto result = Template("{{vector|!!\n"
                 " * @param \\{{{A}}\\} {{C}}}}").fill(make_provider(std::pair("vector", std::ref(cvupc))));

    EXPECT_EQ(result, " * @param {B} D\n * @param {B} D");
}



class ProviderContainerTypeA{};
class ProviderContainerTypeB{};



struct Impl1 {

    static auto get_provider(ProviderContainerTypeA const &) {
        return make_provider("Impl1 for A");
    }
    static auto get_provider(ProviderContainerTypeB const &) {
        return make_provider("Impl1 for B");
    }
};


struct Impl2 {

    static auto get_provider(ProviderContainerTypeA const &) {
        return make_provider("Impl2 for A");
    }
    static auto get_provider(ProviderContainerTypeB const &) {
        return make_provider("Impl2 for B");
    }
};

static_assert(DefaultProviders<Impl1>::has_get_provider_in_provider_container_v<ProviderContainerTypeA>);
static_assert(DefaultProviders<Impl1>::can_get_provider_for_type_v<ProviderContainerTypeA>);
static_assert(DefaultProviders<Impl1>::has_get_provider_in_provider_container_v<ProviderContainerTypeB>);
static_assert(DefaultProviders<Impl1>::can_get_provider_for_type_v<ProviderContainerTypeB>);



TEST(template, ProviderContainers) {
    {
        auto result = Template("{{A}} {{B}}").fill<Impl1>(make_provider<Impl1>(
            std::pair("A", ProviderContainerTypeA()),
            std::pair("B", ProviderContainerTypeB())
        ));
        EXPECT_EQ(result, "Impl1 for A Impl1 for B");
    }
    {
        auto result = Template("{{A}} {{B}}").fill<Impl2>(make_provider<Impl2>(
            std::pair("A", ProviderContainerTypeA()),
            std::pair("B", ProviderContainerTypeB())
        ));
        EXPECT_EQ(result, "Impl2 for A Impl2 for B");
    }
}


TEST(template, ProviderContainersReUse) {
    {
        
            auto result = Template("{{container|!{{}}}} {{container|!{{}}}}").fill<Impl1>(make_provider<Impl1>(
            std::pair("container2", std::vector<std::string>{"one", "two", "three"}),
            std::pair("container", std::vector<std::string>{"one", "two", "three"})
        ));
        EXPECT_EQ(result, "one\ntwo\nthree one\ntwo\nthree");
    }
   
}

TEST(template, NumberProvider) {
    EXPECT_EQ(Template("{{number}}").fill(make_provider(std::pair("number", 5))), "5");
    EXPECT_EQ(Template("{{number}}").fill(make_provider(std::pair("number", 5.5))), "5.5");
}


TEST(template, NullPointer)
{
    int * pi = nullptr;
    EXPECT_THROW(Template("{{null_pointer}}").fill(make_provider(std::pair("a", (int*)nullptr))), TemplateException);
}

TEST(template, EmptyUniquePointer)
{
    EXPECT_THROW(Template("{{empty_unique_pointer}}").fill(make_provider(std::pair("empty_unique_pointer", std::unique_ptr<int>()))), TemplateException);
}


// test the T* Provider
TEST(template, ContainerOfPointers) {
    HasProvider a("a"), b("b"), c("c"), d("d");
    std::vector<HasProvider *> v{&a, &b, &c, &d};

    auto result = Template("{{vector% |!{{string}}}}").fill(make_provider(std::pair("vector", v)));

    EXPECT_EQ(result, "a b c d");
}




TEST(template, EmptyContainerContingentContent) {
    {

        std::vector<std::string> v;

        // newline after trailing contingent substitution shouldn't be contingent
        auto result = Template("BEFORE\nX{{<VECTOR|!!\n{{DUMMY}}>}}Y\nAFTER").fill(std::pair("VECTOR", std::ref(v)));

        EXPECT_EQ(result, "BEFORE\nAFTER");
    }
    {

        std::vector<std::string> v;

        // newline after trailing contingent substitution shouldn't be contingent
        auto result = Template(R"(
{{<<v|!!
asdf}}

{{<<v|!!
asdf{{asdf}}}}

{{<<v|!!
>>}}
        
                                 )").fill(std::pair("v", std::ref(v)));

        EXPECT_EQ(result, "");
    }
    {
        // second should be taken by {{empty>}} not {{<not_empty}}
        auto result = Template("ONE\nA {{<empty>}} B\nTWO").fill(make_provider(
            std::pair("empty", "")
        ));
        EXPECT_EQ(result, "ONE\nTWO");
    } {
        // second should be taken by {{empty>}} not {{<not_empty}}
        auto result = Template("ONE\n\nA {{<<empty>>}} B\n\nTWO").fill(make_provider(
            std::pair("empty", "")
        ));
        EXPECT_EQ(result, "");
    }
    {
        // second should be taken by {{empty>}} not {{<not_empty}}
        auto result = Template("ONE\n\nA {{<<empty>>}} B\n\nC {{<<empty>>}} D\n\nTWO").fill(make_provider(
            std::pair("empty", "")
        ));
        EXPECT_EQ(result, "");
    }
    
}



TEST(template, ContingentContentPrecedence) {
    {
        // second should be taken by {{empty>}} not {{<not_empty}}
        auto result = Template("FIRST {{empty>}} SECOND {{<not_empty}} THIRD").fill(make_provider(
            std::pair("empty", ""),
            std::pair("not_empty", "NOT_EMPTY")
        ));
        EXPECT_EQ(result, "FIRST NOT_EMPTY THIRD");
    }
    {
        // second should be taken by {{empty>}} not {{<not_empty}}
        auto result = Template("FIRST\n{{empty>>}} SECOND\n\n\n\n{{<not_empty}}\nTHIRD").fill(make_provider(
            std::pair("empty", ""),
            std::pair("not_empty", "NOT_EMPTY")
        ));
        EXPECT_EQ(result, "FIRST\nNOT_EMPTY\nTHIRD");
    }
    {
        // second should be taken by {{not_empty>}} not {{<empty}}
        auto result = Template("FIRST {{not_empty>}} SECOND {{<empty}} THIRD").fill(make_provider(
            std::pair("empty", ""),
            std::pair("not_empty", "NOT_EMPTY")
        ));
        EXPECT_EQ(result, "FIRST NOT_EMPTY SECOND  THIRD");
    }
    {
        // second should be taken by {{not_empty>}} not {{<empty}}
        auto result = Template("FIRST {{not_empty>>}} SECOND\n\n\n\n{{<empty}} THIRD").fill(make_provider(
            std::pair("empty", ""),
            std::pair("not_empty", "NOT_EMPTY")
        ));
        EXPECT_EQ(result, "FIRST NOT_EMPTY SECOND\n\n\n\n THIRD");
    }
}


TEST(template, SetOfStrings) {
    std::set<std::string> s;

    auto result = Template("BEFORE\nX{{<VECTOR|!!\n{{DUMMY}}>}}Y\nAFTER").fill(std::pair("VECTOR", ref(s)));
}



class HasProvider2 {
    std::vector<HasProvider> v;

public:
    HasProvider2(std::string s)  {
        v.push_back(HasProvider(s+s));
        v.push_back(HasProvider(s+s));
    }
    ProviderPtr get_provider() const {
        return make_provider(
            std::pair("has_provider", std::ref(v)),
            std::pair("has_provider_copy", v),
            std::pair("five", "five")

        );
    }
};



TEST(template, VectorOfGetProviderableObjects) {

    {
        auto result = Template("{{has_provider|!{{string}}}}").fill(HasProvider2("hp2"));
        EXPECT_EQ(result, "hp2hp2\nhp2hp2");
    }

    {
        HasProvider2 hp2("hp2-2");
        auto result = Template("{{has_provider_copy|!!\n{{string}}}}").fill(std::ref(hp2));

        EXPECT_EQ(result, "hp2-2hp2-2\nhp2-2hp2-2");
    }
}

class VectorOfUniquePtrToHasProvider {
    std::vector<std::unique_ptr<HasProvider>> v;

public:
    VectorOfUniquePtrToHasProvider(std::string s)  {
        v.push_back(std::make_unique<HasProvider>(s+s));
        v.push_back(std::make_unique<HasProvider>(s+s));
    }
    ProviderPtr get_provider() const {
        return make_provider(
            std::pair("has_provider", xl::copy(v))
        );
    }
};

TEST(template, RefToGetProviderableWithVectorOfGetProviderable) {
    {
        VectorOfUniquePtrToHasProvider hp2("hp2-2");
        auto result = Template("{{has_provider|!!\nAAA {{string}}}}").fill(std::ref(hp2));

        EXPECT_EQ(result, "AAA hp2-2hp2-2\nAAA hp2-2hp2-2");
    }
}


TEST(template, MissingCloseCurlyBrace) {
    EXPECT_THROW(Template("{{a}{{b}}").compile(), TemplateException);
}

TEST(template, Comments) {
    {
        auto result = Template("{{#This is a comment}}").fill("BOGUS");
        EXPECT_EQ(result, "");
    }
    {
        auto result = Template("BEFORE {{#This is a comment}} AFTER").fill("BOGUS");
        EXPECT_EQ(result, "BEFORE  AFTER");
    }
    {
        auto result = Template("{{early}} BEFORE {{#This is a comment}} AFTER {{late}}").fill(make_provider(
            std::pair("early", "EARLY"),
            std::pair("late", "LATE")
        ));
        EXPECT_EQ(result, "EARLY BEFORE  AFTER LATE");
    }
    {
        auto result = Template("{{#classes|!{{}}}}").fill("BOGUS");
        EXPECT_EQ(result, "");
    }
    
    
}


class SimpleProviderProvider {
    HasProvider has_provider;
    
public:
    SimpleProviderProvider(std::string s) :
        has_provider(s+s)
    {}
    
    ProviderPtr get_provider() const {
        return make_provider(
            std::pair("has_provider", std::ref(has_provider))
        );
    }
};

TEST(template, PeriodSeparatedNames) {
//    {
//        Template t("{{simple_provider.has_provider}}");
//        EXPECT_THROW(t.fill(std::pair("simple_provider", SimpleProviderProvider("simple_provider"))), TemplateException);
//    }
//    {
//        Template t("{{simple_provider}}");
//        EXPECT_THROW(t.fill(std::pair("simple_provider", SimpleProviderProvider("simple_provider"))), xl::FormattedException);
//    }
//    {
//        Template t("{{}}");
//        EXPECT_THROW(t.fill(std::pair("simple_provider", SimpleProviderProvider("simple_provider"))), xl::FormattedException); // TODO: this is the wrong exception type for this error to be throwing
//    }
    {
        auto result = Template("{{simple_provider.has_provider.string}}").
            fill(std::pair("simple_provider", SimpleProviderProvider("simple_provider")));
        EXPECT_EQ(result, "simple_providersimple_provider");
    }
}


TEST(template, PeriodSeparatedNamesWithContainers) {
    {
        auto result = Template("{{has_provider2|!{{has_provider|!{{string}}}}}}").fill(std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "hp2hp2\nhp2hp2");
    }
    {
        auto result = Template("{{has_provider2.has_provider|!{{string}}}}").fill(std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "hp2hp2\nhp2hp2");
    }
    {
        auto result = Template("{{has_provider2.has_provider.string}}").fill(std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "hp2hp2\nhp2hp2");
    }
}

TEST(template, RewindDots) {
    {
        // baseline no rewind 
        auto result = Template("{{a|!{{b}}}}").fill(
            make_provider(
                std::pair("a", make_provider(std::pair("b", "bad"))),
                std::pair("b", "good")
            )
        );

        EXPECT_EQ(result, "bad");
    }
    {
        // one dot doesn't do anything
        auto result = Template("{{a|!{{.b}}}}").fill(
            make_provider(
                std::pair("a", make_provider(std::pair("b", "bad"))),
                std::pair("b", "good")
            )
        );

        EXPECT_EQ(result, "bad");
    }
    {
        // should rewind back to provider with keys: "a" and "b"
        auto result = Template("{{a|!{{..b}}}}").fill(
            make_provider(
                std::pair("a", make_provider(std::pair("b", "bad"))),
                std::pair("b", "good")
            )
        );

        EXPECT_EQ(result, "good");
    }
    {
        // should rewind "once" - but through non-"core" has-provider provider
        auto result = Template("{{a|!{{..b}}}}").fill(
            make_provider(
                std::pair("a", make_provider(std::pair("b", HasProvider("bad")))),
                std::pair("b", "good")
            )
        );

        EXPECT_EQ(result, "good");
    }
    
}

TEST(template, RewindToContainerElement) {
    {
        // baseline no rewind 
        std::vector<ProviderPtr> v;
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad1"))),
            std::pair("b", "good1")
        ));
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad2"))),
            std::pair("b", "good2")
        ));
        auto result = Template("{{v%|!{{a|!{{b}}}}}}").fill(
            std::ref(v)
        );

        EXPECT_EQ(result, "bad1bad2");
    }
    {
        // baseline no rewind 
        std::vector<ProviderPtr> v;
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad1"))),
            std::pair("b", "good1")
        ));
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad2"))),
            std::pair("b", "good2")
        ));
        auto result = Template("{{v%|!{{a|!{{..a.b}}}}}}").fill(
            std::ref(v)
        );

        EXPECT_EQ(result, "bad1bad2");
    }
    {
        // baseline no rewind 
        std::vector<ProviderPtr> v;
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad1"))),
            std::pair("b", "good1")
        ));
        v.push_back(make_provider(
            std::pair("a", make_provider(std::pair("b", "bad2"))),
            std::pair("b", "good2")
        ));
        auto result = Template("{{v%|!{{a|!{{..b}}}}}}").fill(
            std::ref(v)
        );

        EXPECT_EQ(result, "good1good2");
    }
}


    
TEST(template, FallBackToParentProviderForMissingNames) {
    { 
        auto result = Template("{{has_provider2|!{{has_provider|!{{string}}{{five}}}}}}").fill(
            std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "hp2hp2five\nhp2hp2five");
    } 
    {
        auto result = Template("{{has_provider2.has_provider|!{{string}}{{five}}}}").fill(
            std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "hp2hp2five\nhp2hp2five");
    }
    {
        auto result = Template("{{has_provider2.has_provider|!{{has_provider2.five}}}}").fill(
            std::pair("has_provider2", HasProvider2("hp2")));
        EXPECT_EQ(result, "five\nfive");
    }
}