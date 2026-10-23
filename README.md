# 大作业：基于小区用电情况电力统计推荐系统

> **项目简介**：
> 本项目是一个基于 C 语言与 Mongoose 开发的本地社区用电数据管理与分析平台。系统能将杂乱的用电数据转化为直观的图表，并利用多维度的数据算法，自动生成“优先扩容建议名单”，辅助管理人员进行电网扩容决策。
> 核心功能包括：**用电记录管理**（增删改查与自动计费）、**多维数据查询**、**可视化统计报表**（折线/柱状/饼图）、以及**智能扩容推荐**（基于四项指标自动加权评分排行）。

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

CSV 表头：

```text
community,household,year,month,peak_kwh,valley_kwh,bill
```

- `bill` 由后端按峰谷电价计算（默认峰时 0.6 元/kWh、谷时 0.3 元/kWh）；
- 同一 `(小区, 住户, 年份, 月份)` 记录唯一，重复会被拒绝；
- 小区名、住户名精确匹配，输入须与 CSV 完全一致。

## 可改的地方

### 1. 账号与密码

改 `auth.c` 顶部的 `demo_users` 数组：

```c
static const User demo_users[] = {
    { "你的学号", "你的密码", ROLE_ADMIN },
    { "user",    "user123",  ROLE_USER  }
};
```

### 2. 电价

改 `main.c` 的 `main()`：

```c
data.rate.peak_price = 0.6;    // 峰时电价
data.rate.valley_price = 0.3;  // 谷时电价
```

### 3. 推荐权重

改 `main.c` 的 `main()`：

```c
static const double recommendation_weights[EXPANSION_INDICATOR_COUNT] = {
    0.25, 0.25, 0.25, 0.25
};
```

四项依次为：峰电年增长率、峰值月同比、峰谷比、户均月峰电。程序内部按总和归一化，不必凑成 1。

### 4. 端口

改 `main.c` 的 `main()`：

```c
web_server_start(&data, "http://127.0.0.1:8000", ...);
```

同时改 `main.c` 里 `open_frontend()` 中的 `http://127.0.0.1:8000`。

### 5. 每页条数

- 后端：`web_server.c` 的 `handle_get_records` 中 `query_page(..., 50, results, 50)`；
- 前端：`web/app.js` 顶部 `const PAGE_SIZE = 50;`。

两处同时改。

### 6. 小区名称、住户名称、时间

**方式 A：直接改 CSV**（最简单）

打开 `electricity_demo.csv`：
- 第一列 `community`：改成你的小区名（如 `幸福小区`）；
- 第二列 `household`：住户编号（如 `A01`、`101`）；
- 第三列 `year`、第四列 `month`：时间，`month` 必须 1–12；
- `bill` 保持 `peak_kwh × 0.6 + valley_kwh × 0.3`，与后端一致。

**方式 B：改生成器规则**（`data_generator.c` 的 `generate_csv()`）

```c
// 住户编号格式，默认 H001、H002…
fprintf(file, "%s,H%03zu,...", ...);
//          ↑ 改 "H%03zu" 可换格式，如 "A%02zu"（A01、A02…）

// 月总电量范围，默认 100–400
total_kwh = 100 + rand() % 301;
//           ↑ 改 100 和 301 可改范围

// 峰时占比范围，默认 40%–70%
peak_percent = 40 + rand() % 31;
//              ↑ 改 40 和 31 可改范围
```

**方式 C：小区名列表**（调用 `generate_csv` 的地方）

```c
const char communities[][MODEL_NAME_LENGTH] = {
    "小区01", "小区02", "小区03"   // 改成你的小区名
};
```

**时间筛选规则**：
- `year` 留空 = 全部年份；非空须为 1–9999 整数；
- `month` 留空 = 全部月份；非空须为 1–12；
- 图表 X 轴显示 `YYYY-MM`，推荐页「统计周期」显示 `起始年—结束年`。

### 7. 页面标题与菜单文字

- `web/index.html`：`<title>`、侧边栏 `<button class="nav">` 文字、各 `<h1>`；
- `web/app.js` 顶部 `pageTitles` 对象。

只改显示文字，`data-page` / `data-view` 的值不要改。

### 8. 颜色主题

改 `web/style.css` 顶部 `:root` 变量：

```css
:root {
  --ink: #1d2b3b;        /* 主文字色 */
  --navy: #21384d;       /* 主色（侧边栏、按钮、峰时线） */
  --green: #527b70;      /* 强调色 */
  --amber: #b28a4b;      /* 次要强调（谷时线） */
  --paper: #f4f5f2;      /* 页面背景 */
  --surface: #ffffff;    /* 卡片背景 */
}
```

### 9. 数据生成器每小区户数下限

改 `data_generator.c` 的 `generate_csv()` 参数检查：

```c
household_count < 50 ||    // ← 把 50 改成你想要的数，比如 1
```

### 10. 新增 API

在 `web_server.c` 的 `http_event_handler()` 加分支：

```c
else if (mg_match(request->uri, mg_str("/api/stats"), NULL)) {
    if (mg_match(request->method, mg_str("GET"), NULL)) {
        handle_stats(connection, request);
    } else {
        reply_error(connection, 405, "不支持的请求方法");
    }
}
```

再写处理函数：

```c
static void handle_stats(struct mg_connection* c, struct mg_http_message* req)
{
    if (require_login(c, req) == 0) return;
    mg_http_reply(c, 200,
        "Content-Type: application/json\r\n",
        "{\"total\":%zu}\n", server_data->record_count);
}
```

### 11. 新增前端页面

1）`index.html` 加菜单按钮：

```html
<button class="nav auth-only" data-page="about"><i>i</i>关于</button>
```

2）`index.html` 加页面内容：

```html
<section class="page" data-view="about">
  <div class="page-intro"><h1>关于</h1><p>你的说明文字。</p></div>
</section>
```

3）`app.js` 的 `pageTitles` 注册：

```js
const pageTitles = {
  // ...
  about: '关于'
};
```

`data-page` 与 `data-view` 同名即可自动生效。

## 重新编译

使用 Visual Studio Developer Command Prompt，在 `src` 目录执行：

```text
cl /nologo /O2 /MD /utf-8 /DWIN32_LEAN_AND_MEAN *.c /Fe:"..\GridWise.exe" /link ws2_32.lib shell32.lib
```

编译目标为 x64 Release。

> 当前代码依赖 MSVC 专有的 `fopen_s` / `sscanf_s` / `strcpy_s`，GCC/Clang 需自行替换。

## 已知限制

- Basic Auth 密码 base64 明文传输，只能本地/HTTPS 使用；
- 账号硬编码，演示用途；
- 每页固定 50 条，前端拉全量后聚合，适合中小数据；
- `sorting.c` 冒泡排序 O(n²)，未被 Web 层调用；
- 仅 MSVC + Windows 构建；
- `generate_csv` 要求每小区 ≥ 50 户，覆盖写文件，运行前请备份。
