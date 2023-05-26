/*
 * Copyright(C) 2023
 */

#pragma once

#include <cassert>
#include <string>
#include <utility>
#include <variant>

template <typename T>
class Result {
  struct Error {
    Error(std::string m) noexcept : msg(std::move(m)) {}
    std::string msg;
  };

 public:
  template <typename... Args>
  Result(Args&&... args) {
    T val(std::forward<Args>(args)...);
    inner = std::move(val);
  }

  Result(const Result& result) = default;
  Result(Result&& result) = default;
  ~Result() = default;

  bool IsOK() const noexcept { return !IsError(); }
  bool IsError() const noexcept { return std::holds_alternative<Error>(inner); }

  const T& GetValue() const noexcept {
    assert(IsOK());
    return std::get<T>(inner);
  }
  const std::string& GetErrorMessage() const noexcept {
    assert(IsError());
    return std::get<Error>(inner).msg;
  }

 private:
  template <typename U, typename... Args>
  friend Result<U> MakeValue(Args&&... args);
  template <typename U>
  friend Result<U> MakeError(std::string msg);

  struct ErrorTag {};
  Result(ErrorTag, std::string msg) : inner(Error(std::move(msg))) {}

  std::variant<T, Error> inner;
};

template <typename T, typename... Args>
static Result<T> MakeValue(Args&&... args) {
  return Result<T>(std::forward<Args>(args)...);
}

template <typename T>
static Result<T> MakeError(std::string error) {
  return Result<T>(typename Result<T>::ErrorTag{}, std::move(error));
}
