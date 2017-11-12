#pragma once

namespace xl::templates {

class TemplateException : public xl::FormattedException {

public:

    using xl::FormattedException::FormattedException;

};


} // end namespace xl