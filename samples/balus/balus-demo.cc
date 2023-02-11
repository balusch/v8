// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <functional>
#include <iostream>

#include "include/libplatform/libplatform.h"
#include "include/v8-context.h"
#include "include/v8-exception.h"
#include "include/v8-initialization.h"
#include "include/v8-isolate.h"
#include "include/v8-local-handle.h"
#include "include/v8-primitive.h"
#include "include/v8-script.h"

static void DisplayIsolateStats(v8::Isolate* isolate);

int main(int argc, char* argv[]) {
  // Initialize V8.
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);

    v8::TryCatch catcher(isolate);

    if (catcher.HasCaught()) {
      if (catcher.HasTerminated()) {
        auto msg = catcher.Message();
        msg->ErrorLevel()
        catcher.Reset();
      }
    }

    isolate->RequestInterrupt();
    isolate->TerminateExecution();
    isolate->CancelTerminateExecution();
    isolate->IsExecutionTerminating();
    isolate->PerformMicrotaskCheckpoint()
    v8::Message::GetLineNumber()

    isolate->PerformMicrotaskCheckpoint();
    DisplayIsolateStats(isolate);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    {
      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source =
          v8::String::NewFromUtf8Literal(isolate, "'Hello' + ', World!'");

      // Compile the source code.
      v8::Local<v8::Script> script =
          v8::Script::Compile(context, source).ToLocalChecked();

      // Run the script to get the result.
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

      // Convert the result to an UTF8 string and print it.
      v8::String::Utf8Value utf8(isolate, result);
      printf("%s\n", *utf8);
    }

    {
      // Use the JavaScript API to generate a WebAssembly module.
      //
      // |bytes| contains the binary format for the following module:
      //
      //     (func (export "add") (param i32 i32) (result i32)
      //       get_local 0
      //       get_local 1
      //       i32.add)
      //
      const char csource[] = R"(
        let bytes = new Uint8Array([
          0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
          0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
          0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
          0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
        ]);
        let module = new WebAssembly.Module(bytes);
        let instance = new WebAssembly.Instance(module);
        instance.exports.add(3, 4);
      )";

      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source =
          v8::String::NewFromUtf8Literal(isolate, csource);

      // Compile the source code.
      v8::Local<v8::Script> script =
          v8::Script::Compile(context, source).ToLocalChecked();

      // Run the script to get the result.
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

      // Convert the result to a uint32 and print it.
      uint32_t number = result->Uint32Value(context).ToChecked();
      printf("3 + 4 = %u\n", number);
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}

static void DisplayIsolateStats(v8::Isolate* isolate) {
  v8::HeapStatistics hs;
  isolate->GetHeapStatistics(&hs);

  std::printf(
      "total_heap_size:%lu\ntotal_heap_size_executable:%lu\n"
      "total_physical_size:%lu\ntotal_available_size:%lu\n"
      "total_global_handlers_size:%lu\nused_global_handles_size:%lu\n"
      "used_heap_size:%lu\nheap_size_limit:%lu\nmalloced_memory:%lu\n"
      "external_memory:%lu\npeak_malloced_memory:%lu\n"
      "number_of_native_contexts:%lu\nnumber_of_detached_contexts:%lu\n",
      hs.total_heap_size(), hs.total_heap_size_executable(),
      hs.total_physical_size(), hs.total_available_size(),
      hs.total_global_handles_size(), hs.used_global_handles_size(),
      hs.used_heap_size(), hs.heap_size_limit(), hs.malloced_memory(),
      hs.external_memory(), hs.peak_malloced_memory(),
      hs.number_of_native_contexts(), hs.number_of_detached_contexts());

  v8::ResourceConstraints rc;
}

struct HttpResponse;
static std::string toString(v8::Isolate* isolate, v8::Local<v8::Value> v8str);
static v8::Local<v8::Object> wrapResponse(v8::Isolate* isolate,
                                          HttpResponse* rsp);
static void SendHttpRequest(const std::string url,
                            std::function<void(HttpResponse*)> on_done,
                            std::function<void(std::string)> on_error);

static void JsFetchApi(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto* isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
  args.GetReturnValue().Set(resolver->GetPromise());

  std::string url = toString(isolate, args[0]);

  SendHttpRequest(
      url,
      [isolate, resolver](HttpResponse* rsp) {
        auto context = isolate->GetCurrentContext();
        auto v8rsp = wrapResponse(isolate, rsp);
        resolver->Resolve(context, v8rsp).Check();
      },
      [isolate, resolver](std::string errmsg) {
        auto context = isolate->GetCurrentContext();
        auto ex = v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, errmsg.c_str(),
                                    v8::NewStringType::kNormal,
                                    static_cast<int>(errmsg.size()))
                .ToLocalChecked());
        resolver->Reject(context, ex).Check();
      });
}

static void JsFetchApiInternal(std::variant<std::string, Request>, std::optional<Object>) {

}

registerApi('fetch', JsFetrchAPiInternal);

static void JsFetchApi(const v8::FunctionCallbackInfo<v8::Value>& args) {
  ....
  JsFetchAPiInternal(...);
}
