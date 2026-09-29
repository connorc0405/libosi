#include <iostream>

#include "ProfileHandler.h"

#include "rapidjson/reader.h"

bool ProfileHandler::Int(int val) { return OnNumber(val); }
bool ProfileHandler::Uint(unsigned val) { return OnNumber(val); }
bool ProfileHandler::Int64(int64_t val) { return OnNumber(val); }
bool ProfileHandler::Uint64(uint64_t val) { return OnNumber(static_cast<int64_t>(val)); }
bool ProfileHandler::Default() { return true; }

bool ProfileHandler::Null()
{
    // Only meaningful at depth 2 within "offsets": a struct with a null
    // value (e.g. "UNKNOWN": null) instead of an empty/populated object.
    if (section_ == Section::Offsets && depth_ == 2) {
        TRANSLATE[current_type_] = index_++;
        OFFSET.emplace_back();
    }
    return true;
}

bool ProfileHandler::String(const Ch* str, rapidjson::SizeType len, bool)
{
    // Only meaningful at depth 3 within "enums": the enum member name.
    if (section_ == Section::Enums && depth_ == 3) {
        ENUM[current_enum_].emplace(pending_ordinal_, std::string(str, len));
    }
    return true;
}

bool ProfileHandler::StartObject()
{
    ++depth_;
    if (depth_ == 3) {
        if (section_ == Section::Offsets) {
            current_member_map_.clear();
        }
    } else if (depth_ == 4 && section_ == Section::Offsets) {
        have_offset_ = false;
        have_type_ = false;
        pending_offset_ = 0;
        pending_type_ = 0;
    }
    return true;
}

bool ProfileHandler::EndObject(rapidjson::SizeType)
{
    if (depth_ == 4 && section_ == Section::Offsets) {
        if (have_offset_ && have_type_) {
            current_member_map_.emplace(current_member_,
                                        std::make_pair(pending_offset_, pending_type_));
        } else {
            std::cerr << "Skipping malformed member '" << current_member_ << "' in type '"
                      << current_type_ << "': missing "
                      << (!have_offset_ ? "'offset'" : "")
                      << (!have_offset_ && !have_type_ ? " and " : "")
                      << (!have_type_ ? "'type'" : "") << std::endl;
        }
    } else if (depth_ == 3 && section_ == Section::Offsets) {
        TRANSLATE[current_type_] = index_++;
        OFFSET.push_back(std::move(current_member_map_));
    } else if (depth_ == 2 &&
               (section_ == Section::Offsets || section_ == Section::Enums)) {
        // Closing the "offsets" or "enums" map itself; nothing to do,
        // depth 1 tracks that we're back at the root.
    }
    --depth_;
    if (depth_ == 1) {
        section_ = Section::None;
    }
    return true;
}

bool ProfileHandler::Key(const Ch* str, rapidjson::SizeType len, bool)
{
    const std::string key(str, len);
    if (depth_ == 1) {
        // Top-level section selector: "offsets" or "enums".
        if (key == "offsets") {
            section_ = Section::Offsets;
        } else if (key == "enums") {
            section_ = Section::Enums;
        }
    } else if (depth_ == 2) {
        if (section_ == Section::Offsets) {
            current_type_ = key;
        } else if (section_ == Section::Enums) {
            current_enum_ = key;
        }
    } else if (depth_ == 3) {
        if (section_ == Section::Offsets) {
            current_member_ = key;
        } else if (section_ == Section::Enums) {
            // Ordinal keys are stringified integers, e.g. "2".
            pending_ordinal_ = std::stol(key);
        }
    } else if (depth_ == 4 && section_ == Section::Offsets) {
        pending_key_ = key;
    }
    return true;
}

bool ProfileHandler::OnNumber(int64_t val)
{
    if (depth_ == 4 && section_ == Section::Offsets) {
        if (pending_key_ == "offset") {
            pending_offset_ = static_cast<int>(val);
            have_offset_ = true;
        } else if (pending_key_ == "type") {
            pending_type_ = static_cast<unsigned int>(val);
            have_type_ = true;
        }
    }
    return true;
}
