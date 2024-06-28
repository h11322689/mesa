#!/bin/bash

build_debug() {
    start_time=$(date +%s)                          # Record start time

    if [ -e build_debug ]; then
        echo "$pwd/build_debug exists"
    else
        mkdir build_debug
    fi

    cmake ../ -S llvm -B build_debug -G Ninja    \
        -DLLVM_PARALLEL_COMPILE_JOBS=16          \
        -DLLVM_PARALLEL_LINK_JOBS=16             \
        -DCMAKE_INSTALL_PREFIX="/home/lih/work/llvm-project/output_debug"             \
        -DCMAKE_BUILD_TYPE=Debug                \
        -DLLVM_ENABLE_PROJECTS="clang;lldb;compiler-rt;mlir;openmp;polly;lld;bolt;clang-tools-extra"   \
        -DLLVM_ENABLE_RUNTIMES=all              \
        -DLLVM_BUILD_EXAMPLES=ON                \
        -DLLVM_BUILD_TESTS=ON                   \
        -DLLVM_BUILD_BENCHMARKS=ON              \
        -DLLVM_CCACHE_BUILD=ON                  \
        -DLLVM_INSTALL_UTILS=ON                 \
        -DLLVM_INCLUDE_TESTS=ON                 \
	-DLLVM_TARGETS_TO_BUILD="X86"           \
        -DCLANG_BUILD_EXAMPLES=ON               \
	-DBUILD_SHARED_LIBS=ON		
        
    cd build_debug 
    ninja install -j10

    end_time=$(date +%s)                            # Record end time
    duration=$((end_time - start_time))             # Calculate duration in seconds

    echo "Build duration: $duration seconds"        # Print compilation duration
}

build_debug

# -S llvm                 源代码目录
# -B build_llvm           构建目录
# -G Ninja                生成器类型

# -DLLVM_ENABLE_RUNTIMES  一起构建的运行时库
# -DLLVM_INSTALL_PREFIX   指定安装目录   
# -DLLVM_BUILD_TYPE       指定构件类型
# -DLLVM_BUILD_EXAMPLES   是否构建LLVM的示例程序
# -DLLVM_INSTALL_UTILS    是否同时安装LLVM的一些实用工具
# -DLLVM_INCLUDE_TESTS   
# -DLLVM_BUILD_TESTS      是否构建测试用例
# -DLLVM_BUILD_BENCHMARKS 是否构建性能测试

# -DLLVM_ENABLE_PROJECTS  同时构建的LLVM子项目
# -DCLANG_BUILD_EXAMPLES  是否构建clang的示例代码
# -DCLANG_INCLUDE_TESTS   是否构建clang时包含测试代码 
# -DBUILD_SHARED_LIBS     是否使用动态链接LLVM库
