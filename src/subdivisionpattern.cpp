#include "subdivisionpattern.h"
#include <vector>

std::vector<SubdivisionPattern> builtinSubdivisionPatterns = {
    {
        SubdivisionCategory::Standard,  // category
        "Quarter Note",                 // name
        QVector<SubdivisionPulse>{ {1.0, false, false} } // duration, isRest, accent
    },
    {
        SubdivisionCategory::Standard,
        "Eighth Note",
        QVector<SubdivisionPulse>{ {0.5, false, false}, {0.5, false, false} }
    }
    // ...add more patterns as needed
};