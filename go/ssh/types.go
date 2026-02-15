package main

/*
#cgo CFLAGS: -I../../../include/pnana/features
#include <stdlib.h>
#include <string.h>

// C 接口结构体定义
typedef struct {
    char* host;
    char* user;
    char* password;
    char* key_path;
    int port;
    char* remote_path;
} SSHConfig_C;

typedef struct {
    int success;
    char* content;
    char* error;
} SSHResult_C;
*/
import "C"
