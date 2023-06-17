## 一、功能
  - 支持 HTTP/1.1、HTTP/2。
  - 支持 SSL/TLS 协议。
  - 可自定义请求，当前已测试的请求类型: GET、POST。

## 二、编译环境

该项目使用的构建工具为 `xmake`。下面示例所使用的环境为 Ubuntu 22.04。

### 1、安装 xmake

[xmake 官方安装文档](https://xmake.io/#/zh-cn/guide/installation)。

```shell
curl -fsSL https://xmake.io/shget.text | bash
source ~/.xmake/profile
```

### 2、编译和执行

```shell
# 编译
cd <你的路径>/hfc
xmake

# 执行 example
xmake run get_example <参数>
xmake run post_example <参数>
xmake run httpc2_example <参数>
```

### 3、文档生成

使用 `doxygen` 来生成文档。

```shell
# 安装 doxygen
sudo apt install doxygen graphviz

# 生成文档
cd src
doxygen
```

## 三、如何移植

该项目使用 `POSIX` 标准的接口和 `C` 标准库接口。下面的主要移植步骤： 

1. 将 `hfc` 目录下的文件包含到项目中。
2. 修改 `src/httpc_config.h` 文件。

## 四、错误排查

使用的测试环境为 `Ubuntu`。

### 1.getaddrinfo fail: Name or service not known

```shell
# 添加 DNS 服务器
sudo vim /etc/systemd/resolved.conf
# 在 [Resolve] 后面添加下面的内容
DNS=8.8.8.8 114.114.114.114
# 添加完成后执行下面操作
systemctl restart systemd-resolved.service
systemctl enable systemd-resolved
sudo mv /etc/resolv.conf /etc/resolv.conf.bak
sudo ln -s /run/systemd/resolve/resolv.conf /etc/
# 检测是否生效
cat /etc/resolv.conf
```