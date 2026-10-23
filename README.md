# 小区用电统计与扩容推荐系统 GridWise

GridWise 是一个用 C 语言实现的课程项目，使用 CSV 文件保存用电记录，通过本地 HTTP 服务和网页界面提供查询、统计、排序与小区扩容推荐。项目内附有模拟数据，拿到代码后可以先照下面的示例登录和查询，再替换成自己的数据。

> **项目用途：课程演示。** 账号写在源代码中，HTTP Basic 认证在普通 HTTP 连接上不加密。请只在本机或受控测试环境运行，不要直接暴露到公网，也不要把真实居民信息提交到 GitHub。

## 先运行并查询一条现成数据

### 启动

在 Windows x64 上进入 `GridWise_Package_Final` 文件夹，双击 `GridWise.exe`。程序会启动本地服务并尝试打开浏览器；如果没有自动打开，在浏览器访问：

```text
http://127.0.0.1:8000
```

保持 `GridWise.exe`、`electricity_demo.csv` 和 `web` 文件夹之间的相对位置不变。服务运行时会打开控制台窗口，按 `Ctrl+C` 停止。

### 登录

登录页使用以下演示账号。它们是公开的固定测试账号，不要把它们当作安全凭据：

| 角色 | 账号 | 密码 | 可做的事 |
| --- | --- | --- | --- |
| 管理员 | `admin` | `admin123` | 查询、统计、排序、推荐，并新增/修改/删除用电记录 |
| 普通用户 | `user` | `user123` | 查询、统计、排序和查看推荐 |

### 查询示例

仓库里的演示数据共 **12,000 条**：小区 `小区01` 到 `小区10`；每个小区有住户 `H001` 到 `H050`；时间覆盖 **2024 年 1 月至 2025 年 12 月**，每户每月一条记录。

登录后选择左侧 **用电查询**，输入下面这些值，再点“查询记录”：

| 页面输入项 | 示例值 |
| --- | --- |
| 小区名称 | `小区01` |
| 住户 | `H001` |
| 年份 | `2024` |
| 月份 | `1` |

这条数据在 CSV 中对应：

```csv
小区01,H001,2024,1,87.42,53.58,68.53
```

小区名和住户名需要与 CSV 完全一致。若查不到，先打开 `GridWise_Package_Final/electricity_demo.csv`，查看前两列和年月；空着住户、年份或月份可以扩大筛选范围。统计报表和排序需要填写小区名称，年份可留空；扩容推荐按当前 CSV 中的所有小区计算。

## 用自己的数据替换演示数据

1. 先关闭程序，并备份 `GridWise_Package_Final/electricity_demo.csv`。
2. 用支持 UTF-8 的文本编辑器或表格软件打开 CSV。保留第一行表头和列顺序：

   ```csv
   community,household,year,month,peak_kwh,valley_kwh,bill
   ```

3. 用自己的模拟或脱敏数据替换后续记录。每行表示一户在一个月的用电情况，例如：

   ```csv
   幸福小区,楼栋1-101,2024,1,120.50,80.25,96.38
   ```

4. 每户每月只放一条记录；同一小区、住户、年份、月份不能重复。月份必须为 1–12，年份不能为 0，峰时和谷时电量及电费不能为负数。小区名和住户名不要包含逗号或换行，以免破坏 CSV 列结构。
5. `bill` 列也必须填写。当前 `main.c` 默认峰时电价为 0.6 元/kWh、谷时电价为 0.3 元/kWh，电费按 `峰时电量 × 0.6 + 谷时电量 × 0.3` 计算并保留两位小数。网页中管理员录入记录时由后端计算电费；手动改 CSV 时需要自己写对这一列。
6. 保存文件后重新启动程序。若数据格式错误、记录重复或记录超过容量，程序可能无法加载；此时恢复备份并检查表头、字段数量和每行数据。

系统当前最多保存 **50,000 条**记录，定义在 `GridWise_Package_Final/src/model.h` 的 `MAX_RECORDS`。替换数据后，先从 CSV 中选一个确实存在的小区、住户和年月，按上面的查询流程确认可查。

> 公开仓库只保留模拟数据。若使用真实业务数据，请只在本地运行并做好脱敏，绝不要提交真实姓名、地址、电话、账号或账单。

## 从源码编译

需要 Windows x64 和 MSVC。打开 **x64 Native Tools Command Prompt for Visual Studio**，切换到仓库中的 `GridWise_Package_Final\src`，执行：

```bat
cl /nologo /O2 /MD /utf-8 /DWIN32_LEAN_AND_MEAN *.c /Fe:"..\GridWise.exe" /link ws2_32.lib shell32.lib
```

编译完成后，在 `GridWise_Package_Final` 目录运行 `GridWise.exe`。不要把可执行文件单独移出目录，因为程序需要旁边的网页文件和 CSV 文件。

## 项目文件说明

```text
GridWise_Package_Final/
├── src/
│   ├── model.h              公共数据结构与记录容量
│   ├── main.c               电价、推荐权重和系统启动流程
│   ├── auth.c               演示账号和角色权限
│   ├── data_generator.c     模拟 CSV 生成函数
│   ├── file_manager.c       CSV 读取和保存
│   ├── data_manager.c       记录校验及增删改
│   ├── billing.c            峰谷电费计算
│   ├── statistics.c         家庭与小区统计
│   ├── query.c              记录筛选查询
│   ├── sorting.c            统计结果排序
│   ├── recommendation.c     扩容优先级评分和排序
│   ├── web_server.c         HTTP API 与请求处理
│   └── mongoose.c/.h        Mongoose 网络库
├── web/                     网页、脚本和样式
├── electricity_demo.csv     可直接查询的模拟用电数据
└── GridWise.exe             Windows x64 演示程序
```

## 常见修改位置

| 想调整的内容 | 修改位置 | 修改后是否需要重新编译 |
| --- | --- | --- |
| 小区、住户、年份、月份和用电量 | 替换 `electricity_demo.csv` | 否，重启程序即可 |
| 管理员/普通用户的演示账号 | `src/auth.c` 中的 `demo_users` | 是 |
| 峰谷电价 | `src/main.c` 中 `data.rate` 的赋值 | 是；手动准备的 CSV 电费也要按新电价重算 |
| 推荐指标权重 | `src/main.c` 中 `recommendation_weights` | 是 |
| 最大记录数 | `src/model.h` 中 `MAX_RECORDS` | 是；数组内存也会随之增大 |
| 网页文案和布局 | `web/index.html`、`web/app.js`、`web/style.css` | 通常否，刷新网页即可 |
| HTTP 端口 | `src/main.c` 中服务地址及自动打开浏览器的地址 | 是；两处地址要保持一致 |

## 已知限制

- 当前只提供 Windows/MSVC 编译方式，代码使用 MSVC 安全 CRT 函数。
- 账号是演示用途的硬编码账号，密码没有安全存储。
- HTTP Basic 认证不会加密普通 HTTP 流量；默认只监听 `127.0.0.1:8000`。
- `electricity_demo.csv` 会被管理员操作写回，演示前建议备份。
- 仓库目前没有自动化构建和测试流程。

## 许可证与贡献

项目采用 GNU GPL-2.0-only，完整文本见 [LICENSE](LICENSE)。Mongoose 的版本和许可说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。贡献方式见 [CONTRIBUTING.md](CONTRIBUTING.md)，安全说明见 [SECURITY.md](SECURITY.md)。
