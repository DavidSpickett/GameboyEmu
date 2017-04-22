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
#include <fstream>

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

GDBHandler::GDBHandler(Z80& proc):
    m_client_fd(-1), m_proc(proc)
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
    
    int set_option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&set_option,
               sizeof(set_option));
    
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
            
            if (got)
            {
                char got_str[got+1];
                strncpy(got_str, received, got);
                got_str[got] = 0;
                printf("RECEIVED: %s\n", got_str);
            
                if (got >= 4)
                {
                    //For now assume we don't get fragmentation
                    handle_command(received, got);
                }
            }
        }
    }
}

namespace
{
    //Checksum is the sum of the packet data only, not the $/# and checksum digits
    uint8_t calculate_checksum(const char* buffer, size_t len)
    {
        uint8_t checksum = 0;
        const char* end_data = buffer + len;
        for ( ; buffer != end_data; ++buffer)
        {
            checksum += *buffer;
        }
        return checksum;
    }
    
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
        uint8_t got_checksum = calculate_checksum(received+1, size-4);
        
        return got_checksum == expected_checksum;
    }
    
    bool compare_command(std::string cmd, const char* data, size_t len)
    {
        return (len >= cmd.size()) &&
                 (strncmp(data, cmd.c_str(), cmd.size()) == 0);
    }
}

void GDBHandler::send_gdb_response(const char* data)
{
    size_t len = strlen(data);
    size_t total_size = len+4;
    char packet[total_size+1]; //+1 to print later
    
    packet[0] = '$';
    strncpy(&packet[1], data, len);
    packet[total_size-3] = '#';
    
    uint8_t checksum = calculate_checksum(data, len);
    char checksum_str[3];
    sprintf(checksum_str, "%02x", checksum);
    strncpy(packet+2+len, checksum_str, 2);
    
    send_to_client(packet, total_size);
    packet[total_size] = 0;
    printf("    SENT: %s\n", packet);
    
    
}

void GDBHandler::send_to_client(char* data, size_t len)
{
    if (wrap_socket_err("send", int(send(m_client_fd, data, len, 0))) != len)
    {
        throw std::runtime_error("Couldn't send whole packet!");
    }
}

void GDBHandler::handle_command(char* received, size_t size)
{
    //Ignore client acks
    if (received[0] == '+')
    {
        received++;
        size--;
    }
    if (received[size-1] == '+')
    {
        size--;
    }
    
    bool good_packet = verify_gdb_packet_structure(received, size);
    char response = good_packet ? '+' : '-';
    send_to_client(&response, 1);
    printf("ACK %c\n", response);
    
    if (good_packet)
    {
        process_command(received+1, size-4);
    }
}

#define IS_COMMAND(cmd) compare_command(cmd, data, len)
#define RESPOND(response) char s[] = response; send_gdb_response(s)

void GDBHandler::process_command(char* data, size_t len)
{
    //What do we support?
    if (IS_COMMAND("qSupported"))
    {
        RESPOND("PacketSize=500;qXfer:features:read-");
    }
    //Just respond with nothing, not sure what this does
    else if (IS_COMMAND("vMustReplyEmpty"))
    {
        RESPOND("");
    }
    //Set thread number for subsequent operations (should always be 0 here)
    else if (IS_COMMAND("Hg"))
    {
        RESPOND("OK");
    }
    //Why did we halt?
    else if (IS_COMMAND("?"))
    {
        RESPOND("S05");
    }
    //Get list of threads
    else if (IS_COMMAND("qfThreadInfo"))
    {
        RESPOND("m01");
    }
    //Finish list of threads
    else if (IS_COMMAND("qsThreadInfo"))
    {
        RESPOND("l");
    }
    //Set thread for subsequent operations (don't care what or the thread for this)
    else if (IS_COMMAND("Hc"))
    {
        RESPOND("OK");
    }
    //Get current thread ID
    else if (IS_COMMAND("qC"))
    {
        RESPOND("QC01");
    }
    /*else if (IS_COMMAND("qXfer:features:read:target.xml:0,4fb"))
    {
        std::ifstream file_str = std::ifstream("target.xml", std::ios::in);
        if (!file_str.is_open())
        {
            throw std::runtime_error("Cannot find GDB target.xml");
        }
        
        std::string str;
        std::string file_contents = "l";
        while (std::getline(file_str, str))
        {
            file_contents += str;
            file_contents.push_back('\n');
        }
        
        //Need to escape this eventually.
        send_gdb_response(file_contents.c_str());
    }*/
    //Get register block
    /*else if (IS_COMMAND("g"))
    {
        char regblock[9];
        sprintf(regblock, "%08x", m_proc.pc.read());
        send_gdb_response(regblock);
    }*/
    //Read the PC/others (8=pc)
    else if (IS_COMMAND("p"))
    {
        char regblock[9];
        sprintf(regblock, "%08x", m_proc.pc.read());
        send_gdb_response(regblock);
        
        //It thinks this is 32 bit...I think...
        //RESPOND("DEADCAFE");
    }
    //Read some memory
    else if (IS_COMMAND("m"))
    {
        RESPOND("11223344");
    }
    //Anything else respond with 0 for unsupported
    else
    {
        RESPOND("");
    }
}
