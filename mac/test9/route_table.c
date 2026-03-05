/**
 * libdnet 路由表操作示例 - test9
 */

#include <stdio.h>
#include <dnet.h>
#include <dnet/route.h>

void demo_route_types(void) {
    printf("\n=== 路由类型 ===\n");
    printf("  ROUTE_TYPE_GW  - 网关路由\n");
    printf("  ROUTE_TYPE_MASK - 网络路由\n");
}

void demo_route_ops(void) {
    printf("\n=== 路由操作 ===\n");
    printf("  route_open()  - 打开路由表\n");
    printf("  route_get()   - 获取路由条目\n");
    printf("  route_loop()  - 遍历路由表\n");
    printf("  route_close() - 关闭路由表\n");
}

int main(void) {
    printf("libdnet 路由表操作示例\n");
    demo_route_types();
    demo_route_ops();
    printf("\n所有演示完成！\n");
    return 0;
}
