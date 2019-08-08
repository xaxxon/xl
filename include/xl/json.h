#pragma once

#include <map>
#include <optional>

// JSON parser needs PCRE for recursive regex
#ifndef XL_USE_PCRE
#define XL_USE_PCRE
#endif


#include "exceptions.h"
#include "regexer.h"
#include "library_extensions.h"

namespace xl::json {

inline xl::RegexPcre json_regex(R"REGEX(
\A
(?(DEFINE)(?<StringContents>(?:\\"|[^"])*))
(?(DEFINE)(?<comment>(\s*(
    (?:
        \/\/(?>[^\n]*$)|
        \/\*.*?\*\/)? 
    )
)*
))
(?&comment)
(?<any>\s*
   (?<number>(?:-?\d+\.?\d*|-?\.\d+)([Ee][+-]?\d+)?) |
   (?<boolean>true|false) |
   (?<StringQuote>"|')(?<string>(?:\\\k<StringQuote>|.)*?)(?:\k<StringQuote>) |

   # Array
   (?<array>\[\s*(?<ArrayGuts>(?>(?:(?<ArrayHead>(?&any))(?&comment)
       (?:,(?&comment)\s*(?<ArrayTail>((?&ArrayGuts)\s*)))?)),?)?\s*
   \]) |

   # Object
   (?<object>{(?&comment)\s*(?<ObjectGuts>

(?>(
 (?: # Quoted string or unquoted string
(?<StringQuote2>"|')(?<Key>(?:\\\k<StringQuote2>|.)*?)(?:\k<StringQuote2>) |
(?<Key>\w+?)


)\s*:\s*(?<Value>(?&any))(?&comment)


       (?:,(?&comment)\s*(?<ObjectTail>((?&ObjectGuts)\s*)))?)),?)?\s*
   (?&comment)}) |

   (?<null>null)
\s*) # end any
(?&comment)
\Z
)REGEX",  xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL | xl::MULTILINE | ALLOW_DUPLICATE_SUBPATTERN_NAMES);


inline xl::RegexPcre escaped_character_regex("\\\\(.)", xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);

class JsonException : public xl::FormattedException {
    using xl::FormattedException::FormattedException;
};


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

    Json & operator=(Json const &) = default;
    Json & operator=(Json &&) = default;


    /**
     * Parses content as JSON, returning the regex match info
     * @throw JsonException if the json is invalid
     * @return regex matches against content
     */
    xl::RegexResultPcre parse() const {
        // an empty source will return an empty optional for all API calls
        if (this->source.empty()) {
            return xl::RegexResultPcre();
        }
        auto matches = json_regex.match(this->source);
        if (!matches) {
            throw JsonException("invalid, non-empty json");
        }
        return matches;
    }


    std::optional<double> get_number(std::optional<double> alternate_number = std::optional<double>{}) const {
        auto matches = parse();

        if (auto number = matches["number"]; number.empty()) {
            if (alternate_number) {
                return *alternate_number;
            } else {
                return {};
            }
        } else {
            return std::stod(matches["number"]);
        }
    }

    std::optional<std::string> get_string(std::optional<std::string> alternate_string = std::optional<std::string>{}) const {
        try {
            std::string result = parse()["string"];
            if (!result.empty()) {
                return escaped_character_regex.replace(result, "$1", true);
            }
        }
            // if there's no string at this point, check if an alternate_string was specified
        catch (JsonException const &) {
            if (!alternate_string) {
                throw;
            }
        }
        if (alternate_string) {
            return alternate_string;
        } else {
            return {};
        }

    }

#if 0
    // previous impl
    std::string result = parse()["string"];
        if (!result.empty()) {
            return escaped_character_regex.replace(result, "$1", true);
        } else {
            return {};
        }
#endif

    std::optional<std::map<std::string, Json>> get_object() const {
        auto matches = parse();
        if (matches.has("object")) {
            std::map<std::string, Json> results;
            std::string object_string = matches["object"];
//            std::cerr << fmt::format("object string: {}", object_string) << std::endl;
            while(auto object_entry_match = json_regex.match(object_string)) {
                for (auto tuple : xl::each_i(object_entry_match.get_all_matches())) {
                    auto match = std::get<0>(tuple);
                    int i = std::get<1>(tuple);
//                    std::cerr << fmt::format("match {}: {}", i, match) << std::endl;
                }
                std::string key = object_entry_match["Key"];
//                std::cerr << fmt::format("object entry match for key: {}", key) << std::endl;
//                std::cerr << fmt::format("object entry amtch for value: {}", object_entry_match["Value"]) << std::endl;

                results.emplace(escaped_character_regex.replace(key, "$1", true), Json(object_entry_match["Value"]));
                if (!object_entry_match.has("ObjectTail")) {
                    break;
                }
                object_string = std::string("{") + std::string(object_entry_match["ObjectTail"]) + "}";
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
                if (array_entry_match.has("ArrayHead")) {
                    results.emplace_back(array_entry_match["ArrayHead"]);
                }
                if (!array_entry_match.has("ArrayTail")) {
                    break;
                }
                array_string = std::string("[" +  std::string(array_entry_match["ArrayTail"]) + "]");
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

    std::optional<bool> get_boolean(std::optional<bool> alternate_bool = std::optional<bool>{}) const {
        std::string result = parse()["boolean"];
        return result.empty() ? alternate_bool : result == "true";
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
            auto result = (*object)[name];
//            std::cerr << fmt::format("Looking up key {} got json: {}", name, result.source) << std::endl;

            return result;
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
        try {
            return this->parse();
        } catch (JsonException &) {
            return false;
        }
    }

    operator bool() const {
        return this->is_valid();
    }
};



} // end namespace xl::json




