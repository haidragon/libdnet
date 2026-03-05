/**
 * libdnet 随机数生成示例 - test11
 */

#include <stdio.h>
#include <dnet.h>
#include <dnet/rand.h>

void demo_random_bytes(void) {
    rand_t *r;
    u_char buf[16];

    r = rand_open();
    if (r) {
        rand_get(r, buf, sizeof(buf));

        printf("\n=== 随机字节 ===\n");
        printf("生成的 16 字节随机数:\n");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");

        rand_close(r);
    }
}

void demo_random_uint32(void) {
    rand_t *r;
    uint32_t r1, r2;

    r = rand_open();
    if (r) {
        r1 = rand_uint32(r);
        r2 = rand_uint32(r);

        printf("\n=== 随机 uint32_t ===\n");
        printf("r1: %u (0x%08x)\n", r1, r1);
        printf("r2: %u (0x%08x)\n", r2, r2);

        rand_close(r);
    }
}

void demo_random_range(void) {
    rand_t *r;
    uint32_t val;

    r = rand_open();
    if (r) {
        val = rand_uint32(r);
        val = (val % 100) + 1;  /* 转换到 1-100 范围 */

        printf("\n=== 随机范围 ===\n");
        printf("1-100 之间的随机数: %u\n", val);

        rand_close(r);
    }
}

void demo_random_ops(void) {
    printf("\n=== 随机数操作 ===\n");
    printf("  rand_open()   - 打开随机数生成器\n");
    printf("  rand_get()    - 生成随机字节\n");
    printf("  rand_uint32() - 生成 32 位随机数\n");
    printf("  rand_close()  - 关闭随机数生成器\n");
}

int main(void) {
    printf("libdnet 随机数生成示例\n");
    demo_random_bytes();
    demo_random_uint32();
    demo_random_range();
    demo_random_ops();
    printf("\n所有演示完成！\n");
    return 0;
}
