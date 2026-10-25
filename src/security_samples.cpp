#include "security.hpp"
#include <filesystem>
#include <fstream>

namespace gv {
namespace {
bool safeFile(const std::string &name) {
    return name.rfind("security_", 0) == 0 && name.size() > 13 &&
           name.substr(name.size() - 4) == ".csv" &&
           name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_.") == std::string::npos &&
           name.find("..") == std::string::npos;
}
double numeric(const std::string &value) {
    std::size_t end = 0;
    const double number = std::stod(value, &end);
    if (end != value.size() || !std::isfinite(number) || number <= 0)
        throw std::runtime_error("Invalid security sample parameter.");
    return number;
}
} // namespace

// The catalogue is shared by the application and documentation authoring tools.
std::vector<SecuritySample> securitySamples(const std::string &dataDirectory) {
    std::ifstream in(std::filesystem::u8path(dataDirectory) / "security_samples.csv");
    std::string row;
    if (!std::getline(in, row))
        throw std::runtime_error("Cannot read security sample catalogue.");
    if (!row.empty() && row.back() == '\r')
        row.pop_back();
    if (row != "file,name,purpose,geo_radius,feature_radius,min_pts,fence")
        throw std::runtime_error("Invalid security sample catalogue header.");
    std::vector<SecuritySample> result;
    std::set<std::string> files;
    while (std::getline(in, row)) {
        if (!row.empty() && row.back() == '\r')
            row.pop_back();
        if (row.empty())
            continue;
        const auto fields = parseCsv(row);
        if (fields.size() != 7 || !safeFile(fields[0]) || fields[1].empty() ||
            fields[1].size() > 64 || fields[2].size() > 160 || !files.insert(fields[0]).second ||
            (fields[6] != "campus" && fields[6] != "event_corners"))
            throw std::runtime_error("Invalid security sample catalogue entry.");
        const double minimum = numeric(fields[5]);
        if (minimum > MAX_POINTS || minimum != std::floor(minimum))
            throw std::runtime_error("Invalid security sample MinPts.");
        SecuritySample sample{fields[0],
                              fields[1],
                              fields[2],
                              numeric(fields[3]),
                              numeric(fields[4]),
                              static_cast<int>(minimum),
                              fields[6] == "event_corners"};
        if (sample.geographicRadius > 100000 || sample.featureRadius > 1e7)
            throw std::runtime_error("Security sample radius exceeds limits.");
        result.push_back(std::move(sample));
        if (result.size() > 32)
            throw std::runtime_error("Security sample catalogue exceeds 32 entries.");
    }
    if (result.empty())
        throw std::runtime_error("Security sample catalogue is empty.");
    return result;
}

SecurityConsole loadSecuritySample(const std::string &dataDirectory, const SecuritySample &sample) {
    if (!safeFile(sample.file))
        throw std::runtime_error("Invalid security sample filename.");
    SecurityConsole result;
    result.setEvents(
        CsvLogAdapter{}.load((std::filesystem::u8path(dataDirectory) / sample.file).u8string()));
    result.updateSettings(
        [&](SecuritySettings &draft) { draft.setGeoRadius(sample.geographicRadius); });
    result.updateSettings(
        [&](SecuritySettings &draft) { draft.setFeatureRadius(sample.featureRadius); });
    result.updateSettings([&](SecuritySettings &draft) { draft.setMinPts(sample.minPts); });
    if (sample.eventFence) {
        if (result.events().size() < 4)
            throw std::runtime_error("Boundary sample requires four corner observations.");
        std::vector<Point> fence;
        for (std::size_t i = 0; i < 4; ++i)
            fence.push_back(SecurityConsole::project(result.events()[i].lat(),
                                                     result.events()[i].lon(), result.settings()));
        result.setFence(std::move(fence));
        result.updateSettings(
            [&](SecuritySettings &draft) { draft.setGeoQuery(result.fence().front()); });
    } else
        result.setFence({{-300, -250}, {350, -250}, {350, 300}, {-300, 300}});
    result.validate();
    result.observations(false);
    result.observations(true);
    return result;
}
} // namespace gv
