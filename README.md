# S.A.A.U.S.O
 
```shell
gn gen out/release
ninja -C out/release saauso
gn gen out/release --export-compile-commands
```

```shell
gn gen out/debug --args="is_debug=true"
ninja -C out/debug saauso
gn gen out/debug --export-compile-commands
```

```shell
gn gen out/asan --args="is_asan=true"
ninja -C out/asan saauso
gn gen out/asan --export-compile-commands
```

```shell
gn gen out/unittest --args="is_asan=true"
ninja -C out/asan saauso_unittests
gn gen out/unittest --export-compile-commands
```