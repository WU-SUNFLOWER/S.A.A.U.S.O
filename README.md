# S.A.A.U.S.O
 
## Build

```shell
gn gen out/release
ninja -C out/release vm
gn gen out/release --export-compile-commands
```

```shell
gn gen out/debug --args="is_debug=true"
ninja -C out/debug vm
gn gen out/debug --export-compile-commands
```

```shell
gn gen out/asan --args="is_asan=true"
ninja -C out/asan vm
gn gen out/asan --export-compile-commands
```

## Test (ASan)

```shell
gn gen out/ut --args="is_asan=true"
ninja -C out/ut all_ut
gn gen out/ut --export-compile-commands
```

```powershell
$env:ASAN_SYMBOLIZER_PATH = "D:\LLVM\bin\llvm-symbolizer.exe"
$env:ASAN_OPTIONS = "symbolize=1"
.\out\ut\ut.exe
.\out\ut\embedder_ut.exe
```

## Docs

- [AI 贡献指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-AI-Developer-Contributing-Guide.md)
- [代码风格指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md)
- [构建与测试指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)
- [VM 系统架构总览](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)
