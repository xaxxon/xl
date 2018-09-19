
#pragma once

#include <fstream>
#include <regex>

#include <experimental/filesystem>

#include "templates.h"


namespace xl::templates {

namespace fs = std::experimental::filesystem;


template<class CharT, class Traits>
xl::expected<Template, std::string> load_template_from_istream(std::basic_istream<CharT, Traits> const & istream) {
    if (istream) {
        return Template(
            std::string(
                std::istreambuf_iterator<CharT>(istream.rdbuf()),
                std::istreambuf_iterator<CharT>()));
    } else {
        return xl::make_unexpected(std::string("Invalid ifstream"));
    }
}


inline std::string make_template_name(std::string const & filename) {
    static std::regex r("\\.template$");
    return std::regex_replace(filename, r, "");
}


/**
 * Loads either a single file or a directory of files into a std::map appropriate for sending to Template::fill
 * If the filename ends with .template, the trailing .template is removed from the key in the returned map
 * @param path_name file or directory name
 * @return map of templates with keys being just the filename (not the full path) of the files loaded
 */
inline xl::expected<TemplateMap, std::string> load_templates(std::string_view path_name) {
    fs::path path(path_name);
    TemplateMap results;


    if (fs::is_directory(path)) {
        for (auto entry : fs::directory_iterator(path)) {
            if (auto loaded_template = load_template_from_istream(std::ifstream(entry.path()))) {
                results.emplace(make_template_name(entry.path().filename()), *loaded_template);
            } else {
                return xl::make_unexpected(loaded_template.error());
            }
        }

    } else if (fs::is_regular_file(path)){
        if (auto loaded_template = load_template_from_istream(std::ifstream(path))) {
            results.emplace(make_template_name(path.filename()), *loaded_template);
        } else {
            return xl::make_unexpected(loaded_template.error());
        }

    } else {
        return xl::make_unexpected(std::string("Specified path is not a file or directory: " + std::string(path_name)));
    }

    return results;
}


} // end namespace xl