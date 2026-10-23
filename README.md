# 小区用电统计与扩容推荐系统 GridWise

GridWise 是一个使用 C 语言、CSV 文件和 Mongoose HTTP 服务实现的课程项目，用于管理模拟用电记录、生成统计报表，并按配置权重对小区扩容优先级进行排序。

> **项目状态：课程演示项目。** 当前账号为硬编码演示账号，HTTP 服务使用 Basic 认证且默认通过本机 HTTP 访问。请勿用于生产环境、真实居民数据或直接暴露到公网。

## 功能

- CSV 用电记录读取、查询及管理员增删改
- 峰谷电费计算、住户和小区统计
- 查询、排序及扩容优先级推荐
- 浏览器界面和 JSON HTTP API
- 随附模拟数据和 Windows 演示程序

## 快速开始

### 直接运行 Windows 演示程序

进入 `GridWise_Package_Final` 文件夹，运行 `GridWise.exe`。程序启动后访问：

```text
http://127.0.0.1:8000
```

演示账号写在 `GridWise_Package_Final/src/auth.c` 中，并且在公开源代码里可见：

| 角色 | 账号 | 密码 |
| --- | --- | --- |
| 管理员 | `admin` | `admin123` |
| 普通用户 | `user` | `user123` |

这些账号仅供本地演示。程序会把管理员修改写回 `electricity_demo.csv`，运行前可先备份该文件。

### 从源码编译

需要 Windows x64 和 MSVC（Visual Studio Developer Command Prompt）。在 `GridWise_Package_Final/src` 目录运行：

```bat
cl /nologo /O2 /MD /utf-8 /DWIN32_LEAN_AND_MEAN *.c /Fe:"..\GridWise.exe" /link ws2_32.lib shell32.lib
```

编译完成后从 `GridWise_Package_Final` 目录启动程序，确保 `web` 文件夹和 `electricity_demo.csv` 与可执行文件保持在一起。

## 项目结构

```text
.
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── SECURITY.md
├── THIRD_PARTY_NOTICES.md
└── GridWise_Package_Final/
    ├── src/                 C 源码、头文件及 Mongoose
    ├── web/                 网页界面
    ├── electricity_demo.csv 模拟演示数据
    └── GridWise.exe         Windows x64 演示程序
```

## 数据说明

仓库附带的 `electricity_demo.csv` 使用模拟住户编号和模拟用电量。CSV 列为 `community,household,year,month,peak_kwh,valley_kwh,bill`。请勿向公开仓库提交真实姓名、地址、账号、账单或其他个人信息。

## 已知限制

- 仅提供 Windows/MSVC 构建方式，源码使用 MSVC 安全 CRT 函数。
- 演示账号硬编码，密码未做安全存储。
- Basic 认证只编码凭据；在普通 HTTP 连接上不提供传输加密。
- 默认使用本机地址和固定端口 8000；若需局域网或公网部署，必须先补充安全配置。
- 当前仓库未配置自动化构建和测试流程。

## 许可证

本项目按 GNU GPL-2.0-only 发布，完整文本见 [LICENSE](LICENSE)。Mongoose 的版权及许可声明见 [第三方组件说明](THIRD_PARTY_NOTICES.md) 和 `GridWise_Package_Final/src/mongoose.c`、`GridWise_Package_Final/src/mongoose.h` 文件头。
