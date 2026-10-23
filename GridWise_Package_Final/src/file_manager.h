#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "model.h"

size_t file_load_records(const char* filename, SystemData* data);
size_t file_save_records(const char* filename, const SystemData* data);
#endif