# S.A.A.U.S.O
 
```shell
gn gen out/release
ninja -C out/release --verbose
gn gen out/release --export-compile-commands
```

```shell
gn gen out/debug --args="is_debug=true"
ninja -C out/debug --verbose
gn gen out/debug --export-compile-commands
```

```shell
gn gen out/asan --args="is_asan=true"
ninja -C out/asan --verbose
gn gen out/asan --export-compile-commands
```
