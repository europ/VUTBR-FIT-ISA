#ifndef __constants_hpp
#define __constants_hpp

#define PORT_MAX              65535  // maximum of port number
#define HOSTNAME_LENGTH       64     // maximum of hostname (chars)
#define TIMEOUT_SECONDS       600    // 10 minutes
#define LOG_FILE_NAME         "log"  // name of log file
#define DATA_FILE_NAME        "data" // name of data file
#define DATA_FILE_DELIMITER   "/"    // delimiter used in data file
#define ID_LENGTH             30     // unique-id length (chars)
#define THREAD_RECV_BUFF_SIZE 1024   // char buff[SIZE]

// unique-id char set excluding slash
#define ID_CHARS "!\"#$%&'()*+,-."/*SLASH*/"0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

#endif
