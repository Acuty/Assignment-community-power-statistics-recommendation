#include "data_manager.h"
#include <math.h>
#include <string.h>

/* 检查是否已有相同小区、住户和年月的记录。 */
static size_t record_key_exists(
    const SystemData* data,
    const ElectricBill* record,
    size_t skip_index)
{
    size_t i;

    for (i = 0; i < data->record_count; i++) {
        const ElectricBill* current = &data->records[i];
        if (i != skip_index &&
            strcmp(current->community, record->community) == 0 &&
            strcmp(current->household, record->household) == 0 &&
            current->year == record->year &&
            current->month == record->month) {
            return 1;
        }
    }
    return 0;
}

size_t data_validate_record(const ElectricBill* record)    // 验证单条记录的有效性，返回 1 表示有效，0 表示无效。
{
	if (record == NULL) return 0;  // 检查指针是否为 NULL
	// 检查记录的各个字段是否符合要求
    if (record->community[0] == '\0' ||
        record->household[0] == '\0' ||
        memchr(record->community, '\0', sizeof(record->community)) == NULL ||
        memchr(record->household, '\0', sizeof(record->household)) == NULL ||
        record->year == 0 ||
        record->month < 1 ||
        record->month > 12 ||
        !isfinite(record->peak_kwh) ||
        !isfinite(record->valley_kwh) ||
        !isfinite(record->bill) ||
        record->peak_kwh < 0 ||
        record->valley_kwh < 0 ||
        record->bill < 0) {
        return 0;
    }
    return 1;
}
// 添加单条记录；成功返回 1，数据已满、重复或记录无效时返回 0。
size_t data_add_record(SystemData* data, const ElectricBill* record)
{
	if (data == NULL || data->record_count >= MAX_RECORDS ||
        data_validate_record(record) == 0 ||
        record_key_exists(data, record, (size_t)-1) != 0) return 0;
	data->records[data->record_count] = *record;   // 将新记录添加到数组末尾
	data->record_count++;  // 增加记录计数
    return 1;
}
//更新指定索引的记录，返回 1 表示成功，0 表示失败（索引无效或记录无效）。
size_t data_update_record(SystemData* data,size_t index, const ElectricBill* record)
{
    if (data == NULL || data->record_count > MAX_RECORDS ||
        index >= data->record_count || data_validate_record(record) == 0 ||
        record_key_exists(data, record, index) != 0) {
        return 0;
    }

	data->records[index] = *record;  // 更新指定索引的记录
    return 1;
}
//删除指定索引的记录，返回 1 表示成功，0 表示失败（索引无效）。
size_t data_delete_record(SystemData* data, size_t index)
{
    size_t i;

    if (data == NULL || index >= data->record_count) {
        return 0;
    }

	for (i = index; i + 1 < data->record_count; i++)   // 将后续记录前移覆盖要删除的记录
    {
        data->records[i] = data->records[i + 1];
    }

    data->record_count--;
    return 1;
}
