#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

int route_callback(const struct route_entry *entry, void *arg) {
    int *count = (int *)arg;
    char dst[32], gw[32];
    
    (*count)++;
    printf("  路由 #%d:\n", *count);
    
    addr_ntop(&entry->route_dst, dst, sizeof(dst));
    addr_ntop(&entry->route_gw, gw, sizeof(gw));
    
    printf("    目的地址: %s\n", dst);
    printf("    网关地址: %s\n", gw);
    printf("    接口名称: %s\n", entry->intf_name);
    printf("    跃点数: %d\n\n", entry->metric);
    
    return 0;
}

int main() {
    route_t *r;
    int count = 0;
    
    printf("=== libdnet 网关查询示例 ===\n\n");
    
    // 打开路由表
    r = route_open();
    if (r == NULL) {
        fprintf(stderr, "错误: 无法打开路由表\n");
        return 1;
    }
    
    printf("1. 遍历系统路由表:\n");
    printf("   ================================================\n\n");
    
    // 遍历路由表
    route_loop(r, route_callback, &count);
    
    printf("2. 查询默认网关:\n");
    printf("   ================================================\n");
    
    // 查询默认路由（0.0.0.0）
    struct route_entry entry;
    memset(&entry, 0, sizeof(entry));
    addr_pton("0.0.0.0", &entry.route_dst);
    
    if (route_get(r, &entry) == 0) {
        char gw[32];
        addr_ntop(&entry.route_gw, gw, sizeof(gw));
        printf("   默认网关: %s\n", gw);
        printf("   接口: %s\n", entry.intf_name);
        printf("   跃点数: %d\n", entry.metric);
    } else {
        printf("   未找到默认网关\n");
    }
    
    route_close(r);
    
    printf("\n=== 网关查询示例完成 ===\n");
    return 0;
}
