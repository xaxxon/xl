
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "templates.h"



class Environment : public ::testing::Environment {
public:
    virtual ~Environment() {}
    // Override this to define how to set up the environment.
    virtual void SetUp() {
        xl::templates::log.add_callback([](xl::templates::LogT::LogMessage const & message) {
//           std::cerr << fmt::format("xl::templates: '{}'", message.string) << std::endl;
        });
    }
    // Override this to define how to tear down the environment.
    virtual void TearDown() {}
};


static ::testing::Environment* const dummy = ::testing::AddGlobalTestEnvironment(new Environment);

