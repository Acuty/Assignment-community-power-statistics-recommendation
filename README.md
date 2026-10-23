# 小区用电统计与扩容推荐系统 GridWise

GridWise 是一个基于 C 语言、CSV 和 Mongoose HTTP 服务的课程演示项目。它管理家庭月度用电记录，生成小区统计报表，并根据四项用电指标和配置权重计算扩容优先级。

> **运行提醒：**演示账号写在源码中；HTTP Basic 认证在普通 HTTP 连接上不加密。请只在本机或受控测试环境运行，不要直接暴露到公网。仓库中的数据是模拟数据，不要把真实居民信息提交到 GitHub。

## 1. 直接运行和第一次查询

### 启动程序

在 Windows x64 上打开 `GridWise_Package_Final` 文件夹，双击 `GridWise.exe`。程序会启动本地 HTTP 服务并尝试打开默认浏览器。若浏览器没有自动打开，访问：

```text
http://127.0.0.1:8000
```

服务运行期间会保留控制台窗口；在控制台按 `Ctrl+C` 停止。请让 `GridWise.exe`、`electricity_demo.csv` 和 `web` 文件夹保持在同一目录结构中。仓库不再提供快捷方式文件，因为快捷方式会绑定创建者电脑上的路径。

### 演示账号

| 角色 | 账号 | 密码 | 权限 |
| --- | --- | --- | --- |
| 管理员 | `admin` | `admin123` | 查询、报表、排序、推荐，以及新增/修改/删除记录 |
| 普通用户 | `user` | `user123` | 查询、报表、排序和查看推荐 |

这是公开源代码中的演示凭据，只适合本机体验。

### 可直接使用的查询值

仓库附带的 `GridWise_Package_Final/electricity_demo.csv` 有 **12,000 条模拟记录**：10 个小区（`小区01` 至 `小区10`），每个小区 50 户（`H001` 至 `H050`），覆盖 2024 年 1 月至 2025 年 12 月，每户每月一条记录。

登录后点击左侧 **用电查询**，输入以下值并点击“查询记录”：

| 查询字段 | 输入值 |
| --- | --- |
| 小区名称 | `小区01` |
| 住户 | `H001` |
| 年份 | `2024` |
| 月份 | `1` |

对应 CSV 记录如下，查询结果应能看到同样的峰谷电量和电费：

```csv
小区01,H001,2024,1,87.42,53.58,68.53
```

名称按文本精确匹配。若无结果，请先在 CSV 中查找 `community`、`household`、`year` 和 `month` 的实际值。查询页面必须填写小区名称；住户、年份、月份可以留空以扩大查询范围。报表和排序也需要填写小区名称，年份可留空。扩容推荐使用当前数据中的小区，不需要输入某个住户。

## 2. CSV 数据格式与替换方法

每条记录表示某个住户某个月的用电情况。CSV 表头必须保留，字段顺序不能改变：

```csv
community,household,year,month,peak_kwh,valley_kwh,bill
```

示例：

```csv
community,household,year,month,peak_kwh,valley_kwh,bill
幸福小区,1号楼101,2024,1,120.50,80.25,96.38
幸福小区,1号楼101,2024,2,115.00,75.00,91.50
```

字段说明：

| 字段 | 含义与要求 |
| --- | --- |
| `community` | 小区名称；同一小区每条记录必须使用相同拼写 |
| `household` | 住户名称或编号；与小区、年份、月份共同确定一条记录 |
| `year` | 年份整数，不能为 0 |
| `month` | 月份整数，范围 1–12 |
| `peak_kwh` | 峰时用电量，不能为负数 |
| `valley_kwh` | 谷时用电量，不能为负数 |
| `bill` | 电费，不能为负数；手动编辑 CSV 时按当前电价填写 |

### 用自己的数据替换

1. 关闭程序，备份 `GridWise_Package_Final/electricity_demo.csv`。
2. 用支持 UTF-8 的文本编辑器或表格软件编辑 CSV，保留表头和七列顺序。
3. 每个住户每个月写一行；同一“小区 + 住户 + 年 + 月”不能有重复记录。名称不要含逗号或换行，以免破坏 CSV 分列。
4. 默认峰时电价为 0.6 元/kWh、谷时电价为 0.3 元/kWh，电费公式为：`峰时电量 × 0.6 + 谷时电量 × 0.3`，结果保留两位小数。例：`120.50 × 0.6 + 80.25 × 0.3 = 96.38`。
5. 保存后重新启动程序，再使用 CSV 中确实存在的小区、住户和年月查询。

