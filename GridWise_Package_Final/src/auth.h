#ifndef AUTH_H
#define AUTH_H

#include <stddef.h>
#include "model.h"

/* 验证账号密码；成功返回 1，并将用户信息写入 user，失败返回 0 */
size_t auth_login(
    const char* account,
    const char* password,
    User* user
);

/* 检查用户是否具有指定权限；有权限返回 1，否则返回 0 */
size_t auth_has_role(
    const User* user,
    Role required_role
);

#endif
