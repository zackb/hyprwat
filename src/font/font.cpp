#include "font.hpp"
#include <fontconfig/fontconfig.h>

namespace font {
    std::string defaultFontPath() {
        FcInit();
        FcPattern* pat = FcPatternCreate();
        FcPatternAddString(pat, FC_FAMILY, (const FcChar8*)"sans");
        FcConfigSubstitute(nullptr, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcResult result;
        FcPattern* match = FcFontMatch(nullptr, pat, &result);
        std::string path;

        if (match) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
                path = (const char*)file;
            }
            FcPatternDestroy(match);
        }

        FcPatternDestroy(pat);
        FcFini();

        return path;
    }
} // namespace font
