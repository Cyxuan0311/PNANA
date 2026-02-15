package main

import (
	"encoding/base64"
	"strings"
)

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

