// 本文件统一定义系统各模块共用的数据结构。各模块应使用本文件中的定义。
#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>  // 定义 size_t

#define MODEL_NAME_LENGTH 64
#define MODEL_ACCOUNT_LENGTH 32
#define MODEL_PASSWORD_LENGTH 32
#define MAX_RECORDS 50000
#define EXPANSION_INDICATOR_COUNT 4

// 用户角色
typedef enum {
    ROLE_ADMIN = 1,
    ROLE_USER = 2
} Role;

// 系统用户信息
typedef struct {
    char account[MODEL_ACCOUNT_LENGTH]; 
    char password[MODEL_PASSWORD_LENGTH];
    Role role;
} User;

// 单个家庭单月用电记录
typedef struct {
    char community[MODEL_NAME_LENGTH];
    char household[MODEL_NAME_LENGTH];
    size_t year;
    size_t month;
    double peak_kwh;
    double valley_kwh;
    double bill;  
} ElectricBill;

// 峰时和谷时电价
typedef struct {
    double peak_price;
    double valley_price;
} Price;

// 小区用电统计信息
typedef struct {
    char community[MODEL_NAME_LENGTH];
    size_t year;                 // 单月统计的年份；跨年汇总时为 0
    size_t month;                // 单月统计的月份；跨年汇总时为 0
    size_t period_start_year;
    size_t period_end_year;
    double total_peak_kwh;
    double total_valley_kwh;
    double total_bill;
    size_t household_count;
    size_t household_bills_count;
    double average_kwh_household;
    double average_bill_household;
    double priority_score;
} CommunityBill;



// 扩容筛选需求及四项推荐指标权重
typedef struct {
    size_t target_community_count;
    double min_total_bill;
    /* 峰电年增长、峰值月同比增长、峰谷比、户均月峰电 */
    double indicator_weights[EXPANSION_INDICATOR_COUNT];
} ExpansionDemand;

// 系统数据
typedef struct {
    ElectricBill records[MAX_RECORDS];
    size_t record_count;
    Price rate;
} SystemData;

#endif 
