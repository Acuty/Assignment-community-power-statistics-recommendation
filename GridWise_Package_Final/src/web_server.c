
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mongoose.h"
#include "web_server.h"
#include "auth.h"
#include "billing.h"
#include "data_manager.h"
#include "file_manager.h"
#include "query.h"
#include "recommendation.h"

static struct mg_mgr server_manager;
static SystemData* server_data = NULL;
static const char* server_csv_filename = NULL;
static size_t server_started = 0;
static double server_indicator_weights[EXPANSION_INDICATOR_COUNT];
/* 返回 JSON 错误消息 */

static void reply_error( struct mg_connection* connection,int status, const char* message)
{
    mg_http_reply(
        connection,
        status,
        "Content-Type: application/json; charset=utf-8\r\n",
        "{\"error\":%m}\n",
        MG_ESC(message));
}

/* 用小区、住户、年份、月份定位一条记录 */
static size_t find_record(
    struct mg_http_message* request,
    size_t* found_index)
{
    char community[MODEL_NAME_LENGTH] = "";
    char household[MODEL_NAME_LENGTH] = "";
    char year_text[16] = "";
    char month_text[16] = "";
    char* end;
    unsigned long year_value;
    unsigned long month_value;
    size_t i;
    //获取数据，并判断有效
    if (mg_http_get_var(&request->query, "community",
        community, sizeof(community)) <= 0 ||
        mg_http_get_var(&request->query, "household",
            household, sizeof(household)) <= 0 ||
        mg_http_get_var(&request->query, "year",
            year_text, sizeof(year_text)) <= 0 ||
        mg_http_get_var(&request->query, "month",
            month_text, sizeof(month_text)) <= 0) {
        return 0;
    }
    //把字符串前面的数字部分转成 unsigned long
    year_value = strtoul(year_text, &end, 10);
    if (*end != '\0' || year_value == 0) return 0;
    //把字符串前面的数字部分转成 unsigned long
    month_value = strtoul(month_text, &end, 10);
    if (*end != '\0' || month_value < 1 || month_value > 12) return 0;
    //遍历数据找出匹配的
    for (i = 0; i < server_data->record_count; i++) {
        ElectricBill* record = &server_data->records[i];

        if (strcmp(record->community, community) == 0 &&
            strcmp(record->household, household) == 0 &&
            record->year == (size_t)year_value &&
            record->month == (size_t)month_value) {
            *found_index = i;
            return 1;
        }
    }

    return 0;
}


/* 按 URL 条件查询记录的页码，缺省为第 1 页 */
static size_t read_page(struct mg_http_message* request, size_t* page)
{
    char page_text[16] = "";
    char* end;
    unsigned long value;

    *page = 1;

    if (mg_http_get_var(&request->query, "page",
        page_text, sizeof(page_text)) <= 0) {
        return 1;
    }

    value = strtoul(page_text, &end, 10);

    if (*end != '\0' || value == 0) {
        return 0;
    }

    *page = (size_t)value;
    return 1;
}

/* 将小区名称转义为可安全写入 JSON 字符串的内容。
static size_t json_escape(const char* source, char* destination, size_t capacity)
{
    static const char hex[] = "0123456789abcdef";
    size_t used = 0;
    const unsigned char* current = (const unsigned char*)source;

    if (source == NULL || destination == NULL || capacity == 0) {
        return (size_t)-1;
    }

    while (*current != '\0') {
        size_t needed = 1;
        if (*current == '"' || *current == '\\') {
            needed = 2;
        }
        else if (*current < 0x20) {
            needed = 6;
        }

        if (used + needed >= capacity) return (size_t)-1;

        if (*current == '"' || *current == '\\') {
            destination[used++] = '\\';
            destination[used++] = (char)*current;
        }
        else if (*current < 0x20) {
            destination[used++] = '\\';
            destination[used++] = 'u';
            destination[used++] = '0';
            destination[used++] = '0';
            destination[used++] = hex[*current >> 4];
            destination[used++] = hex[*current & 0x0f];
        }
        else {
            destination[used++] = (char)*current;
        }
        current++;
    }

    destination[used] = '\0';
    return used;
}
*/

