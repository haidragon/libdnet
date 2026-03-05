@echo off
REM libdnet Windows 使用案例编译脚本

echo 正在编译 example.c ...
gcc -I../include -L../src/.libs -o example.exe example.c -ldnet -lws2_32 -liphlpapi

if %errorlevel% equ 0 (
    echo 编译成功！
    echo.
    echo 运行程序：
    example.exe
) else (
    echo 编译失败，请检查错误信息。
)

pause
