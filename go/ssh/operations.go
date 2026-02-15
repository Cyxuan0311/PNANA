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
	"bytes"
	"fmt"
	"os"
	"unsafe"

	"golang.org/x/crypto/ssh"
)

// SSH操作配置
type sshOperationConfig struct {
	host       string
	user       string
	password   string
	keyPath    string
	port       int
	remotePath string
}

// 从C配置解析操作配置
func parseSSHConfig(config *C.SSHConfig_C) sshOperationConfig {
	port := int(config.port)
	if port == 0 {
		port = 22
	}

	return sshOperationConfig{
		host:       C.GoString(config.host),
		user:       C.GoString(config.user),
		password:   C.GoString(config.password),
		keyPath:    C.GoString(config.key_path),
		port:       port,
		remotePath: C.GoString(config.remote_path),
	}
}

// 创建C结果结构
func createSSHResult() *C.SSHResult_C {
	return (*C.SSHResult_C)(C.malloc(C.size_t(unsafe.Sizeof(C.SSHResult_C{}))))
}

// 设置结果错误
func setResultError(result *C.SSHResult_C, err error) {
	result.success = 0
	result.error = C.CString(err.Error())
}

// 设置结果成功
func setResultSuccess(result *C.SSHResult_C, content string) {
	result.success = 1
	result.content = C.CString(content)
}

// 读取远程文件
func readRemoteFile(client *ssh.Client, remotePath string) (string, error) {
	session, err := client.NewSession()
	if err != nil {
		return "", fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	var stdout bytes.Buffer
	session.Stdout = &stdout
	err = session.Run(fmt.Sprintf("cat %s", remotePath))
	if err != nil {
		return "", fmt.Errorf("failed to read file: %v", err)
	}

	return stdout.String(), nil
}

// 写入远程文件
func writeRemoteFile(client *ssh.Client, remotePath string, content string) error {
	session, err := client.NewSession()
	if err != nil {
		return fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	// 使用 base64 编码内容以避免特殊字符问题
	encodedContent := base64Encode(content)

	// 写入文件（使用 base64 解码）
	cmd := fmt.Sprintf("echo '%s' | base64 -d > %s", encodedContent, remotePath)
	err = session.Run(cmd)
	if err != nil {
		// 如果 base64 命令失败，使用 heredoc 方式（需要转义）
		escapedContent := escapeShellString(content)
		// 使用随机分隔符避免内容冲突
		delimiter := fmt.Sprintf("PNANA_EOF_%d", os.Getpid())
		cmd = fmt.Sprintf("cat > %s << '%s'\n%s\n%s", remotePath, delimiter, escapedContent, delimiter)
		err = session.Run(cmd)
		if err != nil {
			return fmt.Errorf("failed to write file: %v", err)
		}
	}

	return nil
}

// 上传文件到远程
func uploadFileToRemote(client *ssh.Client, remotePath string, localData []byte) error {
	session, err := client.NewSession()
	if err != nil {
		return fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	// 使用 base64 编码上传文件内容
	encodedContent := base64Encode(string(localData))

	// 创建远程文件并写入内容
	cmd := fmt.Sprintf("echo '%s' | base64 -d > '%s'", encodedContent, remotePath)
	err = session.Run(cmd)
	if err != nil {
		// 如果 base64 命令失败，使用 heredoc 方式
		escapedContent := escapeShellString(string(localData))
		delimiter := fmt.Sprintf("PNANA_UPLOAD_EOF_%d", os.Getpid())
		cmd = fmt.Sprintf("cat > '%s' << '%s'\n%s\n%s", remotePath, delimiter, escapedContent, delimiter)
		err = session.Run(cmd)
		if err != nil {
			return fmt.Errorf("failed to upload file: %v", err)
		}
	}

	return nil
}

// 从远程下载文件
func downloadFileFromRemote(client *ssh.Client, remotePath string) ([]byte, error) {
	session, err := client.NewSession()
	if err != nil {
		return nil, fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	// 读取远程文件
	var stdout bytes.Buffer
	session.Stdout = &stdout
	err = session.Run(fmt.Sprintf("cat '%s'", remotePath))
	if err != nil {
		return nil, fmt.Errorf("failed to read remote file: %v", err)
	}

	return stdout.Bytes(), nil
}

