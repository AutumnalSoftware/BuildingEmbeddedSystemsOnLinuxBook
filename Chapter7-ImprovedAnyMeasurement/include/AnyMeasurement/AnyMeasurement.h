#pragma once

#include "AnyMeasurement/MeasurementTypes.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
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
        AnyMeasurement(const MeasurementHeaderV1& header, T payload)
            : header_(header)
        {
            using U = std::decay_t<T>;

            (void)MeasurementKindOf<U>::value;

            static_assert(std::is_copy_constructible<U>::value, "Payload must be copy-constructible.");
            static_assert(sizeof(U) <= SboSizeBytes, "Payload too large for AnyMeasurement SBO storage.");
            static_assert(alignof(U) <= alignof(Storage), "Payload alignment exceeds AnyMeasurement storage alignment.");

            vtable_ = &VTableFor<U>();
            new (storage_.bytes) U(std::move(payload));
        }

        AnyMeasurement(const AnyMeasurement& other)
            : header_(other.header_)
            , vtable_(other.vtable_)
        {
            if (!vtable_) { throw std::runtime_error("AnyMeasurement copy from invalid state"); }
            vtable_->copy_construct(storage_.bytes, other.storage_.bytes);
        }

        AnyMeasurement& operator=(const AnyMeasurement& other)
        {
            if (this == &other) return *this;

            if (vtable_) { vtable_->destroy(storage_.bytes); }

            header_ = other.header_;
            vtable_ = other.vtable_;
            vtable_->copy_construct(storage_.bytes, other.storage_.bytes);
            return *this;
        }

        AnyMeasurement(AnyMeasurement&& other) noexcept
            : header_(other.header_)
            , vtable_(other.vtable_)
        {
            if (vtable_)
            {
                vtable_->move_construct(storage_.bytes, other.storage_.bytes);
                other.vtable_ = nullptr;
            }
        }

        AnyMeasurement& operator=(AnyMeasurement&& other) noexcept
        {
            if (this == &other) return *this;

            if (vtable_) { vtable_->destroy(storage_.bytes); }

            header_ = other.header_;
            vtable_ = other.vtable_;
            if (vtable_)
            {
                vtable_->move_construct(storage_.bytes, other.storage_.bytes);
                other.vtable_ = nullptr;
            }
            return *this;
        }

        ~AnyMeasurement()
        {
            if (vtable_)
            {
                vtable_->destroy(storage_.bytes);
                vtable_ = nullptr;
            }
        }

        const MeasurementHeaderV1& header() const noexcept { return header_; }
        MeasurementKind kind() const noexcept { return header_.kind; }
        SourceId source() const noexcept { return header_.source; }

        template <typename T>
        const T& get() const
        {
            using U = std::decay_t<T>;
            if (!vtable_ || vtable_ != &VTableFor<U>())
            {
                throw std::bad_cast();
            }
            return *reinterpret_cast<const U*>(storage_.bytes);
        }

        void serialize(std::ostream& os) const
        {
            if (!vtable_) throw std::runtime_error("serialize on invalid AnyMeasurement");

            write_pod(os, header_.rxTime);
            write_pod(os, header_.eventTime);

            const std::uint16_t kind_u16 = static_cast<std::uint16_t>(header_.kind);
            const std::uint16_t src_u16  = static_cast<std::uint16_t>(header_.source);

            write_pod(os, kind_u16);
            write_pod(os, src_u16);
            write_pod(os, header_.flags);

            vtable_->serialize_payload(os, storage_.bytes);
        }

        static AnyMeasurement deserialize(std::istream& is)
        {
            MeasurementHeaderV1 h{};
            read_pod(is, h.rxTime);
            read_pod(is, h.eventTime);

            std::uint16_t kind_u16 = 0;
            std::uint16_t src_u16 = 0;
            read_pod(is, kind_u16);
            read_pod(is, src_u16);

            h.kind = static_cast<MeasurementKind>(kind_u16);
            h.source = static_cast<SourceId>(src_u16);

            read_pod(is, h.flags);

            const VTable& vt = VTableForKind(h.kind);
            AnyMeasurement m(h, vt);
            vt.deserialize_payload(is, m.storage_.bytes);
            return m;
        }

    private:
        struct Storage
        {
            alignas(std::max_align_t) std::byte bytes[SboSizeBytes];
        };

        struct VTable
        {
            void (*destroy)(void* p) noexcept;
            void (*copy_construct)(void* dst, const void* src);
            void (*move_construct)(void* dst, void* src) noexcept;
            void (*serialize_payload)(std::ostream& os, const void* src);
            void (*deserialize_payload)(std::istream& is, void* dst);

            MeasurementKind kind;
            std::uint16_t payload_size;
        };

        template <typename T>
        static void destroy_fn(void* p) noexcept { reinterpret_cast<T*>(p)->~T(); }

        template <typename T>
        static void copy_construct_fn(void* dst, const void* src) { new (dst) T(*reinterpret_cast<const T*>(src)); }

        template <typename T>
        static void move_construct_fn(void* dst, void* src) noexcept
        {
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
            reinterpret_cast<T*>(src)->~T();
        }

        template <typename T>
        static void serialize_payload_fn(std::ostream& os, const void* src)
        {
            const T& obj = *reinterpret_cast<const T*>(src);
            write_pod(os, obj);
        }

        template <typename T>
        static void deserialize_payload_fn(std::istream& is, void* dst)
        {
            T tmp{};
            read_pod(is, tmp);
            new (dst) T(std::move(tmp));
        }

        template <typename T>
