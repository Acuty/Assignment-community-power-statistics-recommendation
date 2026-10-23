#include "recommendation.h"
#include "statistics.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* 指标顺序与 ExpansionDemand.indicator_weights 保持一致。 */
   
static size_t hash_text(const char* text)  //djb2 哈希函数
{
    size_t hash = 5381;

    while (*text != '\0') {
        hash = hash * 33 + (unsigned char)*text;
        text++;
    }

    return hash;
}

/* 计算“小区、住户”键的哈希值，用于跨月份去重。 */
static size_t hash_household_key(const ElectricBill* record)
{
	size_t hash = hash_text(record->community);       // 小区名写入哈希值
	const unsigned char* text = (const unsigned char*)record->household;   // 引入住户
    hash = hash * 33;
    while (*text != '\0') {
        hash = hash * 33 + *text;
        text++;
    }
    return hash;
}
// 比较函数：按综合优先分降序排列，若相同则按小区名升序排列
static int compare_priority(const void* left, const void* right)
{
    const CommunityBill* a = (const CommunityBill*)left;
    const CommunityBill* b = (const CommunityBill*)right;

    if (a->priority_score < b->priority_score) return 1;
    if (a->priority_score > b->priority_score) return -1;
    return strcmp(a->community, b->community);
}


size_t recommend_communities( const SystemData* data,
    const ExpansionDemand* demand,
    CommunityBill results[],
    size_t result_capacity)
{
    size_t hash_size;
    size_t household_hash_size;
    size_t aggregate_capacity;  
    size_t* community_slots = NULL;
    size_t* household_slots = NULL;
    CommunityBill* monthly = NULL;
    CommunityBill* bills = NULL;
    double* annual_peak = NULL;
    double* monthly_peak = NULL;
    size_t* month_coverage = NULL;
    double* indicators = NULL;
    size_t monthly_count;
    size_t community_count = 0;
    size_t latest_year = 0;
    size_t first_year;
    size_t candidate_count;
    size_t i;
    size_t j;
    size_t slot;
    size_t community_index;
    size_t year_offset;
    size_t peak_month;
    size_t record_index;
    size_t previous_index;
    size_t household_slot;
    size_t month_slot;
    size_t source_index;
    size_t requested_count;
    size_t output_count;
    double minimum;
    double maximum;
    double weight_sum = 0.0;
    //参数检查
    if (data == NULL || demand == NULL || results == NULL ||
        result_capacity == 0 || data->record_count == 0 ||
        data->record_count > MAX_RECORDS ||
        !isfinite(demand->min_total_bill) || demand->min_total_bill < 0.0) {
        return 0;
    }

    /* 权重由调用方提供；允许任意非负比例，后续按总和归一化。 */
    for (j = 0; j < EXPANSION_INDICATOR_COUNT; j++) {
        //判断权重系数是否为有限和非负数
        if (!isfinite(demand->indicator_weights[j]) ||
            demand->indicator_weights[j] < 0.0) {
            return 0;
        }
        weight_sum += demand->indicator_weights[j];   //权重和数
    }
    if (!isfinite(weight_sum) || weight_sum <= 0.0) return 0;    //判断权重和数是否为有限和非负数

    /* 每次推荐开始时只调用一次：从原始 ElectricBill 生成所有月度汇总。 */
    monthly = (CommunityBill*)calloc(data->record_count, sizeof(*monthly));   //分配record_count个的CommunityBill的内存
    if (monthly == NULL) return 0;         
    monthly_count = statistics_all_community_months(data, monthly);     //统计所有的小区月度数据
    //判断如果以上过程失败就释放内存
    if (monthly_count == 0 || monthly_count > MAX_RECORDS) {
        free(monthly);
        return 0;
    }

    /* 选择汇总数据中的最近两个年份。 */
    for (i = 0; i < monthly_count; i++) {
        if (monthly[i].year > latest_year) latest_year = monthly[i].year;
    }
    if (latest_year < 2) {
        free(monthly);
        return 0;
    }
    first_year = latest_year - 1;
    //月度数据引入哈希表
    aggregate_capacity = monthly_count;     //聚合数据容量
    hash_size = aggregate_capacity * 2 + 1;
    household_hash_size = data->record_count * 2 + 1;
    //分配动态内存
    bills = (CommunityBill*)calloc(aggregate_capacity, sizeof(*bills));
    community_slots = (size_t*)calloc(hash_size, sizeof(*community_slots));
    household_slots = (size_t*)calloc( household_hash_size, sizeof(*household_slots));
    annual_peak = (double*)calloc(aggregate_capacity * 2, sizeof(*annual_peak));
    monthly_peak = (double*)calloc(aggregate_capacity * 24, sizeof(*monthly_peak));
    month_coverage = (size_t*)calloc(aggregate_capacity * 24, sizeof(*month_coverage));
    indicators = (double*)calloc(aggregate_capacity * EXPANSION_INDICATOR_COUNT,sizeof(*indicators));
    //判断动态内存是否分配成功
    if (bills == NULL || community_slots == NULL ||
        household_slots == NULL || annual_peak == NULL ||
        monthly_peak == NULL || month_coverage == NULL || indicators == NULL) {
        free(monthly);
        free(bills);
        free(community_slots);
        free(annual_peak);
        free(monthly_peak);
        free(month_coverage);
        free(indicators);
        free(household_slots);
        return 0;
    }

    /* 将月度汇总压成小区两年汇总；哈希查找使此处平均为线性时间。 */
    for (i = 0; i < monthly_count; i++) {
        const CommunityBill* month = &monthly[i];
        //数据清洗
        if (month->community[0] == '\0' ||
            month->year < first_year || month->year > latest_year ||
            month->month < 1 || month->month > 12) {
            continue;
        }
        
        year_offset = month->year - first_year;              //算年份偏移量
        slot = hash_text(month->community) % hash_size;     //算哈希表位置
        //如果哈希表位置被占
        while (community_slots[slot] != 0) {
            community_index = community_slots[slot] - 1;     //取出哈希表存的索引 
            if (strcmp(bills[community_index].community, month->community) == 0) break;  //如果信息一致，跳出循环
            slot = (slot + 1) % hash_size;    //不一致，线性探查
        }
        //如果空槽，就建表
        if (community_slots[slot] == 0) {        
            community_index = community_count++;
            community_slots[slot] = community_index + 1;
            if (strcpy_s(bills[community_index].community,
                sizeof(bills[community_index].community),
                month->community) != 0) {
                free(monthly);
                free(bills);
                free(community_slots);
                free(annual_peak);
                free(monthly_peak);
                free(month_coverage);
                free(indicators);
                free(household_slots);
                return 0;
            }
            bills[community_index].year = 0;
            bills[community_index].month = 0;
            bills[community_index].period_start_year = first_year;
            bills[community_index].period_end_year = latest_year;
        }
        else {
            community_index = community_slots[slot] - 1;
        }
        //数据相加
        bills[community_index].total_peak_kwh += month->total_peak_kwh;
        bills[community_index].total_valley_kwh += month->total_valley_kwh;
        bills[community_index].total_bill += month->total_bill;
        bills[community_index].household_bills_count += month->household_bills_count;
        annual_peak[community_index * 2 + year_offset] += month->total_peak_kwh;
        month_slot = community_index * 24 + year_offset * 12 +
            (month->month - 1);
        monthly_peak[month_slot] += month->total_peak_kwh;
        month_coverage[month_slot]++;
    }
    //建表失败释放缓存
    if (community_count == 0) {
        free(monthly);
        free(bills);
        free(community_slots);
        free(annual_peak);
        free(monthly_peak);
        free(month_coverage);
        free(indicators);
        free(household_slots);
        return 0;
    }

    /* 两年汇总中的户数按不同住户去重，不把每个月重复出现的住户重复计数。*/
    for (record_index = 0; record_index < data->record_count; record_index++) {
        const ElectricBill* record = &data->records[record_index];

        if (record->year < first_year || record->year > latest_year) {
            continue;
        }

        slot = hash_text(record->community) % hash_size;
        while (community_slots[slot] != 0) {
            community_index = community_slots[slot] - 1;
            if (strcmp(bills[community_index].community,
                record->community) == 0) {
                break;
            }
            slot = (slot + 1) % hash_size;
        }

        if (community_slots[slot] != 0) {
            community_index = community_slots[slot] - 1;
            household_slot =
                hash_household_key(record) % household_hash_size;

            while (household_slots[household_slot] != 0) {
                previous_index = household_slots[household_slot] - 1;
                if (strcmp(data->records[previous_index].community,
                        record->community) == 0 &&
                    strcmp(data->records[previous_index].household,
                        record->household) == 0) {
                    break;
                }
                household_slot =
                    (household_slot + 1) % household_hash_size;
            }

            if (household_slots[household_slot] == 0) {
                household_slots[household_slot] = record_index + 1;
                bills[community_index].household_count++;
            }
        }
    } 
    /* 指标比较要求统计周期一致；缺少任一月份的小区不参与排名。  */
    candidate_count = 0;
    for (source_index = 0; source_index < community_count; source_index++) {
        size_t month_index;
        size_t complete = 1;

        for (month_index = 0; month_index < 24; month_index++) {
            if (month_coverage[source_index * 24 + month_index] == 0) {
                complete = 0;
                break;
            }
        }

        if (complete != 0) {
            if (candidate_count != source_index) {
                bills[candidate_count] = bills[source_index];
                memcpy(&annual_peak[candidate_count * 2],
                    &annual_peak[source_index * 2],
                    2 * sizeof(*annual_peak));
                memcpy(&monthly_peak[candidate_count * 24],
                    &monthly_peak[source_index * 24],
                    24 * sizeof(*monthly_peak));
            }
            candidate_count++;
        }
    }
   
    free(month_coverage);
    month_coverage = NULL;

    if (candidate_count == 0) {
        free(monthly);
        free(bills);
        free(community_slots);
        free(annual_peak);
        free(monthly_peak);
        free(indicators);
        free(household_slots);
        return 0;
    }

    /* 对所有小区计算四项正向指标。 */
    for (i = 0; i < candidate_count; i++) {
        double year_one_peak = annual_peak[i * 2];
        double year_two_peak = annual_peak[i * 2 + 1];
        double previous_month_peak;         
        double latest_month_peak;               

        indicators[i * EXPANSION_INDICATOR_COUNT] = year_one_peak > 0.0
            ? (year_two_peak - year_one_peak) / year_one_peak : 0.0;  //如果第一年为0，增长率为0，否则计算

        /* 第二年峰电量最大的月份，与第一年同月比较。 */
        peak_month = 0;
        for (j = 1; j < 12; j++) {
            if (monthly_peak[i * 24 + 12 + j] >
                monthly_peak[i * 24 + 12 + peak_month]) {
                peak_month = j;
            }
        }
        //逐次判断非0计算
        previous_month_peak = monthly_peak[i * 24 + peak_month];
        latest_month_peak = monthly_peak[i * 24 + 12 + peak_month];
        //算峰值月同比增长
        indicators[i * EXPANSION_INDICATOR_COUNT + 1] = previous_month_peak > 0.0
            ? (latest_month_peak - previous_month_peak) / previous_month_peak : 0.0;
        //峰谷比
        indicators[i * EXPANSION_INDICATOR_COUNT + 2] = bills[i].total_valley_kwh > 0.0
            ? bills[i].total_peak_kwh / bills[i].total_valley_kwh : 0.0;
        //户均月峰电
        indicators[i * EXPANSION_INDICATOR_COUNT + 3] = bills[i].household_bills_count > 0
            ? bills[i].total_peak_kwh / bills[i].household_bills_count : 0.0;
        //户均用电存入bill
        bills[i].average_kwh_household = bills[i].household_count > 0
            ? (bills[i].total_peak_kwh + bills[i].total_valley_kwh) /
            bills[i].household_count : 0.0;
        bills[i].average_bill_household = bills[i].household_count > 0
            ? bills[i].total_bill / bills[i].household_count : 0.0;
    }

    /* 正向指标 min-max 归一化；常量指标整列置 0，避免除零。 */
    for (j = 0; j < EXPANSION_INDICATOR_COUNT; j++) {
        minimum = indicators[j];
        maximum = indicators[j];
        for (i = 1; i < candidate_count; i++) {
            double value = indicators[i * EXPANSION_INDICATOR_COUNT + j];
            if (value < minimum) minimum = value;
            if (value > maximum) maximum = value;
        }
        if (maximum > minimum) {
            for (i = 0; i < candidate_count; i++) {
                indicators[i * EXPANSION_INDICATOR_COUNT + j] =
                    (indicators[i * EXPANSION_INDICATOR_COUNT + j] - minimum) /
                    (maximum - minimum);
            }
        }
        else {
            for (i = 0; i < candidate_count; i++) {
                indicators[i * EXPANSION_INDICATOR_COUNT + j] = 0.0;
            }
        }
    }

    /* 使用 ExpansionDemand 中给定的权重计算综合优先分。 */
    for (i = 0; i < candidate_count; i++) {
        bills[i].priority_score = 0.0;
        for (j = 0; j < EXPANSION_INDICATOR_COUNT; j++) {
            bills[i].priority_score +=
                (demand->indicator_weights[j] / weight_sum) *
                indicators[i * EXPANSION_INDICATOR_COUNT + j];
        }
    }

    qsort(bills, candidate_count, sizeof(*bills), compare_priority);  //标准库排序

    /* 最后应用需求：电费门槛按两年总电费计算，数量 0 表示不限。 */
    requested_count = demand->target_community_count == 0
        ? candidate_count : demand->target_community_count;
    output_count = 0;
    for (i = 0; i < candidate_count && output_count < requested_count && output_count < result_capacity; i++) {
        if (bills[i].total_bill >= demand->min_total_bill) {
            results[output_count++] = bills[i];
        }
    }

    free(monthly);
    free(bills);
    free(community_slots);
    free(annual_peak);
    free(monthly_peak);
    free(month_coverage);
    free(indicators);
    free(household_slots);
    return output_count;
}