/* 健康检查接口 */
static void handle_health(struct mg_connection* connection)
{
    mg_http_reply(connection, 200,
        "Content-Type: application/json\r\n",
        "{\"status\":\"ok\"}\n");
}
/* 解析新增或修改记录的 JSON 请求体 */
static size_t parse_record_body(struct mg_http_message* request, ElectricBill* record)
{
    char* community = NULL;
    char* household = NULL;
    double year_value;
    double month_value;
    double peak_kwh;
    double valley_kwh;
    size_t valid = 0;
    //检查参数
    if (request == NULL || record == NULL) {
        return 0;
    }
    //获取数据的community，household
    community = mg_json_get_str(request->body, "$.community");
    household = mg_json_get_str(request->body, "$.household");
    //检查参数为空
    if (community == NULL || household == NULL ||
        !mg_json_get_num(request->body, "$.year", &year_value) ||
        !mg_json_get_num(request->body, "$.month", &month_value) ||
        !mg_json_get_num(request->body, "$.peak_kwh", &peak_kwh) ||
        !mg_json_get_num(request->body, "$.valley_kwh", &valley_kwh)) {
        goto cleanup;
    }
    //检查数据逻辑问题
    if (!isfinite(year_value) || !isfinite(month_value) ||
        !isfinite(peak_kwh) || !isfinite(valley_kwh) ||
        year_value < 1 || year_value > 9999 ||
        month_value < 1 || month_value > 12 ||
        peak_kwh < 0 || valley_kwh < 0 ||
        year_value != floor(year_value) ||
        month_value != floor(month_value)) {
        goto cleanup;
    }
    //初始化record
    *record = (ElectricBill){ 0 };
    //如果复制成功执行，写入
    if (strcpy_s(record->community, sizeof(record->community),
        community) != 0 ||
        strcpy_s(record->household, sizeof(record->household),
            household) != 0) {
        goto cleanup;
    }

    record->year = (size_t)year_value;
    record->month = (size_t)month_value;
    record->peak_kwh = peak_kwh;
    record->valley_kwh = valley_kwh;

    record->bill = billing_calculate(
        record->peak_kwh,
        record->valley_kwh,
        server_data->rate);
    //做代码检查 成功返回1，失败返回0
    valid = data_validate_record(record);    
    //释放内存，如果前面触发跳到cleanup,按顺序也会触发cleanup
cleanup:
    if (community != NULL) {mg_free(community);
 }

    if (household != NULL) {
        mg_free(household);
    }

    return valid;
}

//权限管理

//获取请求中的身份认证，用Basic Auth
static size_t get_request_role(struct mg_http_message* request)
{
    //定义字符数组和系统用户信息结构体
    char account[MODEL_ACCOUNT_LENGTH] = "";
    char password[MODEL_PASSWORD_LENGTH] = "";
    User user;

    //该函数会获取请求的账户密码
    mg_http_creds(request, account, sizeof(account), password, sizeof(password));
    //验证账号密码是否正确
    if (auth_login(account, password, &user) == 0) {
        return 0;
    }
    return  (size_t)user.role;

}
//检查请求是否已登录，拥有基础权限
static size_t require_login(struct mg_connection* connection, struct mg_http_message* request)
{
    size_t role = get_request_role(request);
    //判断是否登录
    if (role == 0) {
        reply_error(connection, 401, "请先登录");
        return 0;
    }
    return 1;
}
//检查是否拥有管理员身份
static size_t require_admin(struct mg_connection* connection, struct mg_http_message* request)
{
    size_t role = get_request_role(request);
    //判断是否登录
    if (role == 0) {
        reply_error(connection, 401, "请先登录");
        return 0;
    }

    if (role != ROLE_ADMIN) {
        reply_error(connection, 403, "只有管理员可以修改数据");
        return 0;
    }
    return 1;
}
//登录函数(包括连接对象，以及http的message)
static void handle_login(
    struct mg_connection* connection,
    struct mg_http_message* request)
{
    //从message的主体里面获取账户密码的路径
    char* account = mg_json_get_str(request->body, "$.account");
    char* password = mg_json_get_str(request->body, "$.password");
    User user;   //初始化一个用户结构体
    //判断账号密码是否正确或者是空的
    if (account == NULL || password == NULL ||
        auth_login(account, password, &user) == 0) {
        if (account != NULL) mg_free(account);
        if (password != NULL) mg_free(password);

        reply_error(connection, 401, "账号或密码错误");
        return;
    }

