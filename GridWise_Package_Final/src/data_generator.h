#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include <stddef.h>
#include "model.h"

// 生成模拟用电数据并写入 CSV 文件。
// 参数：文件名、小区名称、户数、起始年份、电价。
// 返回：成功写入的记录数；参数错误或文件操作失败时返回 0。
size_t generate_csv(
    const char* filename,
    const char communities[][MODEL_NAME_LENGTH],
    size_t community_count,
    size_t household_count,
    size_t start_year,
    Price price
);

#endif /* DATA_GENERATOR_H */
