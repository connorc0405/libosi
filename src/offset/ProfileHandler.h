#pragma once

#include <map>
#include <string>
#include <vector>

#include <rapidjson/reader.h>

// #############################
/**
 * SAX-based parser (using rapidjson's Reader API) that builds up TRANSLATE,
 * OFFSET, and ENUM incrementally as the profile json is parsed, instead of
 * first building a DOM tree and walking it.
 *
 * Expected schema (see exampleschema.json):
 *
 *   {
 *       "offsets": {
 *           struct_name: {
 *               member_name: { "offset": N, "type": N },
 *               ...
 *           } | null,
 *           ...
 *       },
 *       "enums": {                    // optional section
 *           enum_name: {
 *               "<ordinal>": "<enum string>",
 *               ...
 *           },
 *           ...
 *       }
 *   }
 *
 * Nesting depth (depth counts StartObject calls, root object = depth 1):
 *
 *   Inside "offsets":
 *     depth 2 -> the offsets map; keys here are struct names. Value may be
 *                `null` (e.g. "UNKNOWN": null) or an object.
 *     depth 3 -> a struct's member-map object; keys here are member names.
 *                May be empty (e.g. "UNKNOWN": {}).
 *     depth 4 -> a member's {"offset": ..., "type": ...} object.
 *
 *   Inside "enums":
 *     depth 2 -> the enums map; keys here are enum names.
 *     depth 3 -> a single enum's ordinal-map object; keys are stringified
 *                ordinals, values are the enum member name strings.
 *
 * Because SAX events fire in exact source-byte order, TRANSLATE/OFFSET end
 * up populated in file order automatically.
 */

class ProfileHandler
    : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ProfileHandler>
{
public:
    enum class Section {
        None,
        Offsets,
        Enums,
    };
    bool Null();
    bool Int(int val);
    bool Uint(unsigned val);
    bool Int64(int64_t val);
    bool Uint64(uint64_t val);
    bool String(const Ch* str, rapidjson::SizeType len, bool);
    bool StartObject();
    bool EndObject(rapidjson::SizeType);
    bool Key(const Ch* str, rapidjson::SizeType len, bool);
    bool Default();
    // bool Bool(bool) { return true; }        // unused by this schema
    // bool Double(double) { return true; }    // unused by this schema
    // bool StartArray() { return true; } // unused by this schema
    // bool EndArray(rapidjson::SizeType) { return true; } // unused by this schema
    // bool RawNumber(const Ch*, rapidjson::SizeType, bool) { return true; } // unused by
    // this schema

    std::map<std::string, unsigned int> TRANSLATE;
    std::vector<std::map<std::string, std::pair<int, unsigned int>>> OFFSET;
    std::map<std::string, std::map<long, std::string>> ENUM;

private:
    bool OnNumber(int64_t val);

    int depth_ = 0;
    unsigned int index_ = 0;
    Section section_ = Section::None;

    std::string current_type_;
    std::string current_member_;
    std::string pending_key_;
    int pending_offset_ = 0;
    unsigned int pending_type_ = 0;
    bool have_offset_ = false;
    bool have_type_ = false;
    std::map<std::string, std::pair<int, unsigned int>> current_member_map_;

    std::string current_enum_;
    long pending_ordinal_ = 0;
};
