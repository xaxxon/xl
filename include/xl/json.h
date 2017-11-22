#pragma once

#include <map>
#include <optional>

#include "exceptions.h"
#include "regexer.h"

namespace xl::json {

inline xl::RegexPcre json_regex(R"REGEX(
^
(?(DEFINE)(?<ObjectEntry>(\s*"[^"]*"\s*:\s*(?&any)\s*)))
(?(DEFINE)(?<ArrayEntry>(\s*(?&any)\s*)))
\s*
(?<any>
   (?<number>(?:-?\d+\.?\d*|-?\.\d+)([Ee][+-]?\d+)?) |
   (?<boolean>true|false) |
   (?:"(?<string>.*?)") |
   (?<array>\[(?:(?<ArrayHead>(?&ArrayEntry))\s*,?\s*)?(?<ArrayTail>((?&ArrayEntry)\s*,?\s*)*)\]) |
   (?<object>{((\s*"(?<Key>.*?)"\s*:\s*(?<Value>(?&any))\s*)\s*,?\s*)?(?<ObjectTail>(?&ObjectEntry)\s*,?\s*)* }) |
   (?<null>null)
)
\s*
$
)REGEX",  xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);


inline xl::RegexPcre escaped_character_regex("\\\\(.)", xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);

struct JsonException : public xl::FormattedException {
    using xl::FormattedException::FormattedException;
};


struct Json {
    std::string source;

    // needed for operator[] in a map<string, Json>
    Json() = default;

    Json(std::string source) : source(source) {}
    Json(Json const &) = default;
    Json(Json &&) = default;

    xl::RegexResultPcre parse() const {
        auto matches = json_regex.match(this->source);
        if (!matches) {
            throw JsonException("Invalid json: {}", this->source);
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

    std::optional<bool> get_boolean() const {
        auto result = parse()["boolean"];
        return !result.empty() ? result == "true" : std::optional<bool>{};
    }

    bool is_null() const {
        return parse().has("null");
    }

    bool is_valid() {
        try {
            this->parse();
            return true;
        } catch (JsonException & e) {
            return false;
        }
    }
};

} // end namespace xl::json




