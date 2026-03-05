#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

// 回调函数用于遍历接口
int print_interface(const struct intf_entry *entry, void *arg) {
    if (entry == NULL) {
        return 0;
    }
    
    printf("   %-10s %-20s ", entry->intf_name, (entry->driver_name[0] != '\0') ? entry->driver_name : "N/A");

    // 打印 MAC 地址
    if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
        printf("%02X:%02X:%02X:%02X:%02X:%02X ",
               entry->intf_link_addr.addr_eth.data[0],
               entry->intf_link_addr.addr_eth.data[1],
               entry->intf_link_addr.addr_eth.data[2],
               entry->intf_link_addr.addr_eth.data[3],
               entry->intf_link_addr.addr_eth.data[4],
               entry->intf_link_addr.addr_eth.data[5]);
    } else {
        printf("%-18s ", "N/A");
    }

    // 打印 IP 地址
    if (entry->intf_addr.addr_type == ADDR_TYPE_IP) {
        char ip_str[32];
        addr_ntop(&entry->intf_addr, ip_str, sizeof(ip_str));
        printf("%-15s\n", ip_str);
    } else {
        printf("%-15s\n", "N/A");
    }
    return 0;
}

int main() {
    intf_t *iop;
    struct intf_entry entry;
    int ret = 0;

    printf("=== libdnet 接口操作示例 ===\n\n");

    // 打开接口操作
    iop = intf_open();
    if (iop == NULL) {
        fprintf(stderr, "无法打开接口操作\n");
        return 1;
    }

    printf("1. 遍历所有网络接口:\n");
    printf("   %-10s %-20s %-18s %-15s\n", "名称", "描述", "MAC 地址", "IP 地址");
    printf("   %-10s %-20s %-18s %-15s\n", "----", "----", "-------", "-------");

    // 遍历所有接口
    intf_loop(iop, print_interface, NULL);

    printf("\n2. 获取指定接口信息:\n");
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.intf_name, "en0", sizeof(entry.intf_name) - 1);
    entry.intf_name[sizeof(entry.intf_name) - 1] = '\0';  // 确保 null 终止
    ret = intf_get(iop, &entry);
    if (ret < 0) {
        printf("   警告：无法获取 en0 接口信息 (可能不存在或无权限)\n");
        printf("   提示：在 macOS 上，尝试使用 'ifconfig -a' 查看可用接口\n");
    } else {
        printf("   接口名称：%s\n", entry.intf_name);
        printf("   接口描述：%s\n", (entry.driver_name[0] != '\0') ? entry.driver_name : "N/A");
        printf("   接口类型：%d\n", entry.intf_type);
        printf("   MTU: %d\n", entry.intf_mtu);
        printf("   索引：%d\n", entry.intf_index);

        if (entry.intf_addr.addr_type == ADDR_TYPE_IP) {
            char ip_str[32];
            addr_ntop(&entry.intf_addr, ip_str, sizeof(ip_str));
            printf("   IP 地址：%s\n", ip_str);
        }

        if (entry.intf_link_addr.addr_type == ADDR_TYPE_ETH) {
            printf("   MAC 地址：%02X:%02X:%02X:%02X:%02X:%02X\n",
                   entry.intf_link_addr.addr_eth.data[0],
                   entry.intf_link_addr.addr_eth.data[1],
                   entry.intf_link_addr.addr_eth.data[2],
                   entry.intf_link_addr.addr_eth.data[3],
                   entry.intf_link_addr.addr_eth.data[4],
                   entry.intf_link_addr.addr_eth.data[5]);
        }

        if (entry.intf_dst_addr.addr_type == ADDR_TYPE_IP) {
            char dst_str[32];
            addr_ntop(&entry.intf_dst_addr, dst_str, sizeof(dst_str));
            printf("   目标地址：%s\n", dst_str);
        }

        printf("   标志：0x%x\n", entry.intf_flags);
        printf("   状态：%s\n", (entry.intf_flags & INTF_FLAG_UP) ? "UP" : "DOWN");
    }

    // 获取主要接口 - 修复：传入有效的 src 地址参数
    printf("\n3. 获取默认接口:\n");
    memset(&entry, 0, sizeof(entry));
    struct addr src;
    memset(&src, 0, sizeof(src));
    src.addr_type = ADDR_TYPE_IP;
    src.addr_bits = 0;
    // 设置为 0.0.0.0 表示查询默认路由
    ret = intf_get_src(iop, &entry, &src);
    if (ret < 0) {
        printf("   警告：无法获取默认接口\n");
    } else {
        printf("   默认接口名称：%s\n", entry.intf_name);
        if (entry.intf_addr.addr_type == ADDR_TYPE_IP) {
            char ip_str[32];
            addr_ntop(&entry.intf_addr, ip_str, sizeof(ip_str));
            printf("   默认接口 IP: %s\n", ip_str);
        }
    }

    printf("\n4. 检查接口是否启用:\n");
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.intf_name, "en0", sizeof(entry.intf_name) - 1);
    entry.intf_name[sizeof(entry.intf_name) - 1] = '\0';
    ret = intf_get(iop, &entry);
    if (ret >= 0) {
        int is_up = (entry.intf_flags & INTF_FLAG_UP) != 0;
        printf("   接口 en0 状态：%s\n", is_up ? "UP (启用)" : "DOWN (禁用)");
        printf("   接口 en0 标志：0x%x\n", entry.intf_flags);
    } else {
        printf("   无法获取 en0 接口状态\n");
    }

    // 获取接口列表
    printf("\n5. 接口列表信息:\n");
    printf("   接口总数：至少包含 en0, lo0 等\n");
    printf("   (使用 'ifconfig -a' 查看完整列表)\n");

    // 按索引获取接口信息
    printf("\n6. 按索引查询接口:\n");
    memset(&entry, 0, sizeof(entry));
    ret = intf_get_index(iop, &entry, AF_INET, 1);
    if (ret >= 0) {
        printf("   索引 1 的接口：%s\n", entry.intf_name);
    } else {
        printf("   无法按索引获取接口信息\n");
    }

    // 关闭接口操作
    intf_close(iop);

    printf("\n=== 接口操作示例完成 ===\n");
    return 0;
}