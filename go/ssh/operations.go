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
	"io"
	"strings"
	"unsafe"

	"golang.org/x/crypto/ssh"
	"github.com/pkg/sftp"
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

// 读取远程文件（使用 SFTP）
func readRemoteFile(client *ssh.Client, remotePath string) (string, error) {
	// 创建 SFTP 客户端
	sftpClient, err := sftp.NewClient(client)
	if err != nil {
		return "", fmt.Errorf("failed to create SFTP client: %v", err)
	}
	defer sftpClient.Close()
	
	// 打开远程文件
	remoteFile, err := sftpClient.Open(remotePath)
	if err != nil {
		return "", fmt.Errorf("failed to open remote file: %v", err)
	}
	defer remoteFile.Close()
	
	// 获取文件大小
	fileInfo, err := remoteFile.Stat()
	if err != nil {
		return "", fmt.Errorf("failed to get file stat: %v", err)
	}
	
	fileSize := fileInfo.Size()
	
	// 读取文件内容
	data := make([]byte, fileSize)
	n, err := io.ReadFull(remoteFile, data)
	if err != nil && err != io.EOF {
		return "", fmt.Errorf("failed to read file: %v", err)
	}
	
	return string(data[:n]), nil
}

// 写入远程文件（使用 SFTP）
func writeRemoteFile(client *ssh.Client, remotePath string, content string) error {
	// 创建 SFTP 客户端
	sftpClient, err := sftp.NewClient(client)
	if err != nil {
		return fmt.Errorf("failed to create SFTP client: %v", err)
	}
	defer sftpClient.Close()
	
	// 创建远程文件
	remoteFile, err := sftpClient.Create(remotePath)
	if err != nil {
		return fmt.Errorf("failed to create remote file: %v", err)
	}
	defer remoteFile.Close()
	
	// 写入数据
	_, err = remoteFile.Write([]byte(content))
	if err != nil {
		return fmt.Errorf("failed to write data: %v", err)
	}
	
	return nil
}

// 上传文件到远程（使用 SFTP）
func uploadFileToRemote(client *ssh.Client, remotePath string, localData []byte) error {
	// 创建 SFTP 客户端
	sftpClient, err := sftp.NewClient(client)
	if err != nil {
		return fmt.Errorf("failed to create SFTP client: %v", err)
	}
	defer sftpClient.Close()
	
	// 创建远程文件
	remoteFile, err := sftpClient.Create(remotePath)
	if err != nil {
		return fmt.Errorf("failed to create remote file: %v", err)
	}
	defer remoteFile.Close()
	
	// 写入数据
	_, err = remoteFile.Write(localData)
	if err != nil {
		return fmt.Errorf("failed to write data: %v", err)
	}
	
	return nil
}

// 从远程下载文件（使用 SFTP）
func downloadFileFromRemote(client *ssh.Client, remotePath string) ([]byte, error) {
	// 创建 SFTP 客户端
	sftpClient, err := sftp.NewClient(client)
	if err != nil {
		return nil, fmt.Errorf("failed to create SFTP client: %v", err)
	}
	defer sftpClient.Close()
	
	// 打开远程文件
	remoteFile, err := sftpClient.Open(remotePath)
	if err != nil {
		return nil, fmt.Errorf("failed to open remote file: %v", err)
	}
	defer remoteFile.Close()
	
	// 获取文件大小
	fileInfo, err := remoteFile.Stat()
	if err != nil {
		return nil, fmt.Errorf("failed to get file stat: %v", err)
	}
	
	fileSize := fileInfo.Size()
	
	// 读取文件内容
	data := make([]byte, fileSize)
	n, err := io.ReadFull(remoteFile, data)
	if err != nil && err != io.EOF {
		return nil, fmt.Errorf("failed to read file: %v", err)
	}
	
	return data[:n], nil
}

// 列出远程目录，返回格式每行: "d\tname" 或 "f\tname"（d=目录 f=文件），不含 . 和 ..
func listRemoteDir(client *ssh.Client, remotePath string) (string, error) {
	session, err := client.NewSession()
	if err != nil {
		return "", fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	var stdout bytes.Buffer
	session.Stdout = &stdout
	// 进入目录后遍历，输出 d\tname 或 f\tname（d=目录 f=文件）
	script := fmt.Sprintf("cd %s 2>/dev/null && for x in * .[!.]* ..?*; do [ -e \"$x\" ] || continue; [ \"$x\" = . ] && continue; [ \"$x\" = .. ] && continue; [ -d \"$x\" ] && echo \"d\t$x\" || echo \"f\t$x\"; done", escapeShellPath(remotePath))
	err = session.Run("sh -c " + quoteForSh(script))
	if err != nil {
		return "", fmt.Errorf("failed to list directory: %v", err)
	}
	return stdout.String(), nil
}

// 获取路径类型，返回 "dir"、"file" 或 "error"
func getRemotePathType(client *ssh.Client, remotePath string) (string, error) {
	session, err := client.NewSession()
	if err != nil {
		return "", fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	var stdout bytes.Buffer
	session.Stdout = &stdout
	// test -d 为目录则输出 dir，否则 test -f 为文件则输出 file，否则 error
	cmd := fmt.Sprintf("if test -d %s; then echo dir; elif test -f %s; then echo file; else echo error; fi", escapeShellPath(remotePath), escapeShellPath(remotePath))
	err = session.Run("sh -c " + quoteForSh(cmd))
	if err != nil {
		return "error", nil
	}
	return strings.TrimSpace(stdout.String()), nil
}

// runRemoteCommand 在远程 cwd 下执行命令，返回 stdout、stderr 和退出码
func runRemoteCommand(client *ssh.Client, cwd string, command string) (stdout string, stderr string, exitCode int, err error) {
	session, err := client.NewSession()
	if err != nil {
		return "", "", -1, fmt.Errorf("failed to create session: %v", err)
	}
	defer session.Close()

	var outBuf, errBuf bytes.Buffer
	session.Stdout = &outBuf
	session.Stderr = &errBuf

	fullCmd := fmt.Sprintf("cd %s 2>/dev/null && %s", quoteForSh(cwd), command)
	err = session.Run("sh -c " + quoteForSh(fullCmd))
	stdout = outBuf.String()
	stderr = errBuf.String()
	if err != nil {
		if exitErr, ok := err.(*ssh.ExitError); ok {
			exitCode = exitErr.ExitStatus()
			return stdout, stderr, exitCode, nil
		}
		return stdout, stderr, -1, err
	}
	return stdout, stderr, 0, nil
}
