#pragma once

#include <map>
#include <optional>

#include "exceptions.h"
#include "regexer.h"

namespace xl::json {

inline xl::RegexPcre json_regex(R"REGEX(
^\s*
(?(DEFINE)(?<StringContents>(?:\\"|[^"])*))
(?(DEFINE)(?<ObjectEntry>(\s*"(?&StringContents)"\s*:\s*(?&any)\s*)))
(?(DEFINE)(?<ArrayEntry>(\s*(?&any)\s*)))
(?<any>
   (?<number>(?:-?\d+\.?\d*|-?\.\d+)([Ee][+-]?\d+)?) |
   (?<boolean>true|false) |
   (?:"(?<string>(?&StringContents))") |
   (?<array>\[(?:(?<ArrayHead>(?&ArrayEntry))\s*,?\s*)?(?<ArrayTail>((?&ArrayEntry)\s*,?\s*)*)\]) |
   (?<object>{(\s*"(?<Key>(?&StringContents))"\s*:\s*(?<Value>(?&any)))?\s*,?\s*(?<ObjectTail>((?&ObjectEntry)\s*,?\s*)*) }) |
   (?<null>null)
) # end any
\s*$
)REGEX",  xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);


inline xl::RegexPcre escaped_character_regex("\\\\(.)", xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);



/**
 * An Empty Json object is not valid, and will reply with an empty value regardless of 
 *   what type is requested of it, but will not 
 */
struct Json {
private:
    std::string source;

public:
    std::string const & get_source() const {
        return this->source;
    }

    // needed for operator[] in a map<string, Json>
    explicit Json() = default;

    explicit Json(std::string source) : source(source) {}
    Json(Json const &) = default;
    Json(Json &&) = default;

    xl::RegexResultPcre parse() const {
        // an empty source will return an empty optional for all API calls
        if (this->source.empty()) {
            return xl::RegexResultPcre();
        }
        auto matches = json_regex.match(this->source);
        if (!matches) {
            return xl::RegexResultPcre();
        }
        return matches;
    }

    std::optional<double> get_number() const {
        auto matches = parse();
        return matches.has("number") ? std::stod(matches["number"]) : std::optional<double>{};
    }

    std::optional<std::string> get_string() const {
        std::string result = parse()["string"];
        if (!result.empty()) {
            return escaped_character_regex.replace(result, "$1", true);
        } else {
            return {};
        }
    }

    std::optional<std::map<std::string, Json>> get_object() const {
        auto matches = parse();
        if (matches.has("object")) {
            std::map<std::string, Json> results;
            std::string object_string = matches["object"];
            while(auto object_entry_match = json_regex.match(object_string)) {
                std::string key = object_entry_match["Key"];
                results.emplace(escaped_character_regex.replace(key, "$1", true), Json(object_entry_match["Value"]));
                if (!object_entry_match.has("ObjectTail")) {
                    break;
                }
                object_string = fmt::format("{{{}}}", object_entry_match["ObjectTail"]);
            }
            return results;
        } else {
            return std::optional<std::map<std::string, Json>>{};
        }
    };

    std::map<std::string, Json> as_object() const {
        if (auto maybe_object = this->get_object()) {
            return *maybe_object;
        } else {
            return {};
        }
    };

    std::optional<std::vector<Json>> get_array() const {
        auto matches = parse();
        if (matches.has("array")) {
            std::vector<Json> results;
            std::string array_string = matches["array"];
            while(auto array_entry_match = json_regex.match(array_string)) {
                results.emplace_back(array_entry_match["ArrayHead"]);
                if (!array_entry_match.has("ArrayTail")) {
                    break;
                }
                array_string = fmt::format("[{}]", array_entry_match["ArrayTail"]);
            }
            return results;
        } else {
            return std::optional<std::vector<Json>>{};
        }
    }

    std::vector<Json> as_array() const {
        if (auto maybe_array = this->get_array()) {
            return *maybe_array;
        } else {
            return {};
        }
    }

    std::optional<bool> get_boolean() const {
        auto result = parse()["boolean"];
        return !result.empty() ? result == "true" : std::optional<bool>{};
    }


    /**
     * Utility function that always returns a Json object, even if the current object doesn't represent an object
     * or the object doesn't have the specified key.   If the result isn't a valid part of the JSON structure,
     * the returned Json object will return false for is_valid()
     * @param name key name to return Json object for
     * @return Json object - either valid if the key request is valid, otherwise a 'invalid' Json object
     */
    Json get_by_key(xl::string_view name) const {
        if (auto object = this->get_object()) {
            return (*object)[name];
        } else {
            return Json{};
        }
    }


    auto operator[](xl::string_view name) const {
        return get_by_key(name);
    }


    auto operator[](char const * name) const {
        return get_by_key(name);
    }


    /**
     * Utility function that always returns a Json object, even if the current object doesn't represent an array
     * or the array doesn't contain the specified index.   If the result isn't a valid part of the JSON structure,
     * the returned Json object will return false for is_valid()
     * @param name array index to return Json object for
     * @return Json object - either valid if the array index request is valid, otherwise a 'invalid' Json object
     */
    Json get_by_index(size_t index) const {
        if (auto array = this->get_array()) {
            if (array->size() > index) {
                return (*array)[index];
            }
        }
        return Json{};
    }

    auto operator[](size_t index) const {
        return get_by_index(index);
    }

    bool is_null() const {
        return parse().has("null");
    }

    bool is_valid() const {
        return this->parse();
    }

    operator bool() const {
        return this->is_valid();
    }
};



} // end namespace xl::json




