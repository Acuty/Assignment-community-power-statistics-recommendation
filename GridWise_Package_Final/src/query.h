#ifndef QUERY_H
#define QUERY_H

#include "model.h"
// 查询符合条件的记录数量
size_t query_count(
    const SystemData* data,
    const ElectricBill* filter
);
// 查询符合条件的记录，返回实际写入 results 的数量
size_t query_page(
    const SystemData* data,
    const ElectricBill* filter,
    size_t page,
    size_t page_size,
    ElectricBill results[],
    size_t result_capacity
);

#endif