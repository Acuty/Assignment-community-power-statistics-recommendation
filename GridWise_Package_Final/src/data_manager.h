#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdbool.h>
#include "model.h"

size_t data_validate_record(const ElectricBill* record);    // 验证单条记录的有效性，返回 1 表示有效，0 表示无效。
size_t data_add_record(SystemData* data, const ElectricBill* record);   // 添加单条记录；成功返回 1，失败返回 0。
size_t data_update_record(SystemData* data, size_t index, const ElectricBill* record);  // 更新指定索引的记录，返回 1 表示成功，0 表示失败（索引无效或记录无效）。
size_t data_delete_record(SystemData* data, size_t index);       // 删除指定索引的记录，返回 1 表示成功，0 表示失败（索引无效）。
#endif
