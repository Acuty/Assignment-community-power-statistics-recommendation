#include "billing.h"

// 根据峰谷用电量和电价计算电费
double billing_calculate(double peak_kwh, double valley_kwh, Price price)
{
	// 检查输入参数是否为负数
    if (peak_kwh < 0 ||
        valley_kwh < 0 ||
        price.peak_price < 0 ||
        price.valley_price < 0) {
        return 0.0;
    }

    return peak_kwh * price.peak_price
        + valley_kwh * price.valley_price;
}