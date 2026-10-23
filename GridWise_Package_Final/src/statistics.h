#ifndef STATISTICS_H
#define STATISTICS_H

#include "model.h"

/* 查某户某年，汇总全年用电量和电费 */
size_t statistics_household_year(
    const SystemData* data,
    const ElectricBill* condition,
    double* total_kwh,
    double* total_bill
);
/* 计算指定小区某月的统计结果
 * 调用前：result 中填写 community、year、month
 * 调用后：result 中填写电量、电费、户数和户均数据
 * 返回值：匹配到的月记录数；无记录或参数无效时返回 0
 */
size_t statistics_community_month(
    const SystemData* data,
    CommunityBill* result
);

/* 汇总所有小区每月指标；返回“小区+年份+月份”分组数量 */
size_t statistics_all_community_months(
    const SystemData* data,
    CommunityBill results[MAX_RECORDS]
);
#endif
