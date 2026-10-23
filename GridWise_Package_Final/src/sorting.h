#ifndef SORTING_H
#define SORTING_H

#include "model.h"

/* 按小区峰时总电量从高到低排序 */
size_t sort_by_peak_kwh(CommunityBill bills[], size_t count);

/* 按小区谷时总电量从高到低排序 */
size_t sort_by_valley_kwh(CommunityBill bills[], size_t count);

/* 按小区峰谷总电量从高到低排序 */
size_t sort_by_total_kwh(CommunityBill bills[], size_t count);

/* 按小区总电费从高到低排序 */
size_t sort_by_total_bill(CommunityBill bills[], size_t count);

/* 按户均电量从高到低排序 */
size_t sort_by_average_kwh(CommunityBill bills[], size_t count);

/* 按户均电费从高到低排序 */
size_t sort_by_average_bill(CommunityBill bills[], size_t count);

#endif#pragma once
