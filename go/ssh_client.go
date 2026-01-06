package main

/*
#cgo CFLAGS: -I../../include/pnana/features
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
import (
	"bytes"
	"crypto/rsa"
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"fmt"
	"os"
	"strings"
	"unsafe"

	"golang.org/x/crypto/ssh"
)

//export ConnectAndReadFile
func ConnectAndReadFile(config *C.SSHConfig_C) *C.SSHResult_C {
	result := (*C.SSHResult_C)(C.malloc(C.size_t(unsafe.Sizeof(C.SSHResult_C{}))))
	result.success = 0
	result.content = nil
	result.error = nil

	host := C.GoString(config.host)
	user := C.GoString(config.user)
	password := C.GoString(config.password)
	keyPath := C.GoString(config.key_path)
	port := int(config.port)
	remotePath := C.GoString(config.remote_path)

	if port == 0 {
		port = 22
	}

	// 构建 SSH 客户端配置
	authMethods := []ssh.AuthMethod{}

	// 优先使用密钥认证
	if keyPath != "" {
		key, err := loadPrivateKey(keyPath)
		if err == nil {
			signer, err := ssh.NewSignerFromKey(key)
			if err == nil {
				authMethods = append(authMethods, ssh.PublicKeys(signer))
			}
		}
	}

	// 如果提供了密码，也添加密码认证
	if password != "" {
		authMethods = append(authMethods, ssh.Password(password))
	}

	if len(authMethods) == 0 {
		result.error = C.CString("No authentication method provided")
		return result
	}

	sshConfig := &ssh.ClientConfig{
		User:            user,
		Auth:            authMethods,
		HostKeyCallback: ssh.InsecureIgnoreHostKey(), // 仅用于开发，生产环境应验证主机密钥
	}

	// 连接到 SSH 服务器
	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, sshConfig)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to connect: %v", err))
		return result
	}
	defer client.Close()

	// 创建会话
	session, err := client.NewSession()
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to create session: %v", err))
		return result
	}
	defer session.Close()

	// 读取远程文件
	var stdout bytes.Buffer
	session.Stdout = &stdout
	err = session.Run(fmt.Sprintf("cat %s", remotePath))
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to read file: %v", err))
		return result
	}

	content := stdout.String()
	result.content = C.CString(content)
	result.success = 1

	return result
}

//export ConnectAndWriteFile
func ConnectAndWriteFile(config *C.SSHConfig_C, content *C.char) *C.SSHResult_C {
	result := (*C.SSHResult_C)(C.malloc(C.size_t(unsafe.Sizeof(C.SSHResult_C{}))))
	result.success = 0
	result.content = nil
	result.error = nil

	host := C.GoString(config.host)
	user := C.GoString(config.user)
	password := C.GoString(config.password)
	keyPath := C.GoString(config.key_path)
	port := int(config.port)
	remotePath := C.GoString(config.remote_path)
	fileContent := C.GoString(content)

	if port == 0 {
		port = 22
	}

	// 构建 SSH 客户端配置
	authMethods := []ssh.AuthMethod{}

	if keyPath != "" {
		key, err := loadPrivateKey(keyPath)
		if err == nil {
			signer, err := ssh.NewSignerFromKey(key)
			if err == nil {
				authMethods = append(authMethods, ssh.PublicKeys(signer))
			}
		}
	}

	if password != "" {
		authMethods = append(authMethods, ssh.Password(password))
	}

	if len(authMethods) == 0 {
		result.error = C.CString("No authentication method provided")
		return result
	}

	sshConfig := &ssh.ClientConfig{
		User:            user,
		Auth:            authMethods,
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	// 连接到 SSH 服务器
	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, sshConfig)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to connect: %v", err))
		return result
	}
	defer client.Close()

	// 创建会话并写入文件
	session, err := client.NewSession()
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to create session: %v", err))
		return result
	}
	defer session.Close()

	// 使用 base64 编码内容以避免特殊字符问题
	encodedContent := base64Encode(fileContent)

	// 写入文件（使用 base64 解码）
	cmd := fmt.Sprintf("echo '%s' | base64 -d > %s", encodedContent, remotePath)
	err = session.Run(cmd)
	if err != nil {
		// 如果 base64 命令失败，使用 heredoc 方式（需要转义）
		escapedContent := escapeShellString(fileContent)
		// 使用随机分隔符避免内容冲突
		delimiter := fmt.Sprintf("PNANA_EOF_%d", os.Getpid())
		cmd = fmt.Sprintf("cat > %s << '%s'\n%s\n%s", remotePath, delimiter, escapedContent, delimiter)
		err = session.Run(cmd)
		if err != nil {
			result.error = C.CString(fmt.Sprintf("Failed to write file: %v", err))
			return result
		}
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

// 加载私钥
func loadPrivateKey(keyPath string) (*rsa.PrivateKey, error) {
	keyData, err := os.ReadFile(keyPath)
	if err != nil {
		return nil, err
	}

	block, _ := pem.Decode(keyData)
	if block == nil {
		return nil, fmt.Errorf("failed to decode PEM block")
	}

	// 尝试解析 PKCS1 格式
	key, err := x509.ParsePKCS1PrivateKey(block.Bytes)
	if err == nil {
		return key, nil
	}

	// 尝试解析 PKCS8 格式
	parsedKey, err := x509.ParsePKCS8PrivateKey(block.Bytes)
	if err != nil {
		return nil, err
	}

	rsaKey, ok := parsedKey.(*rsa.PrivateKey)
	if !ok {
		return nil, fmt.Errorf("not an RSA private key")
	}

	return rsaKey, nil
}

// Base64 编码
func base64Encode(s string) string {
	return base64.StdEncoding.EncodeToString([]byte(s))
}

// 转义 shell 字符串
func escapeShellString(s string) string {
	// 转义特殊字符
	s = strings.ReplaceAll(s, "\\", "\\\\")
	s = strings.ReplaceAll(s, "$", "\\$")
	s = strings.ReplaceAll(s, "`", "\\`")
	s = strings.ReplaceAll(s, "\"", "\\\"")
	return s
}

//export UploadFile
func UploadFile(config *C.SSHConfig_C, localPath *C.char, remotePath *C.char) *C.SSHResult_C {
	result := (*C.SSHResult_C)(C.malloc(C.size_t(unsafe.Sizeof(C.SSHResult_C{}))))
	result.success = 0
	result.content = nil
	result.error = nil

	host := C.GoString(config.host)
	user := C.GoString(config.user)
	password := C.GoString(config.password)
	keyPath := C.GoString(config.key_path)
	port := int(config.port)
	localFilePath := C.GoString(localPath)
	remoteFilePath := C.GoString(remotePath)

	if port == 0 {
		port = 22
	}

	// 构建 SSH 客户端配置
	authMethods := []ssh.AuthMethod{}

	if keyPath != "" {
		key, err := loadPrivateKey(keyPath)
		if err == nil {
			signer, err := ssh.NewSignerFromKey(key)
			if err == nil {
				authMethods = append(authMethods, ssh.PublicKeys(signer))
			}
		}
	}

	if password != "" {
		authMethods = append(authMethods, ssh.Password(password))
	}

	if len(authMethods) == 0 {
		result.error = C.CString("No authentication method provided")
		return result
	}

	sshConfig := &ssh.ClientConfig{
		User:            user,
		Auth:            authMethods,
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	// 连接到 SSH 服务器
	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, sshConfig)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to connect: %v", err))
		return result
	}
	defer client.Close()

	// 读取本地文件
	localData, err := os.ReadFile(localFilePath)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to read local file: %v", err))
		return result
	}

	// 创建远程文件
	session, err := client.NewSession()
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to create session: %v", err))
		return result
	}
	defer session.Close()

	// 使用 base64 编码上传文件内容
	encodedContent := base64Encode(string(localData))

	// 创建远程文件并写入内容
	cmd := fmt.Sprintf("echo '%s' | base64 -d > '%s'", encodedContent, remoteFilePath)
	err = session.Run(cmd)
	if err != nil {
		// 如果 base64 命令失败，使用 heredoc 方式
		escapedContent := escapeShellString(string(localData))
		delimiter := fmt.Sprintf("PNANA_UPLOAD_EOF_%d", os.Getpid())
		cmd = fmt.Sprintf("cat > '%s' << '%s'\n%s\n%s", remoteFilePath, delimiter, escapedContent, delimiter)
		err = session.Run(cmd)
		if err != nil {
			result.error = C.CString(fmt.Sprintf("Failed to upload file: %v", err))
			return result
		}
	}

	result.success = 1
	return result
}

//export DownloadFile
func DownloadFile(config *C.SSHConfig_C, remotePath *C.char, localPath *C.char) *C.SSHResult_C {
	result := (*C.SSHResult_C)(C.malloc(C.size_t(unsafe.Sizeof(C.SSHResult_C{}))))
	result.success = 0
	result.content = nil
	result.error = nil

	host := C.GoString(config.host)
	user := C.GoString(config.user)
	password := C.GoString(config.password)
	keyPath := C.GoString(config.key_path)
	port := int(config.port)
	remoteFilePath := C.GoString(remotePath)
	localFilePath := C.GoString(localPath)

	if port == 0 {
		port = 22
	}

	// 构建 SSH 客户端配置
	authMethods := []ssh.AuthMethod{}

	if keyPath != "" {
		key, err := loadPrivateKey(keyPath)
		if err == nil {
			signer, err := ssh.NewSignerFromKey(key)
			if err == nil {
				authMethods = append(authMethods, ssh.PublicKeys(signer))
			}
		}
	}

	if password != "" {
		authMethods = append(authMethods, ssh.Password(password))
	}

	if len(authMethods) == 0 {
		result.error = C.CString("No authentication method provided")
		return result
	}

	sshConfig := &ssh.ClientConfig{
		User:            user,
		Auth:            authMethods,
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	// 连接到 SSH 服务器
	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, sshConfig)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to connect: %v", err))
		return result
	}
	defer client.Close()

	// 创建会话
	session, err := client.NewSession()
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to create session: %v", err))
		return result
	}
	defer session.Close()

	// 读取远程文件
	var stdout bytes.Buffer
	session.Stdout = &stdout
	err = session.Run(fmt.Sprintf("cat '%s'", remoteFilePath))
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to read remote file: %v", err))
		return result
	}

	fileContent := stdout.String()

	// 写入本地文件
	err = os.WriteFile(localFilePath, []byte(fileContent), 0644)
	if err != nil {
		result.error = C.CString(fmt.Sprintf("Failed to write local file: %v", err))
		return result
	}

	result.success = 1
	return result
}

func main() {
	// Go 库，不需要 main 函数
}
