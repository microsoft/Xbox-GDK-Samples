![](./media/image1.png)

# 网络安全 OpenSSL Linux 服务器示例

### **注意：这些示例仅作为引用参考。 我们现在或将来均不保证它们足以实现你的网络安全目标。 请务必独立评估你的使用。**

# 说明

此示例演示了使用 OpenSSL 的 DTLS 实现帮助保护 UDP 流量的最佳实践，旨在与网络安全 OpenSSL 示例协同工作。

# ![文本说明已自动生成](./media/image3.png)

# 生成示例

用于生成示例的开发环境为：

- Windows 10 上适用于 Linux 的 Windows 子系统 v2

- Ubuntu 20.04 LTS

- Visual Studio Code

除了基本安装外，还进一步配置了 Ubuntu：

`sudo apt install build-essential clang gcc gdb openssl-dev`

该示例包括 .vscode 目录中的 VS Code 项目配置，可能需要对其进行修改以适应本地系统配置。

如果采用上述配置，也可以使用以下内容生成该项目：

`clang++ -std=c++17 -g ./\*.cpp -o ./NetworkSecurityOpenSSL_Linux -l ssl -l crypto -pthread`

# 运行示例

为了运行此示例，还必须生成并运行网络安全 OpenSSL 示例。 服务器还需要 x509 证书和相关私钥。

通常，生产游戏会使用安全密钥服务来交换证书信息以进行安全验证，但对于此示例，需要在客户端和服务器之间复制/粘贴这些值。

启动客户端后，它将生成并显示标识字符串。 启动服务器需要此值；系统会在启动时提示输入此值。 服务器还将生成标识字符串。

在客户端中选择&ldquo;打开连接&rdquo;时，它将提示输入服务器地址和标识字符串。 输入这些值后，客户端将遵循连接到服务器时的安全最佳做法，且客户端可以向服务器发送任意消息，以便服务器回显它们。

# 实现说明

此示例中的 DTLS 实现与移植到 Linux 的网络安全 OpenSSL 示例中的实现相同。

与客户端版本的区别是：

- Winsock API 已替换为通用 BSD 套接字

- Windows 线程池和重叠 I/O 被替换为 std:: 线程和 select() 循环

该示例还包括基于 STL 构建的 XAsync 和 XTaskQueue 的实现，以实现可移植性。

最后，在生产代码中，应使用密钥服务或大厅服务（如 MPSD）来帮助在游戏实例之间安全地共享密钥材料。

# 已知问题

# 更新历史记录

**初始版本**：2021 年 8 月


