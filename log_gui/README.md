## Log GUI

This program is a graphical application to allow monitoring and changing of log levels for another
application using xl::log with an enabled status file. 

This program requires Qt 5.9 or later to build.

Running Log GUI without any parameters will load any files ending in `.log_status` from the current
directory (each in its own tab).  Alternatively a specific file or directory may be provided.

Changing the status flag on a log level or subject will immediately change the logging status
of a running application or if the application isn't running, will change the status of the
application when it runs (assuming the other application doesn't enforce certain logging 
statuses in its own code).   There is no need to save changes in Log GUI, as they take effect
immediately.

A regular-expression filter may be added which will cause all message strings not matching the
regular expression to be discarded.   To disable filtering, make the regular expression the
empty string.   The regex will not take affect until pressing enter or changing focus away
from the text editor or switching foreground application.