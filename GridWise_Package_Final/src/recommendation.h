#pragma once

#include <stddef.h>
#include "model.h"

/*
 * 根据系统记录和扩容需求生成推荐结果。
 * 只采用最近两个年份均有完整 12 个月记录的小区，结果按优先分降序排列。
 * 四项指标权重由 demand->indicator_weights 提供，函数按权重总和归一化。
 * 返回写入 results 的 CommunityBill 数量；参数错误或无候选小区时返回 0。
 * results 的容量由 result_capacity 指定。
 */
size_t recommend_communities(
    const SystemData* data,
    const ExpansionDemand* demand,
    CommunityBill results[],
    size_t result_capacity
);
