// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso-embedder.h"
#include "saauso.h"

using namespace saauso;

namespace {

// 主路径：先把 Python 逻辑编译成 on_update() 函数，
// 之后每帧直接通过 Function::Call 调用，模拟“游戏脚本回调”。
constexpr std::string_view kAiScript =
    "def on_update():\n"
    "    hp = GetPlayerHealth()\n"
    "    if hp <= 20:\n"
    "        SetPlayerStatus('danger')\n"
    "    elif hp <= 60:\n"
    "        SetPlayerStatus('under_attack')\n"
    "    else:\n"
    "        SetPlayerStatus('normal')\n";

// 兜底路径：当 on_update() 调用失败时，退化为“每帧直接执行脚本片段”。
// 这段脚本与 kAiScript 的核心业务逻辑保持一致，用来展示
// TryCatch + MaybeLocal 的可恢复降级能力。
constexpr std::string_view kFallbackScript =
    "hp = GetPlayerHealth()\n"
    "if hp <= 20:\n"
    "    SetPlayerStatus('danger')\n"
    "elif hp <= 60:\n"
    "    SetPlayerStatus('under_attack')\n"
    "else:\n"
    "    SetPlayerStatus('normal')\n";

struct GameState {
  int64_t player_health{100};
  std::string player_status{"normal"};
};

GameState g_game_state;

// 宿主 API：给脚本读当前玩家血量。
void HostGetPlayerHealth(FunctionCallbackInfo& info) {
  info.SetReturnValue(Local<Value>::Cast(
      Integer::New(info.GetIsolate(), g_game_state.player_health)));
}

// 宿主 API：让脚本回写玩家状态。
void HostSetPlayerStatus(FunctionCallbackInfo& info) {
  std::string status;
  if (!info.GetStringArg(0, &status)) {
    info.ThrowRuntimeError("SetPlayerStatus expects string argument");
    return;
  }
  g_game_state.player_status = status;
  info.SetReturnValue(Local<Value>::Cast(
      String::New(info.GetIsolate(), g_game_state.player_status)));
}

}  // namespace

int main() {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  if (isolate == nullptr) {
    return 1;
  }

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    if (context.IsEmpty()) {
      return 1;
    }

    Local<Object> global = context->Global();
    global->Set(String::New(isolate, "GetPlayerHealth"),
                Local<Value>::Cast(Function::New(isolate, &HostGetPlayerHealth,
                                                 "GetPlayerHealth")));
    global->Set(String::New(isolate, "SetPlayerStatus"),
                Local<Value>::Cast(Function::New(isolate, &HostSetPlayerStatus,
                                                 "SetPlayerStatus")));

#if SAAUSO_ENABLE_CPYTHON_COMPILER

    TryCatch compile_try_catch(isolate);
    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kAiScript));
    if (maybe_script.IsEmpty() || compile_try_catch.HasCaught()) {
      std::cout << "compile failed" << std::endl;
      return 1;
    }

    TryCatch run_try_catch(isolate);
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    if (run_result.IsEmpty() || run_try_catch.HasCaught()) {
      std::cout << "script run failed" << std::endl;
      return 1;
    }

    MaybeLocal<Value> maybe_on_update =
        context->Get(String::New(isolate, "on_update"));
    // 若没有拿到 on_update，直接进入兜底脚本模式。
    bool use_fallback_update = maybe_on_update.IsEmpty();
    Local<Function> on_update;
    if (!use_fallback_update) {
      on_update = Local<Function>::Cast(maybe_on_update.ToLocalChecked());
    }

    std::cout << "=== Game Loop Start ===" << std::endl;
    for (int frame = 1; frame <= 6; ++frame) {
      if (!use_fallback_update) {
        TryCatch call_try_catch(isolate);
        MaybeLocal<Value> call_result =
            on_update->Call(context, Local<Value>(), 0, nullptr);
        if (call_result.IsEmpty() || call_try_catch.HasCaught()) {
          std::cout << "on_update() call failed, switch to script fallback"
                    << std::endl;
          use_fallback_update = true;
        }
      }
      if (use_fallback_update) {
        // 兜底模式：每帧重新编译并执行最小脚本，保证演示流程可继续。
        TryCatch fallback_try_catch(isolate);
        MaybeLocal<Script> fallback_compile =
            Script::Compile(isolate, String::New(isolate, kFallbackScript));
        if (fallback_compile.IsEmpty() || fallback_try_catch.HasCaught() ||
            fallback_compile.ToLocalChecked()->Run(context).IsEmpty()) {
          std::cout << "fallback update failed at frame " << frame << std::endl;
          return 1;
        }
      }

      g_game_state.player_health -= 15;
      std::cout << "[frame " << frame << "] hp=" << g_game_state.player_health
                << ", status=" << g_game_state.player_status << std::endl;
    }
    std::cout << "=== Game Loop End ===" << std::endl;

#else
    std::cout << "CPython frontend compiler is disabled; "
                 "game_engine_demo only runs in frontend build."
              << std::endl;
#endif
  }

  isolate->Dispose();
  Saauso::Dispose();

  return 0;
}
