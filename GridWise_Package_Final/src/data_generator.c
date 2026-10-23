#include <stdio.h>
#include <stdlib.h>
#include "data_generator.h"

/* 生成多个小区每户连续两年的月用电数据，并写入 CSV 文件 */
size_t generate_csv(
    const char* filename,
    const char communities[][MODEL_NAME_LENGTH],
    size_t community_count,
    size_t household_count,
    size_t start_year,
    Price price)
{
    FILE* file = NULL;
    size_t community_index;
    size_t household_index;
    size_t month_index;
    size_t total_kwh;
    size_t peak_percent;
    double peak_kwh;
    double valley_kwh;
    double bill;

    /* 检查参数及总记录数，避免超过数组容量 */
    if (filename == NULL || communities == NULL ||
        community_count == 0 ||
        household_count < 50 ||
        community_count > MAX_RECORDS / 24 ||
        household_count > MAX_RECORDS / 24 / community_count) {
        return 0;
    }

    /* 检查小区名称不能为空 */
    for (community_index = 0;
        community_index < community_count;
        community_index++) {
        if (communities[community_index][0] == '\0') {
            return 0;
        }
    }

    if (fopen_s(&file, filename, "w") != 0 || file == NULL) {
        return 0;
    }

    /* CSV 表头只写一次 */
    if (fprintf(file,
        "community,household,year,month,peak_kwh,valley_kwh,bill\n") < 0) {
        fclose(file);
        return 0;
    }

    for (community_index = 0;
        community_index < community_count;
        community_index++) {

        for (household_index = 0;
            household_index < household_count;
            household_index++) {

            for (month_index = 0; month_index < 24; month_index++) {
                total_kwh = 100 + rand() % 301;       /* 总电量：100～400 */
                peak_percent = 40 + rand() % 31;      /* 峰时占比：40%～70% */

                peak_kwh = total_kwh * peak_percent / 100.0;
                valley_kwh = total_kwh - peak_kwh;
                bill = peak_kwh * price.peak_price
                    + valley_kwh * price.valley_price;

                if (fprintf(file,
                    "%s,H%03zu,%zu,%zu,%.2f,%.2f,%.2f\n",
                    communities[community_index],
                    household_index + 1,
                    start_year + month_index / 12,
                    month_index % 12 + 1,
                    peak_kwh,
                    valley_kwh,
                    bill) < 0) {
                    fclose(file);
                    return 0;
                }
            }
        }
    }

    if (fclose(file) != 0) {
        return 0;
    }

    return community_count * household_count * 24;
}
