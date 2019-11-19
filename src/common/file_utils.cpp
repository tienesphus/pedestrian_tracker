#include "file_utils.hpp"

#include <regex>
#include <dirent.h>

std::vector<std::string> list_files(const std::string& folder) {
    std::vector<std::string> files;
    DIR *dir_folder;
    if ((dir_folder = opendir(folder.c_str())) != nullptr) {
        struct dirent *file_ent;
        while ((file_ent  = readdir(dir_folder)) != nullptr) {
            const std::string file_name = file_ent->d_name;
            if (file_name != "." && file_name != "..")
                files.push_back(file_name);
        }
        closedir(dir_folder);
    }
    return files;
}


std::vector<std::string> list_files_recursive(const std::string& folder) {
    auto files = list_files(folder);

    // Note we iterate over a mutating list here - therefore, as we add to the list, we will eventually
    // go over it again. That is how we get the recusive structure.
    for (size_t i = 0; i < files.size(); i++) {
        const auto filename = files[i];
        const auto full_path = folder + "/" + filename;

        const auto sub_files = list_files(full_path);
        for (const auto& sub_file : sub_files) {
            files.push_back(filename + "/" + sub_file);
        }
    }

    return files;
}

std::vector<std::string> list_files_recursive(const std::string& folder, const std::string& path_regex)
{
    auto files = list_files_recursive(folder);

    std::regex reg(path_regex);
    std::vector<std::string> filtered;
    std::copy_if (files.begin(), files.end(), std::back_inserter(filtered), [&reg](const auto& file){
        return regex_match(file, reg);
    });

    return filtered;
}