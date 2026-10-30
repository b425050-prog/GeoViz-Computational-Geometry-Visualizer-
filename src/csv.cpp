#include "io_detail.hpp"
namespace gv {
using namespace io_detail;
// A small state machine handles quoted commas and doubled quotes; multiline fields are rejected.
std::vector<std::string> parseCsv(const std::string &line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false, closed = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    closed = true;
                }
            } else
                field += c;
        } else if (c == ',') {
            fields.push_back(field);
            field.clear();
            closed = false;
        } else if (c == '"') {
            if (!field.empty() || closed)
                throw std::runtime_error("Malformed CSV quote.");
            quoted = true;
        } else {
            if (closed)
                throw std::runtime_error("Characters after closing CSV quote.");
            field += c;
        }
    }
    if (quoted)
        throw std::runtime_error("Unterminated CSV quote (multiline fields are unsupported).");
    fields.push_back(field);
    return fields;
}
// Double embedded quotes before surrounding a field with CSV quote delimiters.
std::string csvQuote(const std::string &s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"')
            out += '"';
        out += c;
    }
    return out + '"';
}
// Require the exact schema, check typed values, and attach row numbers to parsing errors.
std::vector<Event> CsvLogAdapter::load(const std::string &path) const {
    auto in = openInput(path);
    std::string line;
    std::getline(in, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line !=
        "event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary")
        throw std::runtime_error("CSV header does not match the documented event schema.");
    std::vector<Event> events;
    int row = 1;
    while (std::getline(in, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        try {
            auto f = parseCsv(line);
            if (f.size() != 10)
                throw std::runtime_error("Expected 10 CSV columns.");
            auto req = asInteger(f[6]), fail = asInteger(f[7]);
            if (req < 0 || fail < 0 || fail > req || req > 1000000000)
                throw std::runtime_error("Invalid request/failure counts.");
            Event e;
            e.setId(f[0]);
            e.setTimestamp(asInteger(f[1]));
            e.setUser(f[2]);
            e.setDevice(f[3]);
            e.setLat(asDouble(f[4]));
            e.setLon(asDouble(f[5]));
            e.setRequests(static_cast<int>(req));
            e.setFailures(static_cast<int>(fail));
            e.setLabel(f[8]);
            e.setSummary(f[9]);
            events.push_back(e);
            if (events.size() > 4096)
                throw std::runtime_error("At most 4096 source records are supported.");
        } catch (const std::exception &e) {
            throw std::runtime_error("CSV row " + std::to_string(row) + ": " + e.what());
        }
    }
    SecurityConsole::checkRecords(events);
    if (in.bad())
        throw std::runtime_error("CSV read failed.");
    return events;
}
// Project named geographic paths with explicit vertex and local-extent limits.
std::vector<MapLine> SecurityConsole::loadMap(const std::string &path) const {
    auto in = openInput(path);
    std::string line;
    std::getline(in, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line != "name,latitude,longitude")
        throw std::runtime_error("Invalid map CSV header.");
    std::vector<MapLine> lines;
    std::map<std::string, std::size_t> indices;
    int total = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        auto f = parseCsv(line);
        if (f.size() != 3)
            throw std::runtime_error("Invalid map row.");
        double lat = asDouble(f[1]), lon = asDouble(f[2]);
        if (std::abs(lat) > 90 || std::abs(lon) > 180)
            throw std::runtime_error("Map latitude/longitude out of range.");
        Point p = project(lat, lon, settings_);
        if (std::hypot(p.x(), p.y()) > 50000)
            throw std::runtime_error("Map is outside local projection area.");
        if (!indices.count(f[0])) {
            indices[f[0]] = lines.size();
            lines.push_back({f[0], {}});
        }
        lines[indices[f[0]]].appendPoints(p);
        if (++total > 4096)
            throw std::runtime_error("Map exceeds 4096 vertices.");
    }
    return lines;
}

} // namespace gv
