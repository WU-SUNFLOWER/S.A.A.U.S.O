# Embedder Demo 运行说明

## 一行命令（前端编译开启，推荐答辩演示）

```powershell
.\depot_tools\gn.exe gen out\embedder_front; .\depot_tools\ninja.exe -C out\embedder_front embedder_game_engine_demo -j 4; .\out\embedder_front\embedder_game_engine_demo.exe
```

## 一行命令（后端模式，仅验证可执行入口）

```powershell
.\depot_tools\gn.exe gen out\embedder_backend --args="saauso_enable_cpython_compiler=false"; .\depot_tools\ninja.exe -C out\embedder_backend embedder_game_engine_demo -j 4; .\out\embedder_backend\embedder_game_engine_demo.exe
```

## 预期输出

1. 前端模式：输出 `=== Game Loop Start ===`，随后打印每帧 `hp/status` 变化并结束。
2. 后端模式：输出 `CPython frontend compiler is disabled; game_engine_demo only runs in frontend build.`。
