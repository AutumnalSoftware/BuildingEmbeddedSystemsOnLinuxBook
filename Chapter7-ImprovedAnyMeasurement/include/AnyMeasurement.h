#pragma once

#include "MeasurementTypes.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace weather
{

class AnyMeasurement
{
public:
    static constexpr std::size_t SboSizeBytes = 32;

    AnyMeasurement() = delete;

    template <typename T>
    AnyMeasurement(const MeasurementHeaderV1& header, T measurement)
        : header_(header)
    {
        using U = std::decay_t<T>;

        // Closed-world enforcement
        (void)MeasurementKindOf<U>::value;

        static_assert(std::is_copy_constructible<U>::value,
                      "Measurement type must be copy-constructible");
        static_assert(sizeof(U) <= SboSizeBytes,
                      "Measurement type too large for AnyMeasurement SBO");
        static_assert(alignof(U) <= alignof(Storage),
                      "Measurement type alignment exceeds SBO storage");

        ops_ = &OpsFor<U>();
        new (storage_.bytes) U(std::move(measurement));
    }

    AnyMeasurement(const AnyMeasurement& other) noexcept
        : header_(other.header_)
        , ops_(other.ops_)
    {
        assert(ops_);
        ops_->copy_construct(storage_.bytes, other.storage_.bytes);
    }

    AnyMeasurement& operator=(const AnyMeasurement& other) noexcept
    {
        if (this == &other) return *this;

        if (ops_)
        {
            ops_->destroy(storage_.bytes);
        }

        header_ = other.header_;
        ops_ = other.ops_;

        assert(ops_);
        ops_->copy_construct(storage_.bytes, other.storage_.bytes);
        return *this;
    }

    AnyMeasurement(AnyMeasurement&& other) noexcept
        : header_(other.header_)
        , ops_(other.ops_)
    {
        if (ops_)
        {
            ops_->move_construct(storage_.bytes, other.storage_.bytes);
            other.ops_ = nullptr;
        }
    }

    AnyMeasurement& operator=(AnyMeasurement&& other) noexcept
    {
        if (this == &other) return *this;

        if (ops_)
        {
            ops_->destroy(storage_.bytes);
        }

        header_ = other.header_;
        ops_ = other.ops_;

        if (ops_)
        {
            ops_->move_construct(storage_.bytes, other.storage_.bytes);
            other.ops_ = nullptr;
        }
        return *this;
    }

    ~AnyMeasurement()
    {
        if (ops_)
        {
            ops_->destroy(storage_.bytes);
            ops_ = nullptr;
        }
    }

    const MeasurementHeaderV1& header() const noexcept { return header_; }
    MeasurementKind kind() const noexcept { return header_.kind; }
    SourceId source() const noexcept { return header_.source; }

    template <typename T>
    const T* try_get() const noexcept
    {
        using U = std::decay_t<T>;
        (void)MeasurementKindOf<U>::value;

        if (!ops_ || ops_ != &OpsFor<U>())
        {
            return nullptr;
        }

        return reinterpret_cast<const U*>(storage_.bytes);
    }

    template <typename T>
    const T& get() const noexcept
    {
        const T* p = try_get<T>();
        assert(p && "AnyMeasurement::get<T>() wrong measurement type");
        return *p;
    }

    // -----------------------------------------------------------------
    // Serialization entry points (no exceptions, caller checks stream)
    // -----------------------------------------------------------------

    bool serialize_bds(BdsWriter& w) const noexcept
    {
        if (!ops_) return false;

        w << header_.rxTime;
        w << header_.eventTime;
        w << static_cast<std::uint16_t>(header_.kind);
        w << static_cast<std::uint16_t>(header_.source);
        w << header_.flags;

        if (!stream_ok(w)) return false;

        ops_->serialize_bds(w, storage_.bytes);
        return stream_ok(w);
    }

    static bool deserialize_bds(BdsReader& r, AnyMeasurement& out) noexcept
    {
        MeasurementHeaderV1 h{};
        std::uint16_t kind_u16 = 0;
        std::uint16_t src_u16  = 0;

        r >> h.rxTime;
        r >> h.eventTime;
        r >> kind_u16;
        r >> src_u16;
        r >> h.flags;

        if (!stream_ok(r)) return false;

        h.kind   = static_cast<MeasurementKind>(kind_u16);
        h.source = static_cast<SourceId>(src_u16);

        const Ops* ops = OpsForKind(h.kind);
        if (!ops) return false;

        out.destroy_if_needed();
        out.header_ = h;
        out.ops_ = ops;

        ops->deserialize_bds(r, out.storage_.bytes);
        return stream_ok(r);
    }

    bool serialize_nmea(NmeaWriter& w) const noexcept
    {
        if (!ops_) return false;

        w << header_.rxTime;
        w << header_.eventTime;
        w << static_cast<std::uint16_t>(header_.kind);
        w << static_cast<std::uint16_t>(header_.source);
        w << header_.flags;

        if (!stream_ok(w)) return false;

        ops_->serialize_nmea(w, storage_.bytes);
        return stream_ok(w);
    }

    static bool deserialize_nmea(NmeaReader& r, AnyMeasurement& out) noexcept
    {
        MeasurementHeaderV1 h{};
        std::uint16_t kind_u16 = 0;
        std::uint16_t src_u16  = 0;

        r >> h.rxTime;
        r >> h.eventTime;
        r >> kind_u16;
        r >> src_u16;
        r >> h.flags;

        if (!stream_ok(r)) return false;

        h.kind   = static_cast<MeasurementKind>(kind_u16);
        h.source = static_cast<SourceId>(src_u16);

        const Ops* ops = OpsForKind(h.kind);
        if (!ops) return false;

        out.destroy_if_needed();
        out.header_ = h;
        out.ops_ = ops;

        ops->deserialize_nmea(r, out.storage_.bytes);
        return stream_ok(r);
    }

private:
    struct Storage
    {
        alignas(std::max_align_t) std::byte bytes[SboSizeBytes];
    };

    void destroy_if_needed() noexcept
    {
        if (ops_)
        {
            ops_->destroy(storage_.bytes);
            ops_ = nullptr;
        }
    }

    template <typename S>
    static bool stream_ok(const S& s) noexcept
    {
        if constexpr (requires { s.good(); })
        {
            return s.good();
        }
        else if constexpr (requires { s.ok(); })
        {
            return s.ok();
        }
        else if constexpr (std::is_convertible<S, bool>::value)
        {
            return static_cast<bool>(s);
        }
        else
        {
            return true;
        }
    }

    // ---------------- minimal ops table (manual v-table) ----------------

    struct Ops
    {
        void (*destroy)(void*) noexcept;
        void (*copy_construct)(void*, const void*) noexcept;
        void (*move_construct)(void*, void*) noexcept;

        void (*serialize_bds)(BdsWriter&, const void*) noexcept;
        void (*deserialize_bds)(BdsReader&, void*) noexcept;

        void (*serialize_nmea)(NmeaWriter&, const void*) noexcept;
        void (*deserialize_nmea)(NmeaReader&, void*) noexcept;

        MeasurementKind kind;
        std::uint16_t measurement_size;
    };

    const Ops* ops_ = nullptr;

    template <typename T>
    static void destroy_fn(void* p) noexcept
    {
        reinterpret_cast<T*>(p)->~T();
    }

    template <typename T>
    static void copy_construct_fn(void* dst, const void* src) noexcept
    {
        new (dst) T(*reinterpret_cast<const T*>(src));
    }

    template <typename T>
    static void move_construct_fn(void* dst, void* src) noexcept
    {
        new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        reinterpret_cast<T*>(src)->~T();
    }

    template <typename T>
    static void serialize_bds_fn(BdsWriter& w, const void* src) noexcept
    {
        w << *reinterpret_cast<const T*>(src);
    }

    template <typename T>
    static void deserialize_bds_fn(BdsReader& r, void* dst) noexcept
    {
        T m{};
        r >> m;
        new (dst) T(std::move(m));
    }

    template <typename T>
    static void serialize_nmea_fn(NmeaWriter& w, const void* src) noexcept
    {
        w << *reinterpret_cast<const T*>(src);
    }

    template <typename T>
    static void deserialize_nmea_fn(NmeaReader& r, void* dst) noexcept
    {
        T m{};
        r >> m;
        new (dst) T(std::move(m));
    }

    template <typename T>
    static const Ops& OpsFor() noexcept
    {
        static const Ops ops{
            &destroy_fn<T>,
            &copy_construct_fn<T>,
            &move_construct_fn<T>,

            &serialize_bds_fn<T>,
            &deserialize_bds_fn<T>,

            &serialize_nmea_fn<T>,
            &deserialize_nmea_fn<T>,

            MeasurementKindOf<T>::value,
            static_cast<std::uint16_t>(sizeof(T))
        };
        return ops;
    }

    static const Ops* OpsForKind(MeasurementKind kind) noexcept
    {
        switch (kind)
        {
            case MeasurementKind::Temperature:        return &OpsFor<Temperature>();
            case MeasurementKind::BarometricPressure: return &OpsFor<BarometricPressure>();
            case MeasurementKind::Humidity:           return &OpsFor<Humidity>();
            case MeasurementKind::WindSpeed:          return &OpsFor<WindSpeed>();
            case MeasurementKind::WindDirection:      return &OpsFor<WindDirection>();
            case MeasurementKind::Precipitation:      return &OpsFor<Precipitation>();
            case MeasurementKind::Position:           return &OpsFor<Position>();
            default:
                assert(false && "Unknown MeasurementKind");
                return nullptr;
        }
    }

private:
    MeasurementHeaderV1 header_{};
    Storage storage_{};
};

} // namespace weather