    //charset=utf-8为存在中文时在响应头中必须声明的
    mg_http_reply(connection, 200,
        "Content-Type: application/json; charset=utf-8\r\n",
        "{\"account\":\"%s\",\"role\":%d}\n",
        user.account,
        (int)user.role);

    mg_free(account);
    mg_free(password);
}




//推荐函数的调用
static void handle_recommend(struct mg_connection* connection,struct mg_http_message* request)
{
    // 1. 从请求体读出的参数
    double target_count_value;      //推荐小区
    double minimum_bill;            //最低的电量标准
    // 2. 输入/输出结构体
    ExpansionDemand demand;      // 输入：扩容需求
    CommunityBill* results;      // 输出：推荐结果数组（动态分配）
    // 3. JSON 输出相关
    char json[65536];             // 最终 JSON 字符串
    size_t used = 0;
    size_t written;
    // 4. 计数/索引
    size_t result_count;         // 实际推荐条数
    size_t result_capacity;      // results 数组容量
    size_t i;                    // 循环变量

    //判断是否登录
    if (require_login(connection, request) == 0)return;

    //如果target_count_value在请求中没找到，或者不满足条件，参数无效，直接退出
    //如果minimum_bill在请求中没找到，或者不满足条件，参数无效，直接退出
    if (!mg_json_get_num(request->body,"$.target_community_count",&target_count_value) ||
        !mg_json_get_num( request->body,"$.min_total_bill",&minimum_bill) ||
        !isfinite(target_count_value) ||
        target_count_value < 0.0 ||
        target_count_value >(double)MAX_RECORDS ||
        floor(target_count_value) != target_count_value ||
        !isfinite(minimum_bill) ||
        minimum_bill < 0.0) {
        reply_error(connection, 400, "扩容需求参数无效");
        return;
    }

    //赋值给ExpansionDemand的demand
    demand.target_community_count = (size_t)target_count_value;
    demand.min_total_bill = minimum_bill;

    //赋值请求中的权重给demand
    memcpy(demand.indicator_weights, server_indicator_weights, sizeof(demand.indicator_weights));

    //获取全局SystemData* server_data的最大记录数
    result_capacity = server_data->record_count;

    //如果最大记录数为零，为了分配内存，改为1
    if (result_capacity == 0) {
        result_capacity = 1;
    }

    //分配CommunityBill内存 
    results = (CommunityBill*)calloc( result_capacity,sizeof(*results));
    if (results == NULL) {
        reply_error(connection, 500, "推荐结果内存分配失败");
        return;
    }

    //调用推荐算法
    result_count = recommend_communities(server_data, &demand, results, result_capacity);
    //判断推荐算法是否有效
    if (result_count > result_capacity) {
        free(results);
        reply_error(connection, 500, "推荐结果数量异常");
        return;
    }
    //规定json的响应体开头
    used = mg_snprintf(json,sizeof(json),"[");
    //规定json的响应体主体依次写入results
    for (i = 0; i < result_count; i++) {
        written = mg_snprintf(
            json + used,
            sizeof(json) - used,
            "%s{"
            "\"community\":%m,"
            "\"year\":%zu,"
            "\"month\":%zu,"
            "\"period_start_year\":%zu,"
            "\"period_end_year\":%zu,"
            "\"total_peak_kwh\":%.2f,"
            "\"total_valley_kwh\":%.2f,"
            "\"total_bill\":%.2f,"
            "\"household_count\":%zu,"
            "\"household_bills_count\":%zu,"
            "\"average_kwh_household\":%.2f,"
            "\"average_bill_household\":%.2f,"
            "\"priority_score\":%.6f"
            "}",
            i == 0 ? "" : ",",
            MG_ESC(results[i].community),
            results[i].year,
            results[i].month,
            results[i].period_start_year,
            results[i].period_end_year,
            results[i].total_peak_kwh,
            results[i].total_valley_kwh,
            results[i].total_bill,
            results[i].household_count,
            results[i].household_bills_count,
            results[i].average_kwh_household,
            results[i].average_bill_household,
            results[i].priority_score);

        if (written >= sizeof(json) - used) {
            free(results);
            reply_error(connection, 500, "推荐结果过长");
            return;
        }

        used += written;
    }
    //规定json的响应体结束
    json[used++] = ']';
    json[used] = '\0';
    //成功回复
    mg_http_reply(connection, 200, "Content-Type: application/json; charset=utf-8\r\n","%s\n",json);
    free(results);
}
/* 查询记录：返回 ElectricBill 数组，不在后端做汇总 */
static void handle_get_records( struct mg_connection* connection,struct mg_http_message* request)
{
    //声明用于接受的query需要的传参
    ElectricBill filter = { 0 };
    ElectricBill results[50];
    //
    char community[MODEL_NAME_LENGTH] = "";
    char household[MODEL_NAME_LENGTH] = "";
    char year_text[16] = "";
    char month_text[16] = "";
    char json[32768];
    char* end;
    unsigned long value;
    size_t page;
    size_t result_count;
    size_t i;
    size_t used;
    size_t written;
    //登录
    if (require_login(connection, request) == 0) return;
    //检查小区名，出错返回400
    if (mg_http_get_var( &request->query, "community", community, sizeof(community)) <= 0) {
        reply_error(connection, 400, "请提供小区名称");
        return;
    }

    //获取household
    mg_http_get_var(
        &request->query,
        "household",
        household,
        sizeof(household));
    //获取year
    mg_http_get_var(
        &request->query,
        "year",
        year_text,
        sizeof(year_text));
    //获取month
    mg_http_get_var(
        &request->query,
        "month",
        month_text,
        sizeof(month_text));
    //复制community到filter
    strcpy_s(
        filter.community,
        sizeof(filter.community),
        community);
    //检查格式问题
    if (household[0] != '\0') {
        strcpy_s(
            filter.household,
            sizeof(filter.household),
            household);
    }
    if (year_text[0] != '\0') {
        value = strtoul(year_text, &end, 10);

        if (*end != '\0' || value == 0) {
            reply_error(connection, 400, "年份格式不正确");
            return;
        }

        filter.year = (size_t)value;
    }
    if (month_text[0] != '\0') {
        value = strtoul(month_text, &end, 10);

        if (*end != '\0' || value < 1 || value > 12) {
            reply_error(connection, 400, "月份必须是 1 到 12");
            return;
        }
        filter.month = (size_t)value;
    }
    if (read_page(request, &page) == 0) {
        reply_error(connection, 400, "页码必须是正整数");
        return;
    }


    result_count = query_page(server_data, &filter,page,50,results,50);
    //写json
    used = mg_snprintf(json, sizeof(json), "[");
    for (i = 0; i < result_count; i++) {
        written = mg_snprintf(
            json + used,
            sizeof(json) - used,
            "%s{"
            "\"community\":%m,"
            "\"household\":%m,"
            "\"year\":%zu,"
            "\"month\":%zu,"
            "\"peak_kwh\":%.2f,"
            "\"valley_kwh\":%.2f,"
            "\"bill\":%.2f"
            "}",
            i == 0 ? "" : ",",
            MG_ESC(results[i].community),
            MG_ESC(results[i].household),
            results[i].year,
            results[i].month,
            results[i].peak_kwh,
            results[i].valley_kwh,
            results[i].bill);

        if (written >= sizeof(json) - used) {
            reply_error(connection, 500, "查询结果过长");
            return;
        }
        used += written;
    }
    if (used + 2 > sizeof(json)) {
        reply_error(connection, 500, "查询结果过长");
        return;
    }
    json[used++] = ']';
    json[used] = '\0';
    //回复成功
    mg_http_reply(
        connection,
        200,
        "Content-Type: application/json; charset=utf-8\r\n",
        "%s\n",
        json);
}

