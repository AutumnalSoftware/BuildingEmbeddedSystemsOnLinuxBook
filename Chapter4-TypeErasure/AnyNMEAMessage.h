// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
//#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>

// Forward declarations (use your real headers)
class NMEAInsertionStream;
class NMEAExtractionStream;

// Optional trait: specialize per message type to supply its 3-letter code.
template <class T>
struct NMEATraits
{
    // You must specialize this for each message type you want to support,
    // or use the (talker, messageName, value) constructor.
    static std::string_view messageName();
};

namespace detail
{
template <class...>
struct AlwaysFalse : std::false_type {};

template <class T, class = void>
struct IsNMEAInsertable : std::false_type {};

template <class T>
struct IsNMEAInsertable<T, std::void_t<
                               decltype(std::declval<NMEAInsertionStream&>() << std::declval<const T&>())
                               >> : std::true_type {};

template <class T, class = void>
struct IsNMEAExtractable : std::false_type {};

template <class T>
struct IsNMEAExtractable<T, std::void_t<
                                decltype(std::declval<NMEAExtractionStream&>() >> std::declval<T&>())
                                >> : std::true_type {};
}

class AnyNMEAMessage
{
public:
    AnyNMEAMessage() = default;

    // talker + explicit messageName + value
    template <class T>
    AnyNMEAMessage(std::string_view talker, std::string_view messageName, T&& value)
        : self_(std::make_unique<Model<std::decay_t<T>>>(std::forward<T>(value)))
    {
        setTalker(talker);
        setMessageName(messageName);

        static_assert(detail::IsNMEAInsertable<std::decay_t<T>>::value,
                      "AnyNMEAMessage payload type must support: NMEAInsertionStream& operator<<(NMEAInsertionStream&, const T&)");
        static_assert(detail::IsNMEAExtractable<std::decay_t<T>>::value,
                      "AnyNMEAMessage payload type must support: NMEAExtractionStream& operator>>(NMEAExtractionStream&, T&)");
    }

    // talker + value, messageName deduced via NMEATraits<T>::messageName()
    template <class T>
    AnyNMEAMessage(std::string_view talker, T&& value)
        : AnyNMEAMessage(talker, deduceMessageName<std::decay_t<T>>(), std::forward<T>(value))
    {}

    // Copy / move
    AnyNMEAMessage(const AnyNMEAMessage& o)
        : self_(o.self_ ? o.self_->clone() : nullptr)
        , talker_(o.talker_)
        , messageName_(o.messageName_)
        , checksum_(o.checksum_)
        , size_(o.size_)
    {}

    AnyNMEAMessage& operator=(const AnyNMEAMessage& o)
    {
        if (this != &o)
        {
            self_        = o.self_ ? o.self_->clone() : nullptr;
            talker_      = o.talker_;
            messageName_ = o.messageName_;
            checksum_    = o.checksum_;
            size_        = o.size_;
        }
        return *this;
    }

    AnyNMEAMessage(AnyNMEAMessage&&) noexcept = default;
    AnyNMEAMessage& operator=(AnyNMEAMessage&&) noexcept = default;

    // ---------------------------------------------------------------------
    // State
    // ---------------------------------------------------------------------
    bool empty() const noexcept { return self_ == nullptr; }
    explicit operator bool() const noexcept { return !empty(); }

    void reset() noexcept
    {
        self_.reset();
        talker_.fill('\0');
        messageName_.fill('\0');
        checksum_ = 0;
        size_ = 0;
    }

    // ---------------------------------------------------------------------
    // Metadata (fixed-size storage, no heap)
    // ---------------------------------------------------------------------
    std::string_view getTalker() const noexcept
    {
        return std::string_view(talker_.data(), 2);
    }

    std::string_view getMessageName() const noexcept
    {
        return std::string_view(messageName_.data(), 3);
    }

    std::uint8_t getChecksum() const noexcept { return checksum_; }
    std::size_t  getSize()     const noexcept { return size_; }

    void setChecksum(std::uint8_t c) noexcept { checksum_ = c; }
    void setSize(std::size_t s) noexcept { size_ = s; }

    // If you want to (re)validate after setters, call validateTalkerHeader().
    void setTalker(std::string_view talker)
    {
        if (talker.size() != 2) throw std::invalid_argument("talker must be exactly 2 chars");
        talker_[0] = talker[0];
        talker_[1] = talker[1];
    }

