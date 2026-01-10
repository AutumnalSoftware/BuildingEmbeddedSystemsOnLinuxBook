// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
class AnyNmeaSentence
{
public:
 AnyNmeaSentence() = default;
 /// Primary constructor (1)
 template <class T,
 class Decayed = std::decay_t<T>,
 std::enable_if_t<!std::is_same<Decayed, AnyNmeaSentence>::value, int> = 0>
 AnyNmeaSentence(T&& sentence)
 : self_(std::make_unique<Model<Decayed>>(std::forward<T>(sentence)))
 {
 }
 AnyNmeaSentence(const AnyNmeaSentence& other)
 : self_(other.self_ ? other.self_->clone() : nullptr)
 {
 }
 AnyNmeaSentence& operator=(const AnyNmeaSentence& other)
 {
 if (this != &other)
 {
 self_ = other.self_ ? other.self_->clone() : nullptr;
 }
 return *this;
 }
 AnyNmeaSentence(AnyNmeaSentence&&) noexcept = default;
 AnyNmeaSentence& operator=(AnyNmeaSentence&&) noexcept = default;
 bool hasValue() const noexcept
 {
 return static_cast<bool>(self_);
 }
 explicit operator bool() const noexcept
 {
 return hasValue();
 }
 std::string_view talker() const
 {
 return self_ ? self_->talker() : std::string_view{};
 }
 std::string_view sentenceType() const
 {
 return self_ ? self_->sentenceType() : std::string_view{};
 }
 bool isChecksumValid() const
 {
 return self_ ? self_->isChecksumValid() : false;
 }
private:
 struct Concept
 {
 virtual ~Concept() = default;
 virtual std::unique_ptr<Concept> clone() const = 0;
 virtual std::string_view talker() const = 0;
 virtual std::string_view sentenceType() const = 0;
 virtual bool isChecksumValid() const = 0;
 };
 template <class T>
 struct Model final : Concept
 {
 explicit Model(T sentence)
 : sentence_(std::move(sentence))
 {
 }
 std::unique_ptr<Concept> clone() const override
 {
 return std::make_unique<Model<T>>(sentence_);
 }
 std::string_view talker() const override
 {
 return TalkerAccessor<T>::get(sentence_);
 }
 std::string_view sentenceType() const override
 {
 return SentenceTypeAccessor<T>::get(sentence_);
 }
 bool isChecksumValid() const override
 {
 return ChecksumValidAccessor<T>::get(sentence_);
 }
 T sentence_;
 };
private:
 static std::string_view toStringView(std::string_view sv)
 {
 return sv;
 }
 static std::string_view toStringView(const std::string& s)
 {
 return std::string_view{s};
 }
 static std::string_view toStringView(const char* s)
 {
 return s ? std::string_view{s} : std::string_view{};
 }
private:
 // talker: supports s.talker (field) or s.talker() (method)
 template <class T, class = void>
 struct TalkerAccessor
 {
 static std::string_view get(const T& s)
 {
 return toStringView(s.talker);
 }
 };
 template <class T>
 struct TalkerAccessor<T, decltype(void(std::declval<const T&>().talker()))>
 {
 static std::string_view get(const T& s)
 {
 return toStringView(s.talker());
 }
 };
 // sentenceType: supports s.sentenceType (field) or s.sentenceType() (method)
 template <class T, class = void>
 struct SentenceTypeAccessor
 {
 static std::string_view get(const T& s)
 {
 return toStringView(s.sentenceType);
 }
 };
 template <class T>
 struct SentenceTypeAccessor<T, decltype(void(std::declval<const T&>().sentenceType()))>
 {
 static std::string_view get(const T& s)
 {
 return toStringView(s.sentenceType());
 }
 };
 // isChecksumValid: supports s.isChecksumValid() only (method)
 template <class T, class = void>
 struct ChecksumValidAccessor
 {
 static bool get(const T&)
 {
 static_assert(sizeof(T) == 0, "Concrete sentence type must implement: bool isChecksumValid() const");
 return false;
 }
 };
 template <class T>
 struct ChecksumValidAccessor<T, decltype(void(std::declval<const T&>().isChecksumValid()))>
 {
 static bool get(const T& s)
 {
 return static_cast<bool>(s.isChecksumValid());
 }
 };
private:
 std::unique_ptr<Concept> self_;
};
