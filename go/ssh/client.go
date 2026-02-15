package main

/*
#cgo CFLAGS: -I../../../include/pnana/features
#include <stdlib.h>
#include <string.h>

// C 接口结构体定义（与 types.go 中的定义保持一致）
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
import (
	"fmt"
	"os"
	"unsafe"
)

//export ConnectAndReadFile
func ConnectAndReadFile(config *C.SSHConfig_C) *C.SSHResult_C {
	result := createSSHResult()
	result.success = 0
	result.content = nil
	result.error = nil

	opConfig := parseSSHConfig(config)

	// 使用连接池获取连接
	client, err := getSSHConnection(opConfig.host, opConfig.port, opConfig.user, opConfig.password, opConfig.keyPath)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to connect: %v", err))
		return result
	}
	defer releaseSSHConnection(opConfig.host, opConfig.port, opConfig.user)

	// 读取远程文件
	content, err := readRemoteFile(client, opConfig.remotePath)
	if err != nil {
		setResultError(result, err)
		return result
	}

	setResultSuccess(result, content)
	return result
}

//export ConnectAndWriteFile
func ConnectAndWriteFile(config *C.SSHConfig_C, content *C.char) *C.SSHResult_C {
	result := createSSHResult()
	result.success = 0
	result.content = nil
	result.error = nil

	opConfig := parseSSHConfig(config)
	fileContent := C.GoString(content)

	// 使用连接池获取连接
	client, err := getSSHConnection(opConfig.host, opConfig.port, opConfig.user, opConfig.password, opConfig.keyPath)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to connect: %v", err))
		return result
	}
	defer releaseSSHConnection(opConfig.host, opConfig.port, opConfig.user)

	// 写入远程文件
	err = writeRemoteFile(client, opConfig.remotePath, fileContent)
	if err != nil {
		setResultError(result, err)
		return result
	}

	result.success = 1
	return result
}

//export UploadFile
func UploadFile(config *C.SSHConfig_C, localPath *C.char, remotePath *C.char) *C.SSHResult_C {
	result := createSSHResult()
	result.success = 0
	result.content = nil
	result.error = nil

	opConfig := parseSSHConfig(config)
	localFilePath := C.GoString(localPath)
	remoteFilePath := C.GoString(remotePath)

	// 使用连接池获取连接
	client, err := getSSHConnection(opConfig.host, opConfig.port, opConfig.user, opConfig.password, opConfig.keyPath)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to connect: %v", err))
		return result
	}
	defer releaseSSHConnection(opConfig.host, opConfig.port, opConfig.user)

	// 读取本地文件
	localData, err := os.ReadFile(localFilePath)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to read local file: %v", err))
		return result
	}

	// 上传文件到远程
	err = uploadFileToRemote(client, remoteFilePath, localData)
	if err != nil {
		setResultError(result, err)
		return result
	}

	result.success = 1
	return result
}

//export DownloadFile
func DownloadFile(config *C.SSHConfig_C, remotePath *C.char, localPath *C.char) *C.SSHResult_C {
	result := createSSHResult()
	result.success = 0
	result.content = nil
	result.error = nil

	opConfig := parseSSHConfig(config)
	remoteFilePath := C.GoString(remotePath)
	localFilePath := C.GoString(localPath)

	// 使用连接池获取连接
	client, err := getSSHConnection(opConfig.host, opConfig.port, opConfig.user, opConfig.password, opConfig.keyPath)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to connect: %v", err))
		return result
	}
	defer releaseSSHConnection(opConfig.host, opConfig.port, opConfig.user)

	// 从远程下载文件
	fileContent, err := downloadFileFromRemote(client, remoteFilePath)
	if err != nil {
		setResultError(result, err)
		return result
	}

	// 写入本地文件
	err = os.WriteFile(localFilePath, fileContent, 0644)
	if err != nil {
		setResultError(result, fmt.Errorf("failed to write local file: %v", err))
		return result
	}

	result.success = 1
	return result
}

//export FreeSSHResult
func FreeSSHResult(result *C.SSHResult_C) {
	if result != nil {
		if result.content != nil {
			C.free(unsafe.Pointer(result.content))
		}
		if result.error != nil {
			C.free(unsafe.Pointer(result.error))
		}
		C.free(unsafe.Pointer(result))
	}
}

func main() {
	// Go 库，不需要 main 函数
}

