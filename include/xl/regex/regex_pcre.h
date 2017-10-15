#pragma once

#if defined XL_USE_PCRE

#include <pcre.h>

#include <fmt/ostream.h>

#include "zstring_view.h"

namespace xl {


class RegexPcre {

    pcre * compiled_regex = nullptr;
    pcre_extra * extra = nullptr;
    std::unique_ptr<int[]> results;
    int result_size = 0;

public:

    RegexPcre(xl::zstring_view regex_string, xl::RegexFlags flags = NONE)
    {
        const char *error_string;
        int error_offset;

        compiled_regex = pcre_compile(regex_string.c_str(),
                                      0, // options
                                      &error_string,
                                      &error_offset,
                                      NULL // table pointer ??
        );

        if (compiled_regex == nullptr) {
            // TODO: ERROR
        }

        if (flags | OPTIMIZE) {
            char const ** pcre_study_error_message;
            this->extra = pcre_study(compiled_regex, 0, pcre_study_error_message);
            if (this->extra == nullptr) {
                // TODO: ERROR
            }
        }

        pcre_fullinfo(this->compiled_regex, this->extra, PCRE_INFO_CAPTURECOUNT, &this->result_size);
        std::cerr << fmt::format("found capture count: {}", this->result_size) << std::endl;
        this->result_size = (this->result_size + 1) * 3; // need 3 entries per capture
        this->results = std::make_unique<int[]>(this->result_size);

    }


    void match(xl::zstring_view data) {

        auto result = pcre_exec(this->compiled_regex,
                                this->extra,
                                data.c_str(),
                                data.length(),  // length of string
                                0,                      // Start looking at this point
                                0,                      // OPTIONS
                                this->results.get(),
                                this->result_size);                    // Length of subStrVec

        if (result < 0) {
            // TODO: ERROR
        } else {
            // this means size of this->results was too small but it should have been allocated large enough
            assert(result > 0);

            for(int j=0; j<result; j++) {
                char const * submatch_string;
                pcre_get_substring(data.c_str(), this->results.get(), result, j, &(submatch_string));
                printf("Match(%2d/%2d): (%2d,%2d): '%s'\n", j, result-1, this->results[j*2], this->results[j*2+1], submatch_string);
            } /* end for */

        }

    }

    ~RegexPcre() {
        //pcre_free_study(this->extra);
        pcre_free(this->compiled_regex);
    }

};

#endif

} // end namespace xl