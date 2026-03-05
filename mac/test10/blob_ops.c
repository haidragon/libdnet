/**
 * libdnet Blob 操作示例 - test10
 */

#include <stdio.h>
#include <string.h>
#include <dnet.h>
#include <dnet/blob.h>

void demo_blob_create(void) {
    blob_t *b;
    printf("\n=== Blob 创建 ===\n");
    b = blob_new();
    if (b) {
        printf("Blob 创建成功\n");
        blob_free(b);
    }
}

void demo_blob_ops(void) {
    printf("\n=== Blob 操作 ===\n");
    printf("  blob_new()   - 创建 Blob\n");
    printf("  blob_free()  - 释放 Blob\n");
    printf("  blob_add()   - 添加数据\n");
    printf("  blob_read()  - 读取数据\n");
}

int main(void) {
    printf("libdnet Blob 操作示例\n");
    demo_blob_create();
    demo_blob_ops();
    printf("\n所有演示完成！\n");
    return 0;
}
