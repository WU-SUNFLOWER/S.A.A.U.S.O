# S.A.A.U.S.O Embedder API 架构亮点总结

S.A.A.U.S.O Embedder API 采用 V8-like 的句柄隔离模型，以 `Isolate + HandleScope + EscapableHandleScope` 管理对象生命周期，确保宿主侧包装对象可控回收并避免跨作用域泄漏；在原生函数调度上引入 closure_data 路由，将早期固定槽位分发升级为可扩展绑定机制，实现更稳定的回调注册与多实例并行；在错误体系上统一 `MaybeLocal + TryCatch + CapturePendingException`，形成“失败可观测、异常可穿透、调用可兜底”的防御性编程链路，支撑 C++↔Python 双向互调在实战场景中的可维护性与工程可靠性。
