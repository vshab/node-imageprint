#include <string>

namespace PrintSchema
{
    struct PageMediaSize
    {
        std::wstring displayName;
        std::wstring name;
        unsigned int height;
        unsigned int width;
    };

    struct PageImageableSize
    {
        struct ImageableArea
        {
            unsigned int extentHeight;
            unsigned int extentWidth;
            unsigned int originHeight;
            unsigned int originWidth;
        };

        ImageableArea imageableArea;
        unsigned int height;
        unsigned int width;
    };
}
