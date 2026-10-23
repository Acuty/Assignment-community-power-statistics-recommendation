#include "statistics.h"
#include <stdlib.h>
#include <string.h>

// 计算“小区、年份、月份”键的哈希值，用于跨月份去重。
static size_t hash_community_month(const ElectricBill* record)
{
	size_t hash = 5381;
	size_t i;
	//将小区名称复制到哈希值中
	//将字符串中的每个字符都加入哈希值中
	for (i = 0; i < MODEL_NAME_LENGTH && record->community[i] != '\0'; i++) {
		hash = hash * 33 + (unsigned char)record->community[i];
	}
	//将年份和月份加入哈希值中
	hash = hash * 33 + record->year;
	hash = hash * 33 + record->month;
	return hash;
}


// 计算指定住户的年份的总用电量和总电费，返回匹配的记录数。
size_t statistics_household_year(
	const SystemData* data,
	const ElectricBill* condition,
	double* total_kwh,
	double* total_bill)
{
	size_t i;
	size_t matched_count = 0;
	// 参数检测
	if (data == NULL || condition == NULL ||
		total_kwh == NULL || total_bill == NULL ||
		data->record_count > MAX_RECORDS) {
		return 0;
	}

	*total_kwh = 0.0;
	*total_bill = 0.0;

	for (i = 0; i < data->record_count; i++) {
		const ElectricBill* record = &data->records[i];

		if (strcmp(record->community, condition->community) == 0 
			&&strcmp(record->household, condition->household) == 0
			&&record->year == condition->year) {
			*total_kwh += record->peak_kwh + record->valley_kwh;
			*total_bill += record->bill;
			matched_count++;
		}
	}
	return matched_count;
}

// 计算指定小区的年份和月份的总用电量和总电费，返回匹配的记录数。
size_t statistics_community_month(
	const SystemData* data,
	CommunityBill* result)
{
	size_t i;
	// 参数检测
	if (data == NULL || result == NULL ||
		data->record_count > MAX_RECORDS) {
		return 0;
	}
	// 传递内存初始化
	result->total_peak_kwh = 0.0;
	result->total_valley_kwh = 0.0;
	result->total_bill = 0.0;
	result->household_count = 0;
	result->household_bills_count = 0;
	result->average_kwh_household = 0.0;
	result->average_bill_household = 0.0;
	result->priority_score = 0.0;
	result->period_start_year = result->year;
	result->period_end_year = result->year;

	// 遍历所有记录，统计匹配的小区按照年份和月份的记录数，总用电量和总电费
	for (i = 0; i < data->record_count; i++) {
		const ElectricBill* record = &data->records[i];

		if (strcmp(record->community, result->community) == 0 &&
			record->year == result->year &&
			record->month == result->month) {
			result->total_peak_kwh += record->peak_kwh;
			result->total_valley_kwh += record->valley_kwh;
			result->total_bill += record->bill;
			result->household_count++;
			result->household_bills_count++;
		}
	}
	// 计算户均用电量和户均电费
	if (result->household_count > 0) {
		result->average_kwh_household =
			(result->total_peak_kwh + result->total_valley_kwh) /
			result->household_count;

		result->average_bill_household =
			result->total_bill / result->household_count;
	}

	return result->household_bills_count;
}


// 统计所有小区每月的总用电量和总电费，返回“小区+年份+月份”分组数量。
size_t statistics_all_community_months(const SystemData* data,CommunityBill results[MAX_RECORDS])
{
	size_t i;
	size_t group_count = 0;
	size_t table_size;
	size_t* slots;
	// 参数检测
	if (data == NULL || results == NULL ||
		data->record_count > MAX_RECORDS ||
		data->record_count == 0) {
		return 0;
	}

	/* 统计组数量最多不会超过记录数，哈希表留出空位 */
	table_size = data->record_count * 2 + 1;  // 哈希表大小为记录数的两倍加一，保证有足够的空位
	slots = (size_t*)calloc(table_size, sizeof(size_t));   // 分配哈希表内存
	if (slots == NULL) return 0;         // 内存分配失败，返回 0

	// 遍历所有记录，将每条记录累加到对应的小区月份分组中
	for (i = 0; i < data->record_count; i++) {
		const ElectricBill* record = &data->records[i];
		size_t position = hash_community_month(record) % table_size;   // 计算哈希表位置
		size_t group_index;         // 哈希表对应位置存档数组result的索引

		/* 找到已有分组；哈希冲突时继续向后查找 */
		while (slots[position] != 0) {
			group_index = slots[position] - 1;    // 实际的数组要比哈希表索引小1
			//如果找到相同小区、年份和月份的分组，则跳出循环
			if (strcmp(results[group_index].community,
				record->community) == 0 &&
				results[group_index].year == record->year &&
				results[group_index].month == record->month) {
				break;
			}

			position = (position + 1) % table_size;   // 线性探测，继续查找下一个位置
		}

		/* 没有找到分组，就新建一个 */
		if (slots[position] == 0) {
			group_index = group_count++;       //已有分组数量加1，且索引为当前分组数量
			memset(&results[group_index], 0, sizeof(CommunityBill));  // 初始化新分组的内存为0

			memcpy(results[group_index].community, record->community, MODEL_NAME_LENGTH);   // 将小区名称复制到新分组中
			results[group_index].community[MODEL_NAME_LENGTH - 1] = '\0';         // 确保字符串以 '\0' 结尾

			results[group_index].year = record->year;
			results[group_index].month = record->month;
			results[group_index].period_start_year = record->year;
			results[group_index].period_end_year = record->year;

			slots[position] = group_index + 1;     //索引存入哈希表
		}
		else {
			group_index = slots[position] - 1;   
		}

		/* 将当前记录累加到对应小区月份 */
		results[group_index].total_peak_kwh += record->peak_kwh;
		results[group_index].total_valley_kwh += record->valley_kwh;
		results[group_index].total_bill += record->bill;
		results[group_index].household_count++;
		results[group_index].household_bills_count++;
	}

	/* 所有记录累加完成后统一计算户均值 */
	for (i = 0; i < group_count; i++) {
		if (results[i].household_count > 0) {
			results[i].average_kwh_household =
				(results[i].total_peak_kwh +
					results[i].total_valley_kwh) /
				results[i].household_count;

			results[i].average_bill_household =
				results[i].total_bill /
				results[i].household_count;
		}
	}

	free(slots);
	return group_count;
}