管理员在网页中新增或修改记录时，后端会按当前电价计算电费。手动编辑 CSV 时，必须自己正确填写 `bill`。程序会把管理员的新增、修改和删除写回正在使用的 CSV；如果改坏或覆盖演示数据，可用备份恢复。

> 仓库的模拟数据用于体验功能。若换成真实数据，请仅在本地使用并先脱敏；不要把真实姓名、地址、电话、账号或账单提交到仓库。

## 3. 扩容推荐需要什么数据

当前推荐算法会自动选取数据中的最近两个年份。每个小区需要这两年各 12 个月都齐全，缺少任一月份的小区不会参加推荐。例如演示数据使用 2024 和 2025 两个完整年份。

当前四项指标依次为：

1. 峰电年增长率；
2. 第二年峰电量最大月份相对第一年同月的增长率；
3. 两年峰时总电量与谷时总电量之比；
4. 户均月峰电量。

系统先对指标做正向 min-max 归一化，再按配置权重加权并排序。结果是课程项目的参考排序，不应直接作为真实电网扩容决策依据。

## 4. 常见改造位置

下面列出复用项目时最常需要调整的内容。路径均相对于 `GridWise_Package_Final` 文件夹。

| 要调整的内容 | 修改位置 | 修改后操作与关联项 |
| --- | --- | --- |
| 小区、住户、年月、峰谷电量 | `electricity_demo.csv` | 保留七列和表头；重启程序。手工改数据时同步重算 `bill`。 |
| 模拟数据的小区名、户数、起始年份 | 调用 `generate_csv()` 的代码 | 传入小区数组、数量、户数和起始年；每小区至少 50 户。 |
| 模拟用电量范围、峰时占比 | `src/data_generator.c` | 修改 `100 + rand() % 301` 和 `40 + rand() % 31`；需重新编译并重新生成 CSV。 |
| 管理员/普通用户账号和密码 | `src/auth.c` 的 `demo_users` | 重新编译；这是硬编码演示账号。 |
| 峰时和谷时电价 | `src/main.c` 中 `data.rate` 赋值 | 重新编译；同步更新现有 CSV 的 `bill`，以及生成器使用的 `Price`。 |
| 电费计价公式 | `src/billing.c` 的 `billing_calculate()` | 当前公式为峰电量×峰价 + 谷电量×谷价；改公式后，还需检查 `src/data_generator.c` 和后端新增/修改记录的计费逻辑，并重算 CSV。 |
| 四项推荐指标权重 | `src/main.c` 的 `recommendation_weights` | 顺序必须与下方指标表一致；重新编译。权重非负且总和大于 0，程序会归一化。 |
| 推荐指标或计算方法 | `src/recommendation.c` | 修改指标后，需同步检查 `model.h` 的指标数量、`main.c` 权重数组及下方指标说明；重新编译。 |
| 最大记录数 | `src/model.h` 的 `MAX_RECORDS` | 重新编译；`SystemData` 是固定长度数组，增大上限会增加内存占用。网页另有 40,000 条读取上限，见下文。 |
| 网页最多读取的记录数 | `web/app.js` 的 `MAX_FRONTEND_RECORDS` | 与后端 `MAX_RECORDS` 分别设置；报表/排序从前端已读取记录计算，网页上限小于 CSV 记录数时结果可能不完整。 |
| 每页显示条数 | `src/web_server.c` 的 `query_page()` 调用和 `web/app.js` 的 `PAGE_SIZE` | 两处数值保持一致；改后端需重新编译，改网页需刷新。 |
| 服务地址和端口 | `src/main.c` 的 `web_server_start()` 与 `open_frontend()` | 两处地址一起改并重新编译；端口还需未被其他程序占用。 |
| 程序使用的 CSV 文件名 | `src/main.c` 的 `file_load_records()` 和 `web_server_start()` | 两处文件名要一致；运行包中的 CSV 文件也要改成对应名称。 |
| 网页文字、菜单和页面内容 | `web/index.html`、`web/app.js` | 只改显示内容时刷新网页；新增页面时同步菜单、页面区域和 `pageTitles`。 |
| 网页配色、布局 | `web/style.css` | 修改 `:root` 颜色变量或样式，刷新网页查看。 |
| HTTP 路由和请求/响应字段 | `src/web_server.c`、`src/web_server.h`、`web/app.js` | 前后端路径、参数名和 JSON 字段必须一致；C 代码需重新编译。 |
| CSV 字段、数据结构 | `src/model.h`、`src/file_manager.c`、`src/web_server.c`、`web/app.js` | 这是跨模块改动：同步更新结构体、CSV 读写、接口 JSON 和网页字段，再重新编译并检查旧 CSV 兼容性。 |

