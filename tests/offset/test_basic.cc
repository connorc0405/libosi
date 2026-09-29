#include <sstream>

#include "offset/offset.h"
#include "src/offset/ProfileHandler.h"
#include "gtest/gtest.h"
#include "rapidjson/istreamwrapper.h"


TEST(BasicTest, TypeLibraryLoading)
{
    std::string supported_libraries[] = {
        "linux-32-3.16", "linux-64-3.16",
        "windows-32-2000",
        "windows-32-xpsp2", "windows-32-xpsp3",
        "windows-64-7sp1", "windows-64-7sp0",
        "windows-32-7sp1", "windows-32-7sp0",
    };
    for (auto& library : supported_libraries) {
        auto tlib = load_type_library(library.c_str());
        ASSERT_TRUE(tlib != nullptr) << "Could not locate supported profile";
    }

    auto doesnt_exist = load_type_library("notreal-64-7sp1");
    ASSERT_TRUE(doesnt_exist == nullptr) << "Found a library that didn't exist!";
}

TEST(BasicTest, BasicTranslation)
{
    auto tlib = load_type_library("windows-64-7sp1");
    ASSERT_TRUE(tlib != nullptr) << "Could not locate requested profile";

    auto st = translate(tlib, "_EPROCESS");
    ASSERT_TRUE(is_valid_structure_type(st)) << "Could not locate _EPROCESS type";

    auto st_repeat = translate(tlib, "_EPROCESS");
    ASSERT_TRUE(is_valid_structure_type(st_repeat))
        << "Could not lookup the same type twice";

    auto st2 = translate(tlib, "MyType");
    ASSERT_FALSE(is_valid_structure_type(st2)) << "Found a type that didn't exist";
}

TEST(BasicTest, BasicWin7x64sp1)
{
    auto tlib = load_type_library("windows-64-7sp1");
    ASSERT_TRUE(tlib != nullptr) << "Could not locate requested profile";

    auto st1 = translate(tlib, "_EPROCESS");
    ASSERT_TRUE(is_valid_structure_type(st1)) << "Could not locate _EPROCESS type";

    auto mr1 = offset_of(tlib, st1, "Win32Process");
    ASSERT_TRUE(mr1->offset == 0x258) << "Invalid offset found";
    free_member_result(mr1);


    auto st2 = translate(tlib, "UNKNOWN");
    ASSERT_TRUE(is_valid_structure_type(st2)) << "Could not locate UNKNOWN type";
    ASSERT_TRUE(is_unknown_structure_type(st2));


    auto st3 = translate(tlib, "_MMVAD_SHORT");
    ASSERT_TRUE(is_valid_structure_type(st3)) << "Could not locate _MMVAD_SHORT type";

    auto mr2 = offset_of(tlib, st3, "u1");
    ASSERT_TRUE(mr2->offset == 0) << "Invalid offset found";
    ASSERT_FALSE(is_unknown_structure_type(mr2->type)) << "Structure type is actually unknown";
    ASSERT_FALSE(is_pointer_structure_type(mr2->type)) << "Member is actually pointer to structure type";
    free_member_result(mr2);

    auto mr3 = offset_of(tlib, st3, "LeftChild");
    ASSERT_TRUE(mr3->offset == 8) << "Invalid offset found";
    ASSERT_FALSE(is_unknown_structure_type(mr3->type)) << "Structure type is actually unknown";
    ASSERT_TRUE(is_pointer_structure_type(mr3->type)) << "Member is actually not a pointer to structure type";
    free_member_result(mr3);

    auto mr4 = offset_of(tlib, st3, "StartingVpn");
    ASSERT_TRUE(mr4->offset == 24) << "Invalid offset found";
    ASSERT_TRUE(is_unknown_structure_type(mr4->type)) << "Structure type is actually known";
    ASSERT_FALSE(is_pointer_structure_type(mr4->type)) << "Structure type is actually known";
    free_member_result(mr4);
}

TEST(BasicTest, ProfileParsing)
{
    std::string profile = R"(
{
    "offsets": {
        "UNKNOWN": null,
        "_MMVAD_SHORT": {
            "u1": {
                "offset": 0,
                "type": 406
            },
            "LeftChild": {
                "offset": 8,
                "type": 2147484421
            },
            "StartingVpn": {
                "offset": 24,
                "type": 0
            }
        }
    },
    "enums": {
        "ObTypeIndexTable": {
            "2": "Type",
            "3": "Directory"
        }
    }
}
)";

    std::istringstream ss{profile};
    rapidjson::IStreamWrapper isw{ss};

    ProfileHandler handler;
    rapidjson::Reader reader;
    rapidjson::ParseResult result = reader.Parse(isw, handler);

    // TRANSLATE
    std::map<std::string, unsigned int> TEST_TRANSLATE = {
        {"UNKNOWN", 0},
        {"_MMVAD_SHORT", 1},
    };
    ASSERT_TRUE(TEST_TRANSLATE == handler.TRANSLATE);

    //OFFSET
    std::vector<std::map<std::string, std::pair<int, unsigned int>>> TEST_OFFSET = {
        // UNKNOWN
        {},
        // _MMVAD_SHORT
        {
            {"u1",
                {0, 406}
            },
            {"LeftChild",
                {8, 2147484421}
            },
            {"StartingVpn",
                {24, 0}
            }
        }
    };
    ASSERT_TRUE(TEST_OFFSET == handler.OFFSET);

    // ENUM
    std::map<std::string, std::map<long, std::string>> TEST_ENUM = {
          {
            "ObTypeIndexTable", {
                {2, "Type"},
                {3, "Directory"}
            }
          }
    };
    ASSERT_TRUE(TEST_ENUM == handler.ENUM);
}