/* 新增记录 */
static void handle_add_record( struct mg_connection* connection,struct mg_http_message* request)
{
    ElectricBill record;    //为新增记录准备结构体
    //判断管理员
    if (require_admin(connection, request) == 0) return;
    //参数检查失败返回
    if (parse_record_body(request, &record) == 0) {
        reply_error(connection, 400, "记录内容无效");
        return;
    }
    //增加失败返回
    if (data_add_record(server_data, &record) == 0) {
        reply_error(connection, 400, "记录无效或记录数量已满");
        return;
    }
    //保存失败返回
    if (file_save_records(server_csv_filename, server_data)
        != server_data->record_count) {
        data_delete_record(server_data, server_data->record_count - 1);
        reply_error(connection, 500, "保存 CSV 失败");
        return;
    }
    //返回创建成功201
    mg_http_reply(connection, 201,
        "Content-Type: application/json\r\n",
        "{\"status\":\"added\",\"bill\":%.2f}\n", record.bill);
}

/* 修改记录：URL 参数定位原记录，请求体提供新内容 */
static void handle_update_record(
    struct mg_connection* connection,
    struct mg_http_message* request)
{
    size_t index;
    ElectricBill old_record;
    ElectricBill new_record;
    //管理员检查
    if (require_admin(connection, request) == 0) return;
    //是否找到匹配数据，没有找到无法写入，找到索引给index
    if (find_record(request, &index) == 0) {
        reply_error(connection, 404, "没有找到要修改的记录");
        return;
    }
    //检查新增准确性
    if (parse_record_body(request, &new_record) == 0) {
        reply_error(connection, 400, "新记录内容无效");
        return;
    }
    //获取旧数据
    old_record = server_data->records[index];
    //更新
    if (data_update_record(server_data, index, &new_record) == 0) {
        reply_error(connection, 400, "修改记录失败");
        return;
    }
    //保存
    if (file_save_records(server_csv_filename, server_data)
        != server_data->record_count) {
        data_update_record(server_data, index, &old_record);
        reply_error(connection, 500, "保存 CSV 失败");
        return;
    }

