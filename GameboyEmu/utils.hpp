//
//  utils.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

#include <stdio.h>
#include <string>

/*template< typename... Args >
std::string formatted_string(const char* format, Args... args);*/

template <typename T>
std::string formatted_string_(const char* format, T arg)
{
    return "foo";
}

#endif /* utils_hpp */
