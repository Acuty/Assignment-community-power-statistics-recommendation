#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include "data_generator.h"
#include "file_manager.h"
#include "web_server.h"

#pragma comment(lib, "Shell32.lib")

static size_t prepare_runtime_directory(void)
{
    char exe_path[MAX_PATH];
    char* slash;
    DWORD length;

    length = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if (length == 0 || length >= sizeof(exe_path)) {
        return 0;
    }

    slash = strrchr(exe_path, '\\');
    if (slash == NULL) {
        return 0;
    }

    *slash = '\0';
    return SetCurrentDirectoryA(exe_path) != 0;
}

static void open_frontend(void)
{
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        "http://127.0.0.1:8000",
        NULL,
        NULL,
        SW_SHOWNORMAL);

    if ((INT_PTR)result <= 32) {
        printf("无法自动打开浏览器，请手动访问 http://127.0.0.1:8000\\n");
    }
}

int main(void)
{
    static SystemData data;
    static const double recommendation_weights[EXPANSION_INDICATOR_COUNT] = {
        0.25, 0.25, 0.25, 0.25
    };
    size_t loaded_count;

    SetConsoleOutputCP(CP_UTF8);

    if (prepare_runtime_directory() == 0) {
        printf("无法定位程序运行目录。\\n");
        return 1;
    }

    data.rate.peak_price = 0.6;
    data.rate.valley_price = 0.3;

    loaded_count = file_load_records("electricity_demo.csv", &data);
    if (loaded_count == 0) {
        printf("读取 electricity_demo.csv 失败，或文件中没有有效记录。\\n");
        printf("请确认 electricity_demo.csv 与程序位于同一目录。\\n");
        return 1;
    }

    printf("成功读取 %zu 条记录。\\n", loaded_count);

    if (web_server_start(
        &data,
        "http://127.0.0.1:8000",
        "electricity_demo.csv",
        recommendation_weights) == 0) {
        printf("HTTP 服务启动失败，可能是 8000 端口已被占用。\\n");
        return 1;
    }

    printf("服务已启动：http://127.0.0.1:8000\\n");
    printf("浏览器正在打开，按 Ctrl+C 停止服务。\\n");
    open_frontend();

    for (;;) {
        web_server_poll();
    }

    return 0;
}
