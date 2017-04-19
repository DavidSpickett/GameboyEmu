//
//  GDBHandler.hpp
//  GameboyEmu
//
//  Created by David Spickett on 19/04/2017.
//  Copyright © 2017 David Spickett. All rights reserved.
//

#ifndef GDBHandler_hpp
#define GDBHandler_hpp

#include <stdio.h>

class GDBHandler
{
public:
    GDBHandler();
    ~GDBHandler();
    
    void handle_command(char* received, size_t size);
    void client_read();
    
private:
    void process_command(char* data, size_t len);
    void send_to_client(char* data, size_t len);
    
    int m_client_fd;
};

#endif /* GDBHandler_hpp */
