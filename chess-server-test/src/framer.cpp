#include "framer.h"

std::vector<std::string> extractLines(std::string& buffer, bool& tooLong) {
    std::vector<std::string> lines;
    tooLong = false;
    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();   // chấp nhận CRLF
        if (line.size() > MAX_LINE) { tooLong = true; continue; }
        lines.push_back(line);
    }
    if (buffer.size() > MAX_LINE) {       // quá dài mà chưa thấy '\n'
        buffer.clear();
        tooLong = true;
    }
    return lines;
}

std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> f;
    size_t start = 0, pos;
    while ((pos = line.find('|', start)) != std::string::npos) {
        f.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    f.push_back(line.substr(start));
    return f;
}