### 修改账号、价格和权重

账号定义在 `src/auth.c` 的 `demo_users` 数组。更改账号或密码后，重新编译程序。当前账号硬编码且密码未安全存储，不要用真实密码。

电价在 `src/main.c` 的 `main()` 中设置：

```c
data.rate.peak_price = 0.6;
data.rate.valley_price = 0.3;
```

权重数组在同一个文件中，顺序必须对应上面列出的四项指标：

```c
static const double recommendation_weights[EXPANSION_INDICATOR_COUNT] = {
    0.25, 0.25, 0.25, 0.25
};
```

四项顺序为：峰电年增长率、峰值月同比增长率、峰谷比、户均月峰电量。程序会按权重总和归一化，因此可以提供任意非负比例，但总和必须大于 0。前端提交的扩容数量和最低电费是筛选条件，不会覆盖这里配置的算法权重。

改变电价时，电费仍按下面公式计算：

```text
bill = peak_kwh × peak_price + valley_kwh × valley_price
```

`billing.c` 提供该公式的计算函数。修改峰谷单价时，先改 `main.c` 的 `data.rate`，再用同一组单价重新计算 CSV 的 `bill`。生成器会根据传入的 `Price` 自动计算新记录的电费。若要从峰谷两档扩展成分时段、阶梯电价等规则，当前 `ElectricBill` 只保存峰、谷两类电量，需要同时设计新的数据字段，并修改计费、CSV、接口和网页，不能只改一个单价。

### 修改模拟数据生成规则

`src/data_generator.c` 中的 `generate_csv()` 接收文件名、小区名称数组、小区数量、每小区户数、起始年份和电价，生成多个小区每户连续两年的月用电 CSV。返回生成记录数；参数不合要求或文件操作失败时返回 `0`。

例如，可在单独的测试程序或临时测试函数中这样调用：

```c
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "data_generator.h"

int main(void)
{
    const char communities[][MODEL_NAME_LENGTH] = {
        "幸福小区", "阳光小区"
    };
    Price price = { 0.6, 0.3 };
    size_t written;

    srand((unsigned)time(NULL));  /* 每次运行使用不同的随机序列 */
    written = generate_csv(
        "electricity_generated.csv",
        communities,
        2,       /* 小区数量 */
        50,      /* 每个小区的住户数，至少 50 */
        2024,    /* 起始年份，生成 2024 和 2025 */
        price);

    printf("生成记录数：%zu\n", written);
    return written == 0 ? 1 : 0;
}
```

这是独立测试程序的示例，需与 `src/data_generator.c` 一起编译运行；不要把第二个 `main()` 直接粘贴到现有项目。相对文件名 `electricity_generated.csv` 会写到程序当前工作目录。

在 Visual Studio 中测试时，新建一个临时 C 控制台项目，把这段示例作为唯一的 `main()`，再把 `src/data_generator.c` 加入项目，并把 `src` 设为头文件搜索目录。不要同时编译原项目的 `main.c`，否则会出现两个 `main()`。

上述调用生成 `2 × 50 × 24 = 2,400` 条记录。每个小区住户编号从 `H001` 开始；每户有 24 条记录，月份为起始年 1 月至下一年 12 月。每月总电量随机为 100–400 kWh，峰时占比随机为 40%–70%，谷时电量为剩余部分，电费按传入电价计算。调用 `srand()` 是为了让每次运行生成不同随机序列；不调用时，随机序列通常会重复。

总记录数必须满足 `小区数量 × 每小区户数 × 24 <= MAX_RECORDS`，当前 `MAX_RECORDS` 为 50,000。生成器用写入模式创建文件，会覆盖同名 CSV，因此示例刻意写入 `electricity_generated.csv`，保留原始演示数据。

**使用生成数据的步骤：**

1. 先备份 `electricity_demo.csv`，并在单独的临时测试程序中调用生成器，输出到新文件名，例如 `electricity_generated.csv`。
2. 检查生成文件的表头、行数、小区名和住户编号。成功返回值应等于预期记录数，例如上例为 2,400。
3. 若希望主程序读取生成数据，停止服务后，将新文件复制/改名为 `electricity_demo.csv`；或者同时修改 `src/main.c` 中 `file_load_records()` 与 `web_server_start()` 的文件名，然后重新编译。两处必须使用同一个 CSV 名称。
4. 若修改了单价，生成器调用中的 `Price` 必须与 `main.c` 中的电价一致。

