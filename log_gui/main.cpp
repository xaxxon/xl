#include <optional>

#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

#include "../include/xl/regexer.h"
#include "../include/xl/log.h"

int main(int argc, char *argv[])
{
    {
        QCoreApplication application(argc, argv);
        application.setApplicationName("xl_log_tool");
        application.setApplicationVersion("TODO: create cmake configure'd header for version info");

        QCommandLineParser parser;
        parser.setApplicationDescription("xl log status file tool - supports command-line only and GUI interfaces");
        parser.addHelpOption();
        parser.addVersionOption();

        // addPositionalArgument is for fixed-position arguments which must always be present:
        //  some-command these_three values_are positional_arguments --before=any --optional=arguments

        parser.addOptions({
                              {"status-filename",
                               "Name of the log status file to set values on",
                               "filename"
                              },
                              {"all-level-status",
                                  "Sets the status of all levels to this value",
                                  "on|off"
                              },
                              {"all-subject-status",
                                  "Sets the status of all subjects to this value",
                                  "on|off"
                              }
                          });

        parser.process(application);
        bool command_line_only = false;
        std::optional<bool> all_level_status{};
        std::optional<bool> all_subject_status{};
        xl::Regex boolean_regex("^\\s*(?:(?<true>(?:true|t|1|on|y|yes))|(?<false>(?:false|f|0|off|n|no)))\\s*$");
        if (auto matches = boolean_regex.match(parser.value("all-level-status").toStdString())) {
            all_level_status = matches.has("true");
        }
        if (auto matches = boolean_regex.match(parser.value("all-subject-status").toStdString())) {
            all_subject_status = matches.has("true");
        }



        if (all_level_status || all_subject_status) {
            auto filename = parser.value("status-filename").toStdString();
            if (filename.empty()) {
                std::cerr << fmt::format("Must specify a filename in order to set level or subject statuses") << std::endl;
                exit(1);
            }
            xl::log::LogStatusFile status_file(filename, true);
            if (all_level_status) {
                status_file.levels = *all_level_status;
            }
            if (all_subject_status) {
                status_file.subjects = *all_subject_status;
            }
            status_file.write();
            exit(0);
        }

    }
    QApplication application(argc, argv);
    application.setApplicationName("xl_log_tool");
    application.setApplicationVersion("TODO: create cmake configure'd header for version info");

    QStringList args;
    for(int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    if (args.empty()) {
        args.push_back("./");
    }
    MainWindow w(args);
    w.show();

    return application.exec();
}


