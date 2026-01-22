#pragma once
#include "MeasurementTypes.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace weather {

class AnyMeasurement {
public:
    static constexpr std::size_t SboSizeBytes = 32;

    AnyMeasurement() = delete;

    static AnyMeasurement empty() noexcept
    {
        AnyMeasurement m(EmptyTag{});
        return m;
    }

    bool isEmpty() const noexcept
    {
        return header_.kind == MeasurementKind::Empty;
    }

    template <typename T>
    AnyMeasurement(MeasurementHeaderV1 header, T measurement)
        : header_(header)
    {
        using U = std::decay_t<T>;
        (void)MeasurementKindOf<U>::value;

        static_assert(std::is_copy_constructible<U>::value,
                      "Measurement type must be copy-constructible.");
        static_assert(sizeof(U) <= SboSizeBytes,
                      "Measurement type too large for AnyMeasurement SBO storage.");
        static_assert(alignof(U) <= alignof(Storage),
                      "Measurement type alignment exceeds AnyMeasurement storage alignment.");

        header_.kind = MeasurementKindOf<U>::value;

        ops_ = &OpsFor<U>();
        new (storage_.bytes) U(std::move(measurement));
    }

    AnyMeasurement(const AnyMeasurement& other) noexcept
        : header_(other.header_), ops_(other.ops_)
    {
        assert(ops_);
        ops_->copy_construct(storage_.bytes, other.storage_.bytes);
    }

    AnyMeasurement& operator=(const AnyMeasurement& other) noexcept
    {
        if (this == &other) return *this;
        destroy_if_needed();
        header_ = other.header_;
        ops_ = other.ops_;
        assert(ops_);
        ops_->copy_construct(storage_.bytes, other.storage_.bytes);
        return *this;
    }

    AnyMeasurement(AnyMeasurement&& other) noexcept
        : header_(other.header_), ops_(other.ops_)
    {
        assert(ops_);
        ops_->move_construct(storage_.bytes, other.storage_.bytes);
        other.ops_ = &EmptyOps();
        other.header_ = {};
        other.header_.kind = MeasurementKind::Empty;
        other.header_.source = SourceId::Unknown;
    }

    AnyMeasurement& operator=(AnyMeasurement&& other) noexcept
    {
        if (this == &other) return *this;
        destroy_if_needed();
        header_ = other.header_;
        ops_ = other.ops_;
        assert(ops_);
        ops_->move_construct(storage_.bytes, other.storage_.bytes);

        other.ops_ = &EmptyOps();
        other.header_ = {};
        other.header_.kind = MeasurementKind::Empty;
        other.header_.source = SourceId::Unknown;

        return *this;
    }

    ~AnyMeasurement() { destroy_if_needed(); }

    MeasurementKind kind() const noexcept { return header_.kind; }

    template <typename T>
    const T* try_get() const noexcept
    {
        using U = std::decay_t<T>;
        (void)MeasurementKindOf<U>::value;
        if (ops_ != &OpsFor<U>()) return nullptr;
        return reinterpret_cast<const U*>(storage_.bytes);
    }

    template <typename T>
    const T& get() const noexcept
    {
        const T* p = try_get<T>();
        assert(p);
        return *p;
    }

private:
    struct Storage {
        alignas(std::max_align_t) std::byte bytes[SboSizeBytes];
    };

    struct Ops {
        void (*destroy)(void*) noexcept;
        void (*copy_construct)(void*, const void*) noexcept;
        void (*move_construct)(void*, void*) noexcept;
        MeasurementKind kind;
        std::uint16_t measurement_size;
    };

    struct EmptyTag {};

    explicit AnyMeasurement(EmptyTag) noexcept
    {
        header_ = {};
        header_.kind = MeasurementKind::Empty;
        header_.source = SourceId::Unknown;
        ops_ = &EmptyOps();
    }

    static void empty_destroy(void*) noexcept {}
    static void empty_copy_construct(void*, const void*) noexcept {}
    static void empty_move_construct(void*, void*) noexcept {}

    static const Ops& EmptyOps() noexcept
    {
        static const Ops ops{
            &empty_destroy,
            &empty_copy_construct,
            &empty_move_construct,
            MeasurementKind::Empty,
            0
        };
        return ops;
    }

    template <typename T>
    static void destroy_fn(void* p) noexcept { reinterpret_cast<T*>(p)->~T(); }

    template <typename T>
    static void copy_construct_fn(void* dst, const void* src) noexcept {
        new (dst) T(*reinterpret_cast<const T*>(src));
    }

    template <typename T>
    static void move_construct_fn(void* dst, void* src) noexcept {
        new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        reinterpret_cast<T*>(src)->~T();
    }

    template <typename T>
    static const Ops& OpsFor() noexcept
    {
        static const Ops ops{
            &destroy_fn<T>,
            &copy_construct_fn<T>,
            &move_construct_fn<T>,
            MeasurementKindOf<T>::value,
            static_cast<std::uint16_t>(sizeof(T))
        };
        return ops;
    }

    void destroy_if_needed() noexcept
    {
        if (ops_) {
            ops_->destroy(storage_.bytes);
            ops_ = nullptr;
        }
    }

private:
    MeasurementHeaderV1 header_{};
    Storage storage_{};
    const Ops* ops_ = nullptr;
};

} // namespace weather
