#include <string.h>
#include "query.h"

/* 判断记录是否符合筛选条件
 * 空字符串和 0 表示不筛选对应字段
 */
static size_t record_matches(
    const ElectricBill* record,
    const ElectricBill* filter)
{

    if (filter->community[0] != '\0' &&
        strcmp(record->community, filter->community) != 0) {
        return 0;
    }

    if (filter->household[0] != '\0' &&
        strcmp(record->household, filter->household) != 0) {
        return 0;
    }
	// 检查年份是否匹配
    if (filter->year != 0 && record->year != filter->year) {
        return 0;
    }
	// 检查月份是否匹配
    if (filter->month != 0 && record->month != filter->month) {
        return 0;
    }

    return 1;
}

/* 计算符合筛选条件的记录总数 */
size_t query_count(
    const SystemData* data,
    const ElectricBill* filter)
{
    size_t i;
    size_t record_count;
    size_t matched_count = 0;
	// 参数检查
    if (data == NULL || filter == NULL || filter->month > 12) {
        return 0;
    }
	// 获取记录数，同时防止越界
    record_count = data->record_count;
    if (record_count > MAX_RECORDS) {
        record_count = MAX_RECORDS;
    }
	// 遍历所有记录，统计匹配的记录数
    for (i = 0; i < record_count; i++) {
        if (record_matches(&data->records[i], filter) != 0) {
            matched_count++;
        }
    }

    return matched_count;
}

// 查询符合条件的记录，返回实际写入 results
size_t query_page(
    const SystemData* data,
    const ElectricBill* filter,
    size_t page,
    size_t page_size,
    ElectricBill results[],
    size_t result_capacity)
{
    size_t i;
	size_t record_count;       // 总记录数
	size_t matched_index = 0;  // 当前匹配的记录索引
	size_t copied_count = 0;  // 已复制到结果数组的记录数
	size_t start_index;  // 当前页的起始索引
	// 参数检查
    if (data == NULL || filter == NULL || results == NULL ||
        page == 0 || page_size == 0 ||
        result_capacity < page_size || filter->month > 12) {
        return 0;
    }

    /* 检查页码计算是否会溢出 */
    if (page - 1 > (size_t)-1 / page_size) {
        return 0;
    }

	start_index = (page - 1) * page_size;// 计算当前页的起始索引

    record_count = data->record_count;
	// 防止越界
    if (record_count > MAX_RECORDS) {
        record_count = MAX_RECORDS;
    }

	// 遍历所有记录，找到匹配的记录并复制到结果数组中

    for (i = 0; i < record_count; i++) {
		const ElectricBill* record = &data->records[i];  // 遍历所有用电记录
        if (record_matches(record, filter) == 0) {
            continue;
        }

		// 只有当匹配的记录索引大于等于当前页的起始索引时才复制到结果数组中
        if (matched_index >= start_index) {
            results[copied_count] = *record; 
            copied_count++;

            if (copied_count == page_size) {
                break;
            }
        }
        
        matched_index++;  //计入结果的记录数据
    }

    return copied_count;
}
