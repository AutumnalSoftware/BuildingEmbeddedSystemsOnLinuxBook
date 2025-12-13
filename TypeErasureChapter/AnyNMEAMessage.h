//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <iostream>

// AnyNMEAMessage.hpp
#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>

using namespace std;

// Your stream types (use your real headers)
class NMEAInsertionStream;
class NMEAExtractionStream;

// ADL hooks you define beside each message type
template <class T> NMEAInsertionStream& operator<<(NMEAInsertionStream&, const T&);
template <class T> NMEAExtractionStream& operator>>(NMEAExtractionStream&, T&);

// Optional trait: specialize per message type to supply its 3-letter code
template <class T>
struct NMEATraits
{
    static std::string messageName(); // e.g. return "GGA"
};

class AnyNMEAMessage
{
public:
    AnyNMEAMessage() = default;

    // talker + explicit messageName + value
    template <class T>
    AnyNMEAMessage(std::string talker, std::string messageName, T value)
        : self_(std::make_unique<Model<T>>(std::move(value)))
        , talker_(std::move(talker))
        , messageName_(std::move(messageName))
    {
        validateTalkerHeader();
    }

    // talker + value, messageName deduced via NMEATraits<T>
    template <class T>
    AnyNMEAMessage(std::string talker, T value)
        : self_(std::make_unique<Model<T>>(std::move(value)))
        , talker_(std::move(talker))
        , messageName_(NMEATraits<T>::messageName())
    {
        validateTalkerHeader();
    }

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
            self_         = o.self_ ? o.self_->clone() : nullptr;
            talker_       = o.talker_;
            messageName_  = o.messageName_;
            checksum_     = o.checksum_;
            size_         = o.size_;
        }
        return *this;
    }

    AnyNMEAMessage(AnyNMEAMessage&&) noexcept = default;
    AnyNMEAMessage& operator=(AnyNMEAMessage&&) noexcept = default;

    bool isEmpty() const { return self_ == nullptr; }

    explicit operator bool() const noexcept { return isEmpty(); }

    // Type queries / access
    const std::type_info& type() const noexcept
    {
        return self_ ? self_->type() : typeid(void);
    }

    template <class T>
    bool isType() const
    {
        return self_ && (self_->type() == typeid(T));
    }

    template <class T>
    T& get()
    {
        checkType<T>();
        return static_cast<Model<T>*>(self_.get())->value_;
    }

    template <class T>
    const T& get() const
    {
        checkType<T>();
        return static_cast<const Model<T>*>(self_.get())->value_;
    }

    // Serialization / deserialization — payload only; your ADL frames/deframes
    void serialize(NMEAInsertionStream& ns) const
    {
        if (!self_) throw std::runtime_error("Empty AnyNMEAMessage");
        using ::operator<<;      // ADL
        self_->write(ns);        // ns << value_;
        // If your inserter exposes these, feel free to uncomment:
        // checksum_ = ns.checksum();
        // size_     = ns.size();
    }

    void deserialize(NMEAExtractionStream& ex)
    {
        if (!self_) throw std::runtime_error("Empty AnyNMEAMessage");
        using ::operator>>;      // ADL
        self_->read(ex);         // ex >> value_;
        // If your extractor exposes these, you can cache them:
        // talker_   = ex.talker();
        // header_   = ex.header();
        // checksum_ = ex.checksum();
        // size_     = ex.size();
    }

    // Read-only metadata (set at construction; optionally refresh internally after (de)serialize)
    const std::string& getTalker()      const noexcept { return talker_; }
    const std::string& getMessageName() const noexcept { return messageName_; }
    std::uint8_t       getChecksum()    const noexcept { return checksum_; }
    std::size_t        getSize()        const noexcept { return size_; }

private:
    // Type-erasure core
    struct Concept
    {
        virtual ~Concept() = default;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual const std::type_info& type() const noexcept = 0;
        virtual void write(NMEAInsertionStream&) const = 0; // ADL payload write
        virtual void read (NMEAExtractionStream&)      = 0; // ADL payload read
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
            using ::operator<<; ns << value_;
        }

        void read(NMEAExtractionStream& ex) override
        {
            using ::operator>>; ex >> value_;
        }
    };

    template <class T>
    void checkType() const
    {
        if (!self_ || self_->type() != typeid(T)) throw std::bad_cast();
    }

    void validateTalkerHeader() const
    {
        if (talker_.size() != 2) throw std::runtime_error("talker must be 2 chars");
        if (messageName_.size() != 3) throw std::runtime_error("messageName must be 3 chars");
    }

private:
    std::unique_ptr<Concept> self_ { nullptr };
    std::string talker_;
    std::string messageName_;
    std::uint8_t checksum_ = 0; // optional cache from your streams
    std::size_t  size_     = 0; // optional cache from your streams
};
