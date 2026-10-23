#include "sorting.h"

/* 取得小区峰时总电量 */
static double get_peak_kwh(const CommunityBill* bill)
{
    return bill->total_peak_kwh;
}

/* 取得小区谷时总电量 */
static double get_valley_kwh(const CommunityBill* bill)
{
    return bill->total_valley_kwh;
}

/* 取得小区峰谷总电量 */
static double get_total_kwh(const CommunityBill* bill)
{
    return bill->total_peak_kwh + bill->total_valley_kwh;
}

/* 取得小区总电费 */
static double get_total_bill(const CommunityBill* bill)
{
    return bill->total_bill;
}

/* 取得户均电量 */
static double get_average_kwh(const CommunityBill* bill)
{
    return bill->average_kwh_household;
}

/* 取得户均电费 */
static double get_average_bill(const CommunityBill* bill)
{
    return bill->average_bill_household;
}

/*
 * 按指定指标从高到低排序。
 * bills：小区统计数组
 * count：数组中有效记录数量
 * get_key：取得每条记录排序指标的函数
 * 返回：排序的记录数量；参数无效时返回 0
 */
static size_t sort_community_bills(CommunityBill bills[], size_t count, double (*get_key)(const CommunityBill*))  // 按指定指标从高到低排序
{
    size_t i;
    size_t j;
	CommunityBill temp;     // 临时变量用于交换记录
	//  参数检查
    if (bills == NULL || get_key == NULL || count > MAX_RECORDS) {
        return 0;
    }

    /* 冒泡排序：较大的指标值排在前面 */
    for (i = 0; i < count; i++) {
        for (j = 0; j + 1 < count - i; j++) {
            if (get_key(&bills[j]) < get_key(&bills[j + 1])) {
                temp = bills[j];
                bills[j] = bills[j + 1];
                bills[j + 1] = temp;
            }
        }
    }

    return count;
}

size_t sort_by_peak_kwh(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_peak_kwh);
}

size_t sort_by_valley_kwh(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_valley_kwh);
}

size_t sort_by_total_kwh(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_total_kwh);
}

size_t sort_by_total_bill(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_total_bill);
}

size_t sort_by_average_kwh(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_average_kwh);
}

size_t sort_by_average_bill(CommunityBill bills[], size_t count)
{
    return sort_community_bills(bills, count, get_average_bill);
}
