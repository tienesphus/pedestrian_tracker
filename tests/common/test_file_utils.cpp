#include <file_utils.hpp>

#include "catch.hpp"

#define TEST_FOLDER_LOCATION (SOURCE_DIR "/tests/common/test_file_structure")

void require_lists_same(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    REQUIRE(a.size() == b.size());
    for (const auto& f : a) {
        bool missing = std::find(b.begin(), b.end(), f) == b.end();
        if (missing) {
            REQUIRE(f == std::string("not found")); // fail with file name
        }
    }
}

TEST_CASE( "File listing", "[file_utils]" )
{
    const auto& files = list_files(TEST_FOLDER_LOCATION);

    const std::vector<std::string> expected = {
            "empty_folder",
            "full_folder",
            "a.txt",
            "b.txt"
    };

    require_lists_same(expected, files);
}




TEST_CASE( "File Recursive listing", "[file_utils]" )
{
    const auto& files = list_files_recursive(TEST_FOLDER_LOCATION);

    const std::vector<std::string> expected = {
            "empty_folder",
            "full_folder",
            "a.txt",
            "b.txt",
            "full_folder/c.txt",
            "full_folder/d.txt",
            "full_folder/e.dat",
            "empty_folder/.gitignore"
    };

    require_lists_same(expected, files);
}


TEST_CASE( "File Recursive listing with filter", "[file_utils]" )
{
    const auto& files = list_files_recursive(TEST_FOLDER_LOCATION, ".*ll.*/.*\\.txt");

    const std::vector<std::string> expected = {
            "full_folder/c.txt",
            "full_folder/d.txt"
    };

    require_lists_same(expected, files);
}