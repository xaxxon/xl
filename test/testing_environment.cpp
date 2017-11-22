
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fmt/format.h>
#include "../include/xl/log.h"

#include "../include/xl/templates/exceptions.h"



class Environment : public ::testing::Environment {
public:
    virtual ~Environment() {}
    // Override this to define how to set up the environment.
    virtual void SetUp() {

        // default Info to off but if there's a status file, use that
        xl::templates::log.set_status(xl::log::DefaultLevels::Levels::Info, false);

        xl::templates::log.add_callback([](xl::templates::LogT::LogMessage const & message) {
           std::cerr << fmt::format("xl::templates ({}): '{}'", xl::templates::log.get_name(message.subject), message.string) << std::endl;
        });

        // use status file settings if they exist, otherwise create it with the current settings
        xl::templates::log.enable_status_file("test-xl.log_status");
    }
    // Override this to define how to tear down the environment.
    virtual void TearDown() {}
};


static ::testing::Environment* const dummy = ::testing::AddGlobalTestEnvironment(new Environment);

