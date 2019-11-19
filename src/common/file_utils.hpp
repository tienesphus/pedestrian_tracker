#ifndef BUS_COUNT_FILE_UTILS_HPP
#define BUS_COUNT_FILE_UTILS_HPP

#include <vector>
#include <string>

/**
 * Lists all files and folders in a directory
 * @param folder the folder to get a listing of
 * @return the list of files/folders relative to the given folder. Unsorted. Returns empty set if the
 * given path does not exist or is not a folder.
 */
std::vector<std::string> list_files(const std::string& folder);

/**
 * Lists all files and folders in a directory and all subdirectories
 * @param folder the folder to get a listing of
 * @return the list of files/folders relative to the given folder. Unsorted. Returns empty set if the
 * given path does not exist or is not a folder.
 */
std::vector<std::string> list_files_recursive(const std::string& folder);

/**
 * Recursively lists all files in a folder whose relative path matches the given regex
 * @param folder the folder to search
 * @param path_regex the regex to match against
 * @return a list of file paths relative to the given folder (unsorted)
 */
std::vector<std::string> list_files_recursive(const std::string& folder, const std::string& path_regex);

#endif //BUS_COUNT_FILE_UTILS_HPP
