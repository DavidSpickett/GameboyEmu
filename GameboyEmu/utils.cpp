//
//  utils.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "utils.hpp"

template< typename... Args >
std::string formatted_string(const char* format, Args... args)
{
    //Nullptr doesn't write anything, just returns length needed
    /*int length = std::snprintf(nullptr, 0, format, args...);
    
    if (length < 0)
    {
        throw "Error formatting string.";
    }
    
    char* buf = new char[length + 1];
    std::snprintf( buf, length + 1, format, args... );
    
    std::string str(buf);
    delete[] buf;
    return std::move(str);*/
    return "foo";
}
