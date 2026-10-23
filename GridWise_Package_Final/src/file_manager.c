#include <stdio.h>
#include "file_manager.h"
#include "data_manager.h"


// 从 CSV 文件中加载用电记录到系统数据中。
size_t file_load_records(const char* filename, SystemData* data)  
{
	FILE* file = NULL;  // 文件指针
	char line[256];
	ElectricBill record;  // 临时存储单条记录
	size_t field_count;  // 字段计数
	// 检查输入参数
	if (filename == NULL || data == NULL) {
		return 0;
	}
	data->record_count = 0; // 初始化记录计数
	// 打开 CSV 文件
	if (fopen_s(&file, filename, "r") != 0 || file == NULL) {
		return 0;
	}
	// 跳过表头
	if (fgets(line, sizeof(line), file) == NULL) {
		fclose(file); 
		return 0;
	}
	while (fgets(line, sizeof(line), file) != NULL) {
		record = (ElectricBill){ 0 };   // 初始化记录
		// 使用 sscanf_s 解析 CSV 行，确保字段数量正确
		field_count = (size_t)sscanf_s(
			line,
			"%63[^,],%63[^,],%zu,%zu,%lf,%lf,%lf",
			record.community, (unsigned)sizeof(record.community),
			record.household, (unsigned)sizeof(record.household),
			&record.year,
			&record.month,
			&record.peak_kwh,
			&record.valley_kwh,
			&record.bill
		);
		// 如果字段数量不正确或添加记录失败，则关闭文件并返回 0
		if (field_count != 7 || data_add_record(data, &record) == 0) {
			fclose(file);
			data->record_count = 0;
			return 0;
		}
	}
	// 关闭文件并检查是否成功
	if (fclose(file) != 0) {
		data->record_count = 0;
		return 0;
	}
	return data->record_count;
}

// 将 SystemData 中的记录保存到 CSV 文件
size_t file_save_records(const char* filename, const SystemData* data)
{
	FILE* file = NULL;  
	size_t i;
	// 检查输入参数
	if (filename == NULL || data == NULL ||
		data->record_count > MAX_RECORDS) {
		return 0;
	}
	// 打开 CSV 文件进行写入
	if (fopen_s(&file, filename, "w") != 0 || file == NULL) {
		return 0;
	}  
	// 写入表头，如果失败则关闭文件并返回 0
	if (fprintf(file,
		"community,household,year,month,peak_kwh,valley_kwh,bill\n") < 0) {
		fclose(file);
		return 0;
	}
	// 遍历记录数组，将每条记录写入 CSV 文件
	for (i = 0; i < data->record_count; i++) {
		const ElectricBill* record = &data->records[i];

		if (data_validate_record(record) == 0 ||
			fprintf(file, "%s,%s,%zu,%zu,%.2f,%.2f,%.2f\n",
				record->community,
				record->household,
				record->year,
				record->month,
				record->peak_kwh,
				record->valley_kwh,
				record->bill) < 0) {
			fclose(file);
			return 0;
		}
	}
	// 关闭文件并检查是否成功
	if (fclose(file) != 0) {
		return 0;
	}

	return data->record_count;
}
