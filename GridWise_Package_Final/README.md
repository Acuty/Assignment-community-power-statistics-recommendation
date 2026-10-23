# GridWise 社区用电分析平台

## 一键运行

双击 `GridWise.lnk`，程序会启动本地 HTTP 服务，并自动打开默认浏览器访问：

```text
http://127.0.0.1:8000
```

也可以直接双击 `GridWise.exe`。

程序运行期间会保持后端服务运行。关闭程序时，在控制台窗口按 `Ctrl+C`。

## 默认账号

```text
管理员：admin / admin123
普通用户：user / user123
```

## 发布包结构

```text
GridWise_Package_Final/
├── GridWise.exe              # x64 Release 可执行文件
├── GridWise.lnk              # 双击后启动并打开网页的快捷方式
├── electricity_demo.csv      # 运行数据，增删改后写回此文件
├── web/                      # 前端网页文件
│   ├── index.html
│   ├── app.js
│   └── style.css
└── src/                      # 主要 C 源码和头文件
```

本发布包没有复制 Visual Studio 工程文件、`.vs` 目录、`x64` 目录、`.sln` 文件或 `.vcxproj` 文件。

## 运行要求

- Windows 64 位系统；
- 8000 端口没有被其他程序占用；
- `GridWise.exe`、`electricity_demo.csv` 和 `web` 文件夹保持在同一目录；
- 如果浏览器没有自动打开，可以手动访问 `http://127.0.0.1:8000`。

## 数据说明

程序启动时读取 `electricity_demo.csv`。管理员新增、修改或删除记录后，系统会把数据保存回该文件，请不要在程序运行时移动或删除它。

## 重新编译

使用 Visual Studio Developer Command Prompt，在 `src` 目录执行：

```text
cl /nologo /O2 /MD /utf-8 /DWIN32_LEAN_AND_MEAN *.c /Fe:"..\GridWise.exe" /link ws2_32.lib shell32.lib
```

编译目标为 x64 Release。
