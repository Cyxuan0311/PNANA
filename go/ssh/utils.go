package main

import (
	"encoding/base64"
	"strings"
)

// quoteForSh 单引号包裹并转义，供 sh -c 使用
func quoteForSh(s string) string {
	return "'" + strings.ReplaceAll(s, "'", "'\\''") + "'"
}

// escapeShellPath 对路径进行 shell 转义（单引号包裹）
func escapeShellPath(s string) string {
	return quoteForSh(s)
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

