#pragma once

namespace wissel::console
{
    void log(const char* format, ...);
    void log_verbose(const char* format, ...);
    void error(const char* format, ...);

    void group(const char* format, ...);
    void group_end();
}
