package main

import (
	"fmt"
	"sync"
	"time"

	"golang.org/x/crypto/ssh"
)

// 连接池结构
type sshConnection struct {
	client   *ssh.Client
	lastUsed time.Time
	refCount int
	mutex    sync.Mutex
}

// 连接池管理器
type connectionPool struct {
	connections     map[string]*sshConnection
	mutex           sync.RWMutex
	cleanupTicker   *time.Ticker
	maxIdleTime     time.Duration
	cleanupInterval time.Duration
}

// 全局连接池实例
var (
	globalPool *connectionPool
	poolOnce   sync.Once
)

// 初始化全局连接池
func initConnectionPool() {
	poolOnce.Do(func() {
		globalPool = &connectionPool{
			connections:     make(map[string]*sshConnection),
			maxIdleTime:     10 * time.Minute,
			cleanupInterval: 5 * time.Minute,
		}
		globalPool.startCleanup()
	})
}

// 生成连接键
func connectionKey(host string, port int, user string) string {
	return fmt.Sprintf("%s:%d:%s", host, port, user)
}

// 启动清理goroutine
func (p *connectionPool) startCleanup() {
	p.cleanupTicker = time.NewTicker(p.cleanupInterval)
	go func() {
		for range p.cleanupTicker.C {
			p.cleanupIdleConnections()
		}
	}()
}

// 获取或创建SSH连接
func (p *connectionPool) getConnection(host string, port int, user string, password string, keyPath string) (*ssh.Client, error) {
	key := connectionKey(host, port, user)

	p.mutex.Lock()
	defer p.mutex.Unlock()

	// 检查连接池中是否已有连接
	if conn, exists := p.connections[key]; exists {
		conn.mutex.Lock()
		defer conn.mutex.Unlock()

		// 检查连接是否仍然有效
		if conn.client != nil {
			// 简单检查：尝试创建一个会话
			session, err := conn.client.NewSession()
			if err == nil {
				session.Close()
				conn.refCount++
				conn.lastUsed = time.Now()
				return conn.client, nil
			}
			// 连接已失效，从池中移除
			delete(p.connections, key)
		}
	}

	// 创建新连接
	authMethods, err := buildAuthMethods(keyPath, password)
	if err != nil {
		return nil, err
	}

	sshConfig := &ssh.ClientConfig{
		User:            user,
		Auth:            authMethods,
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
		Timeout:         10 * time.Second,
	}

	address := fmt.Sprintf("%s:%d", host, port)
	client, err := ssh.Dial("tcp", address, sshConfig)
	if err != nil {
		return nil, err
	}

	// 添加到连接池
	p.connections[key] = &sshConnection{
		client:   client,
		lastUsed: time.Now(),
		refCount: 1,
	}

	return client, nil
}

// 释放SSH连接（减少引用计数）
func (p *connectionPool) releaseConnection(host string, port int, user string) {
	key := connectionKey(host, port, user)

	p.mutex.Lock()
	defer p.mutex.Unlock()

	if conn, exists := p.connections[key]; exists {
		conn.mutex.Lock()
		defer conn.mutex.Unlock()

		conn.refCount--
		conn.lastUsed = time.Now()

		// 如果引用计数为0且空闲时间过长，关闭连接
		if conn.refCount <= 0 {
			if time.Since(conn.lastUsed) > p.maxIdleTime {
				if conn.client != nil {
					conn.client.Close()
				}
				delete(p.connections, key)
			}
		}
	}
}

// 清理空闲连接
func (p *connectionPool) cleanupIdleConnections() {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	now := time.Now()
	for key, conn := range p.connections {
		conn.mutex.Lock()
		if conn.refCount <= 0 && now.Sub(conn.lastUsed) > p.maxIdleTime {
			if conn.client != nil {
				conn.client.Close()
			}
			delete(p.connections, key)
		}
		conn.mutex.Unlock()
	}
}

// 获取或创建SSH连接（全局函数，用于向后兼容）
func getSSHConnection(host string, port int, user string, password string, keyPath string) (*ssh.Client, error) {
	initConnectionPool()
	return globalPool.getConnection(host, port, user, password, keyPath)
}

// 释放SSH连接（全局函数，用于向后兼容）
func releaseSSHConnection(host string, port int, user string) {
	initConnectionPool()
	globalPool.releaseConnection(host, port, user)
}
