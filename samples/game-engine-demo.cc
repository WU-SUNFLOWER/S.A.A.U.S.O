// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso-embedder.h"
#include "saauso.h"

using namespace saauso;

namespace {

// 每帧直接通过 Function::Call 调用，模拟“游戏脚本回调”。
constexpr std::string_view kAiScript =
    "def on_update():\n"
    "    hp = GetPlayerHealth()\n"
    "    if hp <= 20:\n"
    "        SetPlayerStatus('danger')\n"
    "    elif hp <= 60:\n"
    "        SetPlayerStatus('under_attack')\n"
    "    else:\n"
    "        SetPlayerStatus('normal')\n";

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
    if (maybe_on_update.IsEmpty()) {
      std::cout << "get on_update function failed" << std::endl;
      return 1;
    }

    Local<Function> on_update =
        Local<Function>::Cast(maybe_on_update.ToLocalChecked());

    std::cout << "=== Game Loop Start ===" << std::endl;
    for (int frame = 1; frame <= 6; ++frame) {
      TryCatch call_try_catch(isolate);
      MaybeLocal<Value> call_result =
          on_update->Call(context, Local<Value>(), 0, nullptr);
      if (call_result.IsEmpty() || call_try_catch.HasCaught()) {
        std::cout << "on_update() call failed, switch to script fallback"
                  << std::endl;
        return 1;
      }

      g_game_state.player_health -= 15;
      std::cout << "[frame " << frame << "] hp=" << g_game_state.player_health
                << ", status=" << g_game_state.player_status << std::endl;
    }
  }
  std::cout << "=== Game Loop End ===" << std::endl;
#else
    std::cout << "CPython frontend compiler is disabled; "
                 "game_engine_demo only runs in frontend build."
              << std::endl;
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

  isolate->Dispose();
  Saauso::Dispose();

  return 0;
}
