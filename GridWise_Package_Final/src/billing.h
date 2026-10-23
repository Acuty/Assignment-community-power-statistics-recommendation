#ifndef BILLING_H
#define BILLING_H

#include "model.h"

// 根据峰谷用电量和电价计算电费
// 参数：峰时电量、谷时电量、电价
// 返回：电费；参数为负数时返回 0.0

//注意我会在增改的时候调用一次
double billing_calculate(double peak_kwh,
    double valley_kwh,
    Price price);

#endif