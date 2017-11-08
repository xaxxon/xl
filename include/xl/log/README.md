## Log

A logging library which calls registered callbacks when it receives a logging message.  Useful for integrating logging in libraries with different logging systems in the applications which use them.


### Levels and Subjects

Each log message has a log level and subject associated with it.

User-defineable logging levels and subjects can be specified.   The standard levels are info, warn, and error.   These are
often sufficient for many uses.   There are helper functions for using the default log levels.  For example, for the info
level:

    log.info(Subject::SomeSubject, "info message");


There is only one default subject, "default".   Most use cases will want to provide their own subjects.

Logging can be controlled by enabling/disabling both levels and subjects.   Only if both elements of the pair are enabled
is a log message processed.   


### libfmt integration

When `libfmt` is present, you can specify log messages using the libfmt style of string formatting:

    log.error(Subject::SomeSubject, "Error in system {}: {}", system_name, error_details);
    
### Log Status File

If enabled, the log object will mirror its state to a file.  Any programatic changes will be reflected in this file.

If the status file is edited 'by hand' then the changes will be picked up and used - either immediately if the program is
still running or the next time the program begins.   

*Note*: there is currently no locking on this file, so there may be issues when multiple things try to change it at once.


### Custom Subjects and Levels
    
Here is an example of how to make custom subjects:
    
    struct MyCustomSubjects {
        inline static std::string subject_names[] = {
            "subject 1", "subject 2"
        };
    
        enum Subjects {
            Subject1, Subject2, 
            LOG_LAST_SUBJECT
        };
    };
    

Custom levels are the same as subjects, just change everything from subject to 
level.



### Not supported but would be cool

* Logging a message with a combination of subjects
