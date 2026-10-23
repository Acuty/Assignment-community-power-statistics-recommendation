#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stddef.h>
#include "model.h"

/* 启动 HTTP 服务；成功返回 1，失败返回 0 */
size_t web_server_start(
    SystemData* data,
    const char* address,
    const char* csv_filename,
    const double indicator_weights[EXPANSION_INDICATOR_COUNT]
);

/* 处理一次 HTTP 事件 */
void web_server_poll(void);

/* 停止服务 */
void web_server_stop(void);

#endif