当前正常启动流程不会自动调用 `generate_csv()`，而是直接读取 `electricity_demo.csv`。只改生成器的随机范围不会自动改变演示 CSV。若打算每次启动都生成数据，应明确把调用放进 `main()` 的读取流程之前；这会重新覆盖生成目标文件，通常不适合保留用户已编辑的记录。

### 修改记录容量和查询分页

目前 `src/model.h` 的 `MAX_RECORDS` 是 50,000；`SystemData` 按该值静态分配记录数组。网页 `web/app.js` 的 `MAX_FRONTEND_RECORDS` 是 40,000，`PAGE_SIZE` 是 50；后端 `src/web_server.c` 的查询分页大小也是 50。若要提高可用数据量，应同时考虑：

1. 提高 `MAX_RECORDS` 并重新编译；
2. 提高 `MAX_FRONTEND_RECORDS`，否则前端报表、排序等最多只读取该上限内的记录；
3. 保证实际 CSV 记录数不超过 `MAX_RECORDS`，并确认电脑内存足够；
4. 若改每页条数，同步改后端和前端的 `50`。分页数值与数据总上限是两种不同设置。

### 修改前端页面

- 页面标题、菜单文字和页面内容在 `web/index.html`；
- 页面切换与接口调用在 `web/app.js`；
- 颜色变量和布局样式在 `web/style.css`；
- 新增菜单时，菜单按钮的 `data-page` 要与页面区域的 `data-view` 对应；同时在 `app.js` 的 `pageTitles` 中登记页面标题。

目前报表和排序没有独立后端 API：网页从 `/api/records` 分页获取筛选后的记录，再在前端汇总。后端每页默认 50 条，前端 `PAGE_SIZE` 也为 50；修改时两边要一致。

### 扩展 API

当前接口路由定义在 `src/web_server.c` 的 `http_event_handler()`：

| 方法 | 路径 | 用途 |
| --- | --- | --- |
| `GET` | `/api/health` | 服务健康检查 |
| `POST` | `/api/login` | 登录 |
| `GET` | `/api/records` | 分页查询记录 |
| `POST` | `/api/records` | 管理员新增记录 |
| `PUT` | `/api/records` | 管理员修改记录 |
| `DELETE` | `/api/records` | 管理员删除记录 |
| `POST` | `/api/recommend` | 生成扩容推荐 |

目前**没有** `/api/stats` 接口。新增 API 时，需要在 `http_event_handler()` 添加路由分支，在 `web_server.c` 实现请求校验和 JSON 响应，并在 `web/app.js` 添加调用；需要登录的接口要沿用现有权限检查。统计逻辑应调用 `statistics.c`，不要在 HTTP 路由中重复实现统计。

## 5. 从源码编译

需要 Windows x64 和 MSVC。打开 **x64 Native Tools Command Prompt for Visual Studio**，进入 `GridWise_Package_Final\src`，执行：

```bat
cl /nologo /O2 /MD /utf-8 /DWIN32_LEAN_AND_MEAN *.c /Fe:"..\GridWise.exe" /link ws2_32.lib shell32.lib
```

随后从 `GridWise_Package_Final` 目录运行 `GridWise.exe`，不要单独移动可执行文件。当前代码使用 `fopen_s`、`sscanf_s`、`strcpy_s` 等 MSVC 函数；GCC/Clang 不能直接按此命令编译。

## 6. 目录结构

```text
.
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── SECURITY.md
├── THIRD_PARTY_NOTICES.md
└── GridWise_Package_Final/
    ├── GridWise.exe
    ├── electricity_demo.csv
    ├── src/       C 源码、头文件和 Mongoose
    └── web/       HTML、JavaScript 和 CSS
```

## 7. 已知限制和许可证

- 仅提供 Windows/MSVC 编译方式。
- 演示账号硬编码，Basic 认证在普通 HTTP 上不加密。
- 每页默认 50 条；报表和排序由前端取回记录后汇总。
- `sorting.c` 使用冒泡排序，复杂度为 O(n²)，当前网页排序使用前端聚合结果排序。
- 推荐只对最近两年数据完整的小区评分。
- 模拟数据生成器要求每个小区至少 50 户，且会覆盖目标文件。

项目按 GNU GPL-2.0-only 发布，完整文本见 [LICENSE](LICENSE)。Mongoose 的上游许可说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。贡献方式和安全说明分别见 [CONTRIBUTING.md](CONTRIBUTING.md) 与 [SECURITY.md](SECURITY.md)。