    void setMessageName(std::string_view messageName)
    {
        if (messageName.size() != 3) throw std::invalid_argument("messageName must be exactly 3 chars");
        messageName_[0] = messageName[0];
        messageName_[1] = messageName[1];
        messageName_[2] = messageName[2];
    }

    void validateTalkerHeader() const
    {
        // Here mostly for symmetry / future rules (ASCII upper, etc.)
        if (talker_[0] == '\0' || talker_[1] == '\0')
            throw std::runtime_error("talker not set");
        if (messageName_[0] == '\0' || messageName_[1] == '\0' || messageName_[2] == '\0')
            throw std::runtime_error("messageName not set");
    }

    // ---------------------------------------------------------------------
    // Type queries / access
    // ---------------------------------------------------------------------
    const std::type_info& type() const noexcept
    {
        return self_ ? self_->type() : typeid(void);
    }

    template <class T>
    bool isType() const noexcept
    {
        return self_ && (self_->type() == typeid(T));
    }

    template <class T>
    T* tryGet() noexcept
    {
        return isType<T>() ? &static_cast<Model<T>*>(self_.get())->value_ : nullptr;
    }

    template <class T>
    const T* tryGet() const noexcept
    {
        return isType<T>() ? &static_cast<const Model<T>*>(self_.get())->value_ : nullptr;
    }

    template <class T>
    T& get()
    {
        auto* p = tryGet<T>();
        if (!p) throw std::bad_cast();
        return *p;
    }

    template <class T>
    const T& get() const
    {
        auto* p = tryGet<T>();
        if (!p) throw std::bad_cast();
        return *p;
    }

    // ---------------------------------------------------------------------
    // Payload serialization/deserialization (NO framing/EndMsg here)
    // ---------------------------------------------------------------------
    void serializePayload(NMEAInsertionStream& ns) const
    {
        if (!self_) throw std::runtime_error("Empty AnyNMEAMessage");
        self_->write(ns); // expects ns << value_ to write PAYLOAD ONLY
    }

    void deserializePayload(NMEAExtractionStream& ex)
    {
        if (!self_) throw std::runtime_error("Empty AnyNMEAMessage");
        self_->read(ex);  // expects ex >> value_ to read PAYLOAD ONLY
    }

private:
    // Type-erasure core
    struct Concept
    {
        virtual ~Concept() = default;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual const std::type_info& type() const noexcept = 0;
        virtual void write(NMEAInsertionStream&) const = 0;
        virtual void read(NMEAExtractionStream&) = 0;
    };

    template <class T>
    struct Model final : Concept
    {
        T value_;

        explicit Model(T v)
            : value_(std::move(v))
        {}

        std::unique_ptr<Concept> clone() const override
        {
            return std::make_unique<Model<T>>(value_);
        }

        const std::type_info& type() const noexcept override
        {
            return typeid(T);
        }

        void write(NMEAInsertionStream& ns) const override
        {
            ns << value_; // ADL finds operator<< beside T (or in associated namespace)
        }

        void read(NMEAExtractionStream& ex) override
        {
            ex >> value_; // ADL finds operator>> beside T (or in associated namespace)
        }
    };

    template <class T>
    static std::string_view deduceMessageName()
    {
        // If you didn’t specialize NMEATraits<T>, this will typically fail at link time
        // or compile time depending on how you implement it. Here we guard with a
        // “can’t compile” fallback if someone tries to use it without providing it.
        if constexpr (std::is_same_v<decltype(NMEATraits<T>::messageName()), std::string_view>)
        {
            auto sv = NMEATraits<T>::messageName();
            if (sv.size() != 3) throw std::invalid_argument("NMEATraits<T>::messageName() must return 3 chars");
            return sv;
        }
        else
        {
            static_assert(detail::AlwaysFalse<T>::value,
                          "NMEATraits<T>::messageName() must be specialized to return std::string_view");
            return {};
        }
    }

private:
    std::unique_ptr<Concept> self_{nullptr};

    // Exactly-sized tokens; avoids heap allocations for fixed metadata
    std::array<char, 2> talker_{ {'\0','\0'} };
    std::array<char, 3> messageName_{ {'\0','\0','\0'} };

    // Optional caches from your framing layer/streams
    std::uint8_t checksum_{0};
    std::size_t  size_{0};
};
