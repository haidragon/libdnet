#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

int fw_callback(const struct fw_rule *rule, void *arg) {
    int *count = (int *)arg;
    char src[32], dst[32];
    
    (*count)++;
    printf("  规则 #%d:\n", *count);
    printf("    设备: %s\n", rule->fw_device);
    printf("    操作: %s\n", rule->fw_op == FW_OP_ALLOW ? "允许" : "阻止");
    printf("    方向: %s\n", rule->fw_dir == FW_DIR_IN ? "入站" : "出站");
    printf("    协议: %d\n", rule->fw_proto);
    
    addr_ntop(&rule->fw_src, src, sizeof(src));
    addr_ntop(&rule->fw_dst, dst, sizeof(dst));
    printf("    源地址: %s\n", src);
    printf("    目的地址: %s\n", dst);
    printf("    源端口: %d-%d\n", rule->fw_sport[0], rule->fw_sport[1]);
    printf("    目的端口: %d-%d\n\n", rule->fw_dport[0], rule->fw_dport[1]);
    
    return 0;
}

int main() {
    fw_t *f;
    struct fw_rule rule;
    int count = 0;
    char src[32];
    
    printf("=== libdnet 防火墙操作示例 ===\n");
    printf("注意: 此示例仅演示API使用，实际修改防火墙规则需要 root 权限\n\n");
    
    // 打开防火墙
    f = fw_open();
    if (f == NULL) {
        printf("注意: 无法打开防火墙（可能需要 root 权限）\n");
        printf("将演示如何构建防火墙规则结构\n\n");
    } else {
        printf("1. 列出现有防火墙规则:\n");
        printf("   ================================================\n\n");
        fw_loop(f, fw_callback, &count);
        printf("   共 %d 条规则\n\n", count);
        fw_close(f);
    }
    
    printf("2. 构建防火墙规则示例:\n");
    printf("   ================================================\n\n");
    
    // 示例规则1：允许所有入站 TCP 流量到端口 80
    struct addr zero_addr = {0};
    fw_pack_rule(&rule, "en0", FW_OP_ALLOW, FW_DIR_IN, IPPROTO_TCP, 
                 zero_addr, zero_addr, 0, 0, 80, 80);
    
    printf("  规则1: 允许入站 TCP 到端口 80\n");
    printf("    设备: %s\n", rule.fw_device);
    printf("    操作: %s\n", rule.fw_op == FW_OP_ALLOW ? "允许" : "阻止");
    printf("    方向: %s\n", rule.fw_dir == FW_DIR_IN ? "入站" : "出站");
    printf("    协议: TCP (%d)\n", rule.fw_proto);
    printf("    目的端口: %d\n\n", rule.fw_dport[0]);
    
    // 示例规则2：阻止特定IP的出站流量
    addr_pton("192.168.1.100", &rule.fw_src);
    fw_pack_rule(&rule, "en0", FW_OP_BLOCK, FW_DIR_OUT, 0, 
                 rule.fw_src, zero_addr, 0, 0, 0, 0);
    
    addr_ntop(&rule.fw_src, src, sizeof(src));
    printf("  规则2: 阻止来自 %s 的出站流量\n", src);
    printf("    操作: %s\n", rule.fw_op == FW_OP_BLOCK ? "阻止" : "允许");
    printf("    方向: %s\n\n", rule.fw_dir == FW_DIR_OUT ? "出站" : "入站");
    
    // 示例规则3：阻止特定范围的端口
    fw_pack_rule(&rule, "en0", FW_OP_BLOCK, FW_DIR_IN, 0, 
                 zero_addr, zero_addr, 0, 0, 22, 23);
    
    printf("  规则3: 阻止入站端口 22-23 的流量\n");
    printf("    目的端口: %d-%d\n\n", rule.fw_dport[0], rule.fw_dport[1]);
    
    printf("3. 提示:\n");
    printf("   - 实际添加/删除防火墙规则需要 root 权限\n");
    printf("   - 使用 fw_add() 添加规则\n");
    printf("   - 使用 fw_delete() 删除规则\n");
    printf("   - 不同的平台防火墙实现可能不同\n");
    
    printf("\n=== 防火墙操作示例完成 ===\n");
    return 0;
}