struct VTableHolder
{
    static const VTable table;
};

template <typename T>
static const VTable& VTableFor()
{
    return VTableHolder<T>::table;
}

        static const VTable& VTableForKind(MeasurementKind kind)
        {
            switch (kind)
            {
                case MeasurementKind::Temperature: return VTableFor<Temperature>();
                case MeasurementKind::BarometricPressure: return VTableFor<BarometricPressure>();
                case MeasurementKind::Humidity: return VTableFor<Humidity>();
                case MeasurementKind::WindSpeed: return VTableFor<WindSpeed>();
                case MeasurementKind::WindDirection: return VTableFor<WindDirection>();
                case MeasurementKind::Precipitation: return VTableFor<Precipitation>();
                case MeasurementKind::Position: return VTableFor<Position>();
                default: throw std::runtime_error("Unknown MeasurementKind in VTableForKind");
            }
        }

        template <typename T>
        static void write_pod(std::ostream& os, const T& v)
        {
            static_assert(std::is_trivially_copyable<T>::value, "write_pod requires trivially copyable T");
            os.write(reinterpret_cast<const char*>(&v), static_cast<std::streamsize>(sizeof(T)));
            if (!os) throw std::runtime_error("write_pod failed");
        }

        template <typename T>
        static void read_pod(std::istream& is, T& v)
        {
            static_assert(std::is_trivially_copyable<T>::value, "read_pod requires trivially copyable T");
            is.read(reinterpret_cast<char*>(&v), static_cast<std::streamsize>(sizeof(T)));
            if (!is) throw std::runtime_error("read_pod failed");
        }

        AnyMeasurement(const MeasurementHeaderV1& header, const VTable& vt)
            : header_(header), vtable_(&vt)
        {
        }

    private:
        MeasurementHeaderV1 header_{};
        Storage storage_{};
        const VTable* vtable_ = nullptr;
    };


// -------------------------------------------------------------------------
// VTableHolder<T> definition (header-only, one per program via inline variable)
// -------------------------------------------------------------------------

template <typename T>
inline const AnyMeasurement::VTable AnyMeasurement::VTableHolder<T>::table{
    &AnyMeasurement::destroy_fn<T>,
    &AnyMeasurement::copy_construct_fn<T>,
    &AnyMeasurement::move_construct_fn<T>,
    &AnyMeasurement::serialize_payload_fn<T>,
    &AnyMeasurement::deserialize_payload_fn<T>,
    MeasurementKindOf<T>::value,
    static_cast<std::uint16_t>(sizeof(T))
};
} // namespace weather