    mg_http_reply(connection, 200,
        "Content-Type: application/json\r\n",
        "{\"status\":\"updated\",\"bill\":%.2f}\n", new_record.bill);
}

/* 删除记录 */
static void handle_delete_record(
    struct mg_connection* connection,
    struct mg_http_message* request)
{
    size_t index;
    ElectricBill old_record;
    //管理员身份
    if (require_admin(connection, request) == 0) return;
    //定位
    if (find_record(request, &index) == 0) {
        reply_error(connection, 404, "没有找到要删除的记录");
        return;
    }
    //获取旧数据
    old_record = server_data->records[index];
    //删除
    if (data_delete_record(server_data, index) == 0) {
        reply_error(connection, 400, "删除记录失败");
        return;
    }
    //保存
    if (file_save_records(server_csv_filename, server_data)
        != server_data->record_count) {
        data_add_record(server_data, &old_record);
        reply_error(connection, 500, "保存 CSV 失败");
        return;
    }

    mg_http_reply(connection, 200,
        "Content-Type: application/json\r\n",
        "{\"status\":\"deleted\"}\n");
}



//路由分发
static void http_event_handler(struct mg_connection* connection, int event, void* event_data)
{
    struct mg_http_message* request;   //保存http传递的内容
    if (event != MG_EV_HTTP_MSG) return;  //判断是不是由一条完整的 HTTP 请求解析好了，没有结束函数
    request = (struct mg_http_message*)event_data;
    if (request == NULL) { return;}
    //如果这个请求的 URL 是 /api/health，就调用健康检查函数
    if (mg_match(request->uri, mg_str("/api/health"), NULL)) {
        handle_health(connection);
    }
    //如果这个请求的 URL 是 /api/login，并且调用POST方法，就调用登录函数
    else if (mg_match(request->uri, mg_str("/api/login"), NULL) &&
        mg_match(request->method, mg_str("POST"), NULL)) {
        handle_login(connection, request);
    }
    //如果这个请求的 URL 是 /api/recommend，并且调用POST方法，就调用推荐函数
    else if (mg_match(request->uri, mg_str("/api/recommend"), NULL)) {
        if (mg_match(request->method, mg_str("POST"), NULL)) {
            handle_recommend(connection, request);
        }
        else {
            reply_error(connection, 405, "不支持的请求方法");
        }
    }
    //如果这个请求的 URL 是 /api/records，并且判断是否为增删改查对应方法，就调用登录函数
    else if (mg_match(request->uri, mg_str("/api/records"), NULL)) {
        if (mg_match(request->method, mg_str("GET"), NULL)) {
            handle_get_records(connection, request);
        }
        else if (mg_match(request->method, mg_str("POST"), NULL)) {
            handle_add_record(connection, request);
        }
        else if (mg_match(request->method, mg_str("PUT"), NULL)) {
            handle_update_record(connection, request);
        }
        else if (mg_match(request->method, mg_str("DELETE"), NULL)) {
            handle_delete_record(connection, request);
        }
        else {
            reply_error(connection, 405, "不支持的请求方法");
        }
    }
    //"如果前面所有 API 接口都没匹配上，而且方法是 GET 或 HEAD，那就从 web 文件夹里找对应的网页文件返回给浏览器。
    else if (mg_match(request->method, mg_str("GET"), NULL) ||
        mg_match(request->method, mg_str("HEAD"), NULL)) {
        struct mg_http_serve_opts options = { 0 };   //定义一个配置结构体，提供静态文件
        options.root_dir = "web";       //将结构体根目录设在名为"web"
        mg_http_serve_dir(connection, request, &options);   //返回静态文件 
    }
    else {
        reply_error(connection, 405, "不支持的请求方法");
    }

}

