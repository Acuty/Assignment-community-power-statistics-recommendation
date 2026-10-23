#include <string.h>
#include "auth.h"

/* 演示账号：提交前请修改密码 */
static const User demo_users[] = {
    { "admin", "admin123", ROLE_ADMIN },
    { "user",  "user123",  ROLE_USER }
};      

/* 验证账号密码，并返回对应用户信息 */
size_t auth_login(
    const char* account,
    const char* password,
    User* user)
{
    size_t i;
    size_t user_count = sizeof(demo_users) / sizeof(demo_users[0]);       //确定账户数量
	//参数检查
    if (account == NULL || password == NULL || user == NULL) {
        return 0;
    }

    for (i = 0; i < user_count; i++) {
        if (strcmp(account, demo_users[i].account) == 0 &&
            strcmp(password, demo_users[i].password) == 0) {
			*user = demo_users[i];    // 将匹配的用户信息写入 user
            return 1;
        }
    }

    return 0;
}

/* 管理员拥有普通用户权限；普通用户不能执行管理员操作 */
size_t auth_has_role(
    const User* user,
    Role required_role)
{
    if (user == NULL) {
        return 0;
    }
	// 管理员拥有普通用户权限
    if (required_role == ROLE_USER) {
		return user->role == ROLE_USER || user->role == ROLE_ADMIN;    //返回判断权限，如果是普通用户或者管理员都返回1
    }
	// 只有管理员才能执行管理员操作
    if (required_role == ROLE_ADMIN) {
		return user->role == ROLE_ADMIN;                                 //返回判断权限，如果是管理员返回1，否则返回0
    }

    return 0;
}