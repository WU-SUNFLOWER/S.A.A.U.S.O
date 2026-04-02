#!/bin/bash

# 设置报错即停止
set -e

# 检测操作系统，如果是 Windows (MINGW/MSYS)，则使用 .bat 后缀
case "$(uname -s)" in
    MSYS*|MINGW*|CYGWIN*)
        GN_EXE="gn.exe"
        NINJA_EXE="ninja.exe"
        ;;
    *)
        GN_EXE="gn"
        NINJA_EXE="ninja"
        ;;
esac

# 定义帮助信息
usage() {
    echo "使用方法: $0 [release | debug | asan | ut | ut_backend]"
    echo "------------------------------------------------"
    echo "  release  : 默认发布版本"
    echo "  debug    : 调试版本 (is_debug=true)"
    echo "  asan     : 内存检测版本 (is_asan=true)"
    echo "  ut       : 单元测试版本 (is_asan=true, 编译 ut)"
    echo "  ut_backend: 纯后端单元测试版本 (is_asan=true, saauso_enable_cpython_compiler=false, 编译 ut)"
    exit 1
}

# 检查输入参数
if [ -z "$1" ]; then
    usage
fi

MODE=$1
OUT_DIR=""
GN_ARGS=""
TARGET="vm"

case $MODE in
  release)
        OUT_DIR="out/release"
        GN_ARGS=""
        ;;
    debug)
        OUT_DIR="out/debug"
        GN_ARGS="is_debug=true"
        ;;
    asan)
        OUT_DIR="out/asan"
        GN_ARGS="is_asan=true"
        ;;
    ut)
        OUT_DIR="out/ut"
        GN_ARGS="is_asan=true"
        TARGET="all_ut"
        ;;
    ut_backend)
        OUT_DIR="out/ut_backend"
        GN_ARGS="is_asan=true saauso_enable_cpython_compiler=false"
        TARGET="all_ut"
        ;;
    *)
        echo "错误: 未知的模式 '$MODE'"
        usage
        ;;
esac

echo "==== 正在构建模式: $MODE ===="
echo "----------------------------"

# 1. 生成配置
if [ -z "$GN_ARGS" ]; then
    $GN_EXE gen "$OUT_DIR"
else
    $GN_EXE gen "$OUT_DIR" --args="$GN_ARGS"
fi

# 2. 依赖与可见性检查
$GN_EXE check "$OUT_DIR"

# 3. 执行编译

# 清理ninja缓存，会导致所有.cc文件重新编译
# $NINJA_EXE -C "$OUT_DIR" -t clean

$NINJA_EXE -C "$OUT_DIR" "$TARGET"

# 4. 导出编译命令 (用于 IDE 补全)
$GN_EXE gen "$OUT_DIR" --export-compile-commands

echo "----------------------------"
echo "🎉🎉🎉构建完成！"
