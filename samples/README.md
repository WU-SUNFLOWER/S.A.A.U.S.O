# Embedder Demo 运行说明

## 一行命令（前端编译开启，推荐答辩演示）

### hello_world

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front embedder_hello_world; .\out\embedder_front\embedder_hello_world.exe
```

### embedder_game_engine_demo

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front embedder_game_engine_demo; .\out\embedder_front\embedder_game_engine_demo.exe
```

## 一行命令（后端模式，仅验证可执行入口）

```powershell
.\depot_tools\gn.exe gen out\embedder_backend --args="saauso_enable_cpython_compiler=false"; .\depot_tools\ninja.exe -C out\embedder_backend embedder_game_engine_demo; .\out\embedder_backend\embedder_game_engine_demo.exe
```

## 第 5 章性能样例

以下样例用于配合论文第 5 章中的补充性性能测试。建议在开启前端编译能力的配置下运行。

### bubble_sort

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front bubble_sort; .\out\embedder_front\bubble_sort.exe
```

### fibonacci_benchmark

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front fibonacci_benchmark; .\out\embedder_front\fibonacci_benchmark.exe
```

### object_create_benchmark

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front object_create_benchmark; .\out\embedder_front\object_create_benchmark.exe
```

### object_method_call_benchmark

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front object_method_call_benchmark; .\out\embedder_front\object_method_call_benchmark.exe
```
