//
//  GDBHandler.cpp
//  GameboyEmu
//
//  Created by David Spickett on 19/04/2017.
//  Copyright © 2017 David Spickett. All rights reserved.
//

#include "GDBHandler.hpp"
#include <stdexcept>
#include "utils.hpp"
#include <errno.h>
#include <sys/select.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

namespace
{
    int wrap_socket_err(std::string func_name, int ret)
    {
        if (ret < 0)
        {
            throw std::runtime_error(formatted_string("%s returned %d, errno \"%s\" (%d)", func_name.c_str(), ret, strerror(errno), errno));
        }
        return ret;
    }
    
    char *get_ip_str(const struct sockaddr *sa, char *s, unsigned maxlen)
    {
        switch(sa->sa_family) {
            case AF_INET:
                inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                          s, maxlen);
                break;
                
            case AF_INET6:
                inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                          s, maxlen);
                break;
                
            default:
                strncpy(s, "Unknown AF", maxlen);
                return NULL;
        }
        
        return s;
    }
}

GDBHandler::~GDBHandler()
{
    if (m_client_fd != -1)
    {
        close(m_client_fd);
    }
}

GDBHandler::GDBHandler():
    m_client_fd(-1)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    char gdb_port[] = "1234";
    wrap_socket_err("getaddrinfo", getaddrinfo(NULL, gdb_port, &hints, &res));
    
    //Open socket, bind and listen
    int sockfd = wrap_socket_err("socket",
                    socket(res->ai_family,
                    res->ai_socktype,
                    res->ai_protocol));
    wrap_socket_err("bind", bind(sockfd, res->ai_addr, res->ai_addrlen));
    wrap_socket_err("listen", listen(sockfd, 1));
    
    //Wait for a connection
    fd_set sock_set;
    FD_ZERO(&sock_set);
    FD_SET(sockfd, &sock_set);
    timeval timeout;
    timeout.tv_sec = 1;
    
    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    addr_size = sizeof their_addr;
    printf("Waiting for GDB connection on localhost:%s...\n", gdb_port);
    m_client_fd = wrap_socket_err("accept", accept(sockfd, (struct sockaddr *)&their_addr, &addr_size));
    close(sockfd);
    
    char connected[INET6_ADDRSTRLEN];
    printf("Connected to %s\n",
           get_ip_str((struct sockaddr *)&their_addr, connected, INET6_ADDRSTRLEN));
}

void GDBHandler::client_read()
{
    if (m_client_fd == -1)
    {
        return;
    }
    
    char received[200];
    
    while (1)
    {
        fd_set sockset;
        FD_ZERO(&sockset);
        FD_SET(m_client_fd, &sockset);
        //Note that the time is sometimes modified by select
        //and that both values must be initialised. (sec and usec)
        timeval t = {0, 100};
        
        int result = select(m_client_fd + 1, &sockset, NULL, NULL, &t);
        
        if (result < 0)
        {
            printf("Error handling GDB connection, %d: %s\n", errno, strerror(errno));
            m_client_fd = -1;
            return;
        }
        else if (result == 0)
        {
            break;
        }
        else
        {
            size_t got = read(m_client_fd, received, 200);
            //For now assume we don't get fragmentation
            handle_command(received, got);
        }
    }
}

namespace
{
    bool verify_gdb_packet_structure(char* received, size_t size)
    {
        if (received[0] != '$')
        {
            return false;
        }
        
        if (received[size-3] != '#')
        {
            return false;
        }
        
        char checksum[3];
        checksum[0] = received[size-2];
        checksum[1] = received[size-1];
        checksum[2] = 0;
        uint8_t expected_checksum = strtol(checksum, NULL, 16);
        
        //Checksum is the sum of the packet data only, not the $/# and checksum digits
        uint8_t got_checksum = 0;
        char* packet_data = received+1;
        char* end_packet_data = packet_data + (size-4);
        for ( ; packet_data != end_packet_data; ++packet_data)
        {
            got_checksum += *packet_data;
        }
        
        return got_checksum == expected_checksum;
    }
}

void GDBHandler::send_to_client(char* data, size_t len)
{
    if (wrap_socket_err("send", int(send(m_client_fd, &data, len, 0))) != len)
    {
        throw std::runtime_error("Couldn't send whole packet!");
    }
}

void GDBHandler::handle_command(char* received, size_t size)
{
    //Not sure why we get this at the start
    if (received[0] == '+')
    {
        received++;
        size--;
    }
    
    bool good_packet = verify_gdb_packet_structure(received, size);
    char response = good_packet ? '+' : '-';
    send_to_client(&response, 1);
}