/* 启动 HTTP 服务 */

//初始化server_data ，监听地址，文件名，权重指标
size_t web_server_start(
    SystemData* data,
    const char* address,
    const char* csv_filename,
    const double indicator_weights[EXPANSION_INDICATOR_COUNT])
{
    size_t i;
    double weight_sum = 0.0;

    if (data == NULL || address == NULL || csv_filename == NULL ||
        indicator_weights == NULL || data->record_count > MAX_RECORDS ||
        server_started != 0) {
        return 0;
    }

    for (i = 0; i < EXPANSION_INDICATOR_COUNT; i++) {
        if (!isfinite(indicator_weights[i]) || indicator_weights[i] < 0.0) {
            return 0;
        }
        weight_sum += indicator_weights[i];
    }
    if (!isfinite(weight_sum) || weight_sum <= 0.0) {
        return 0;
    }

    memcpy(server_indicator_weights, indicator_weights,
        sizeof(server_indicator_weights));
    server_data = data;
    server_csv_filename = csv_filename;

    mg_mgr_init(&server_manager);

    if (mg_http_listen(&server_manager,
        address, http_event_handler, NULL) == NULL) {
        mg_mgr_free(&server_manager);
        server_data = NULL;
        server_csv_filename = NULL;
        return 0;
    }

    server_started = 1;
    return 1;
}

/* 处理一次网络事件 */
void web_server_poll(void)
{
    if (server_started != 0) {
        mg_mgr_poll(&server_manager, 1000);
    }
}

/* 停止服务 */
void web_server_stop(void)
{
    if (server_started != 0) {
        mg_mgr_free(&server_manager);
        server_started = 0;
        server_data = NULL;
        server_csv_filename = NULL;
    }
}
