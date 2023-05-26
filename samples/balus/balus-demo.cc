// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "./util.h"
#include "include/libplatform/libplatform.h"
#include "include/v8-context.h"
#include "include/v8-exception.h"
#include "include/v8-function.h"
#include "include/v8-initialization.h"
#include "include/v8-isolate.h"
#include "include/v8-local-handle.h"
#include "include/v8-primitive.h"
#include "include/v8-script.h"

static v8::Local<v8::String> newV8Str(v8::Isolate* isolate,
                                      const std::string& str);
static Result<std::string> readFile(const std::string& path);

static void ExecuteFile(const v8::FunctionCallbackInfo<v8::Value>& args);
static void TestString(const v8::FunctionCallbackInfo<v8::Value>& args);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage:" << argv[0] << " path" << std::endl;
    return -1;
  }

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

    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    {
      auto registerFunc = [isolate, context, global = context->Global()](
                              const std::string& name,
                              v8::FunctionCallback func) {
        auto js_name = newV8Str(isolate, name);
        auto js_func = v8::Function::New(context, func).ToLocalChecked();
        global->Set(context, js_name, js_func).Check();
      };

      registerFunc("ExecuteFile", ExecuteFile);
      registerFunc("TestString", TestString);

      auto maybeCode = readFile(argv[1]);
      if (maybeCode.IsError()) {
        std::cerr << "Failed to read " << argv[0] << ": "
                  << maybeCode.GetErrorMessage() << std::endl;
        return -1;
      }

      v8::Local<v8::String> source = newV8Str(isolate, maybeCode.GetValue());
      v8::TryCatch tryCatch(isolate);
      v8::Local<v8::Script> script;
      if (!v8::Script::Compile(context, source).ToLocal(&script)) {
        if (tryCatch.HasCaught()) {
          v8::String::Utf8Value ex(isolate, tryCatch.Exception());
          std::cerr << "Failed to compile" << argv[0] << ": " << *ex
                    << std::endl;
        }
        return -1;
      }
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
      [[maybe_unused]] v8::String::Utf8Value utf8(isolate, result);
      // printf("result: %s\n", *utf8);
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}

static v8::Local<v8::String> newV8Str(v8::Isolate* isolate,
                                      const std::string& str) {
  v8::EscapableHandleScope scope(isolate);
  v8::Local<v8::String> ret =
      v8::String::NewFromUtf8(isolate, str.data(), v8::NewStringType::kNormal,
                              static_cast<int>(str.size()))
          .ToLocalChecked();
  return scope.Escape(ret);
}

static Result<std::string> readFile(const std::string& path) {
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return MakeError<std::string>(strerror(errno));
  }

  struct stat statbuf {};
  ::fstat(fd, &statbuf);

  std::string text;
  text.resize(statbuf.st_size);
  ssize_t nread = ::read(fd, text.data(), statbuf.st_size);
  if (nread != statbuf.st_size) {
    ::close(fd);
    return MakeError<std::string>(strerror(errno));
  }

  ::close(fd);
  return text;
}

static void ExecuteFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (args.Length() == 0) {
    isolate->ThrowException(v8::Exception::TypeError(
        newV8Str(isolate,
                 "Failed to execute 'ExecuteFile': expect 1 parameter, but "
                 "only 0 presents.")));
    return;
  }
}

static void TestString(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (args.Length() == 0) {
    isolate->ThrowException(v8::Exception::TypeError(
        newV8Str(isolate,
                 "Failed to execute 'TestString': expect 1 parameter, but "
                 "only 0 presents.")));
    return;
  }

  v8::Local<v8::String> tostring;
  if (!args[0]->ToString(context).ToLocal(&tostring)) {
    isolate->ThrowException(v8::Exception::TypeError(
        newV8Str(isolate,
                 "Failed to execute 'TestString': the provided value cannot be "
                 "converted to 'string'")));
    return;
  }

  v8::String::Utf8Value u(isolate, tostring);
  std::printf(
      "=============================TEST STRING============================\n"
      "is one byte:%d, only contains one byte:%d, length:%d, utf8 length:%d, "
      "Utf8Value: [%d]:[%s]\n",
      tostring->IsOneByte(), tostring->ContainsOnlyOneByte(),
      tostring->Length(), tostring->Utf8Length(isolate), u.length(), *u);

  // test UTF-16
  int u16_len = tostring->Length();
  std::vector<uint16_t> u16_buf(u16_len, 0xEE);
  int options = v8::String::WriteOptions::NO_NULL_TERMINATION |
                v8::String::WriteOptions::REPLACE_INVALID_UTF8;
  int u16_written = tostring->Write(isolate, u16_buf.data(), 0, -1, options);
  std::printf(
      "\t*************TEST UTF16*************\n"
      "\tu16_len:%d, u16_written:%d\n\t",
      u16_len, u16_written);
  for (auto u16 : u16_buf) {
    std::printf("%x ", u16);
  }
  std::printf("\n\t");
  for (auto u16 : u16_buf) {
    std::printf("%x %x ", uint8_t(u16 >> 8), uint8_t(u16 & 0xFF));
  }

  // write UTF-8
  int u8_len = tostring->Utf8Length(isolate);
  std::vector<char> u8_buf(u8_len, 0xEE);
  int nchars = 0;
  int u8_written =
      tostring->WriteUtf8(isolate, u8_buf.data(), -1, &nchars, options);
  std::printf(
      "\n\t*************TEST UTF8*************\n"
      "\tu8_len:%d, u8_written:%d\n\t",
      u8_len, u8_written);
  for (auto u8 : u8_buf) {
    std::printf("%x ", uint8_t(u8));
  }

  // One Byte
  int onebyte_len = tostring->Utf8Length(isolate);
  std::vector<uint8_t> onebyte_buf(onebyte_len, 0xEE);
  int onebyte_written =
      tostring->WriteOneByte(isolate, onebyte_buf.data(), 0, -1, options);
  std::printf(
      "\n\t*************TEST ONEBYTE*************\n"
      "\tonebyte_len:%d, onebyte_written:%d\n\t",
      onebyte_len, onebyte_written);
  for (auto onebyte : onebyte_buf) {
    std::printf("%x ", uint8_t(onebyte));
  }

  std::printf("\n");
}
