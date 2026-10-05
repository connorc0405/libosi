#pragma once

#include <istream>
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * Result of parsing an offset profile (see exampleschema.json).
 */
struct Profile {
    std::map<std::string, unsigned int> TRANSLATE;
    std::vector<std::map<std::string, std::pair<int, unsigned int>>> OFFSET;
    std::map<std::string, std::map<long, std::string>> ENUM;
};

/**
 * Parses `in` with RapidJSON's DOM API, validating it against the embedded profile
 * schema. Returns false (after logging the reason to stderr) on a syntax error or
 * schema violation; `out` is left untouched in that case.
 */
bool ParseProfile(std::istream& in, Profile& out);
