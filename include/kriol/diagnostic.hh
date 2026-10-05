#ifndef KRIOL_DIAGNOSTIC_HH
#define KRIOL_DIAGNOSTIC_HH

#include <string>

namespace kriol {

// The "file:line: severity: " prefix of a compiler message, the format editors
// recognise; the line is left out when it is 0.
inline std::string DiagnosticPrefix(const std::string& file, int line, const char* severity) {
    std::string location = file.empty() ? "kriol" : file;
    if (line > 0) location += ":" + std::to_string(line);
    return location + ": " + severity + ": ";
}

} // namespace kriol

#endif
