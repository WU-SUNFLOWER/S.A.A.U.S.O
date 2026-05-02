import gc
import time

def create_cyclic_objects(n):
    """创建 n 个互相引用的 Node 对象，形成一个大的循环链表"""
    class Node:
        def __init__(self):
            self.ref = None

    # 创建 n 个 Node
    nodes = []
    i = 0
    while i < n:
        nodes.append(Node())
        i += 1

    # 构建循环：每个节点引用下一个，最后一个引用第一个
    i = 0
    while i < n:
        nodes[i].ref = nodes[(i + 1) % n]
        i += 1

    return nodes

def main(object_count):
    # 为确保准确性，先禁用自动 GC
    gc.disable()

    # 先做一次全量 GC，清理当前环境（避免残留对象影响）
    gc.collect()

    # 创建大量循环引用对象，预期在 GC 结束后死亡
    create_cyclic_objects(object_count)

    # 创建大量循环引用对象，预期在 GC 结束后生还
    live_objects = create_cyclic_objects(object_count)

    start = time.perf_counter()
    collected = gc.collect()   # 执行全量 GC，回收所有不可达对象
    end = time.perf_counter()
    elapsed_ms = end - start
    print(elapsed_ms)

    # 恢复自动 GC（可选）
    gc.enable()

main(10000)

# S.A.A.U.S.O 0.0024s
# 3.12 0.0012s
# 3.8 0.0021s
# 3.5 0.0017s