// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso-embedder.h"
#include "saauso.h"

namespace saauso {
namespace {

struct GameState {
  int64_t player_health{100};
  std::string player_status{"normal"};
};

GameState g_game_state;

void HostGetPlayerHealth(FunctionCallbackInfo& info) {
  info.SetReturnValue(Local<Value>::Cast(
      Integer::New(info.GetIsolate(), g_game_state.player_health)));
}

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
}  // namespace saauso

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) {
    return 1;
  }
  int exit_code = 0;

  {
    saauso::HandleScope scope(isolate);
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    if (context.IsEmpty()) {
      exit_code = 1;
    } else {
      saauso::Local<saauso::Object> global = context->Global();
      global->Set(
          saauso::String::New(isolate, "GetPlayerHealth"),
          saauso::Local<saauso::Value>::Cast(saauso::Function::New(
              isolate, &saauso::HostGetPlayerHealth, "GetPlayerHealth")));
      global->Set(
          saauso::String::New(isolate, "SetPlayerStatus"),
          saauso::Local<saauso::Value>::Cast(saauso::Function::New(
              isolate, &saauso::HostSetPlayerStatus, "SetPlayerStatus")));

#if SAAUSO_ENABLE_CPYTHON_COMPILER
      const std::string ai_script =
          "def on_update():\n"
          "    hp = GetPlayerHealth()\n"
          "    if hp <= 20:\n"
          "        SetPlayerStatus('danger')\n"
          "    elif hp <= 60:\n"
          "        SetPlayerStatus('under_attack')\n"
          "    else:\n"
          "        SetPlayerStatus('normal')\n";

      saauso::TryCatch compile_try_catch(isolate);
      saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
          isolate, saauso::String::New(isolate, ai_script));
      if (maybe_script.IsEmpty() || compile_try_catch.HasCaught()) {
        std::cout << "compile failed" << std::endl;
        exit_code = 1;
      } else {
        saauso::TryCatch run_try_catch(isolate);
        saauso::MaybeLocal<saauso::Value> run_result =
            maybe_script.ToLocalChecked()->Run(context);
        if (run_result.IsEmpty() || run_try_catch.HasCaught()) {
          std::cout << "script run failed" << std::endl;
          exit_code = 1;
        } else {
          saauso::MaybeLocal<saauso::Value> maybe_on_update =
              context->Get(saauso::String::New(isolate, "on_update"));
          bool use_fallback_update = maybe_on_update.IsEmpty();
          saauso::Local<saauso::Function> on_update;
          if (!use_fallback_update) {
            on_update = saauso::Local<saauso::Function>::Cast(
                maybe_on_update.ToLocalChecked());
          }

          std::cout << "=== Game Loop Start ===" << std::endl;
          for (int frame = 1; frame <= 6; ++frame) {
            if (!use_fallback_update) {
              saauso::TryCatch call_try_catch(isolate);
              saauso::MaybeLocal<saauso::Value> call_result = on_update->Call(
                  context, saauso::Local<saauso::Value>(), 0, nullptr);
              if (call_result.IsEmpty() || call_try_catch.HasCaught()) {
                std::cout
                    << "on_update() call failed, switch to script fallback"
                    << std::endl;
                use_fallback_update = true;
              }
            }
            if (use_fallback_update) {
              const std::string fallback_script =
                  "hp = GetPlayerHealth()\n"
                  "if hp <= 20:\n"
                  "    SetPlayerStatus('danger')\n"
                  "elif hp <= 60:\n"
                  "    SetPlayerStatus('under_attack')\n"
                  "else:\n"
                  "    SetPlayerStatus('normal')\n";
              saauso::TryCatch fallback_try_catch(isolate);
              saauso::MaybeLocal<saauso::Script> fallback_compile =
                  saauso::Script::Compile(
                      isolate, saauso::String::New(isolate, fallback_script));
              if (fallback_compile.IsEmpty() ||
                  fallback_try_catch.HasCaught() ||
                  fallback_compile.ToLocalChecked()->Run(context).IsEmpty()) {
                std::cout << "fallback update failed at frame " << frame
                          << std::endl;
                exit_code = 1;
                break;
              }
            }

            saauso::g_game_state.player_health -= 15;
            std::cout << "[frame " << frame
                      << "] hp=" << saauso::g_game_state.player_health
                      << ", status=" << saauso::g_game_state.player_status
                      << std::endl;
          }
          std::cout << "=== Game Loop End ===" << std::endl;
        }
      }
#else
      std::cout << "CPython frontend compiler is disabled; "
                   "game_engine_demo only runs in frontend build."
                << std::endl;
#endif
    }
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return exit_code;
}
