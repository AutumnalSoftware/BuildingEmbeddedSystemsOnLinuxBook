#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// ---------- Enums and helpers ----------

enum class MeasurementType
{
    Temperature,
    Pressure,
    Position,
    Unknown
};

enum class Quality
{
    Good,
    Suspect,
    Bad,
    Unknown
};

const char* to_string(MeasurementType v)
{
    switch (v)
    {
        case MeasurementType::Temperature: return "Temperature";
        case MeasurementType::Pressure:    return "Pressure";
        case MeasurementType::Position:    return "Position";
        default:                           return "Unknown";
    }
}

const char* to_string(Quality v)
{
    switch (v)
    {
        case Quality::Good:    return "Good";
        case Quality::Suspect: return "Suspect";
        case Quality::Bad:     return "Bad";
        default:               return "Unknown";
    }
}

std::ostream& operator<<(std::ostream& os, MeasurementType v)
{
    return os << to_string(v);
}

std::ostream& operator<<(std::ostream& os, Quality v)
{
    return os << to_string(v);
}

// ---------- Measurement header ----------

struct MeasurementHeader
{
    std::string name;              // e.g. "outdoor_temp", "baro_pressure"
    std::string units;             // e.g. "C", "kPa", "%", "deg"
    std::uint64_t timestampUs = 0; // microseconds since epoch (or other agreed timebase)

    MeasurementType type = MeasurementType::Unknown;
    Quality quality = Quality::Unknown;
};

// ---------- Example concrete measurements ----------

struct Temperature
{
    double celsius = 0.0;
};

struct Pressure
{
    double kpa = 0.0;
};

struct Position
{
    double latitude = 0.0;
    double longitude = 0.0;
};

std::ostream& operator<<(std::ostream& os, const Temperature& t)
{
    return os << t.celsius;
}

std::ostream& operator<<(std::ostream& os, const Pressure& p)
{
    return os << p.kpa;
}

std::ostream& operator<<(std::ostream& os, const Position& p)
{
    return os << "lat=" << p.latitude << " lon=" << p.longitude;
}

// ---------- AnyMeasurement (pure-virtual model, heap-only) ----------

class AnyMeasurement
{
public:
    AnyMeasurement() = default;

    AnyMeasurement(const AnyMeasurement& other)
        : m_header(other.m_header)
    {
        if (other.m_model)
        {
            m_model = other.m_model->clone();
        }
    }

    AnyMeasurement& operator=(const AnyMeasurement& other)
    {
        if (this == &other)
        {
            return *this;
        }

        AnyMeasurement tmp(other);
        swap(tmp);
        return *this;
    }

    AnyMeasurement(AnyMeasurement&&) noexcept = default;
    AnyMeasurement& operator=(AnyMeasurement&&) noexcept = default;

    bool has_value() const { return static_cast<bool>(m_model); }

    const MeasurementHeader& header() const { return m_header; }
    MeasurementHeader& header() { return m_header; }

    void clear()
    {
        m_model.reset();
        m_header = {};
    }

    void swap(AnyMeasurement& other) noexcept
    {
        using std::swap;
        swap(m_header, other.m_header);
        swap(m_model, other.m_model);
    }

    friend std::ostream& operator<<(std::ostream& os, const AnyMeasurement& m)
    {
        os << "name=" << m.m_header.name
           << " units=" << m.m_header.units
           << " ts_us=" << m.m_header.timestampUs
           << " type=" << m.m_header.type
           << " quality=" << m.m_header.quality;

        if (m.m_model)
        {
            os << " value=";
            m.m_model->print(os);
            os << " " << m.m_header.units;
        }
        else
        {
            os << " value=<empty>";
        }

        return os;
    }

    template <typename T>
    static AnyMeasurement make(MeasurementHeader header, T value)
    {
        AnyMeasurement out;
        out.m_header = std::move(header);
        out.m_model = std::unique_ptr<IMeasurementModel>(
            new Model<T>(std::move(value))
        );
        return out;
    }

private:
    struct IMeasurementModel
    {
        virtual ~IMeasurementModel() = default;
        virtual std::unique_ptr<IMeasurementModel> clone() const = 0;
        virtual void print(std::ostream& os) const = 0;
    };

    template <typename T>
    struct Model final : IMeasurementModel
    {
        explicit Model(T v) : value(std::move(v)) {}

        std::unique_ptr<IMeasurementModel> clone() const override
        {
            return std::unique_ptr<IMeasurementModel>(
                new Model<T>(value)
            );
        }

        void print(std::ostream& os) const override
        {
            os << value;
        }

        T value;
    };

private:
    MeasurementHeader m_header{};
    std::unique_ptr<IMeasurementModel> m_model{};
};

// ---------- Demo main() ----------

int main()
{
    MeasurementHeader tempHeader;
    tempHeader.name = "outdoor_temp";
    tempHeader.units = "C";
    tempHeader.timestampUs = 1736539200000000ULL; // demo number
    tempHeader.type = MeasurementType::Temperature;
    tempHeader.quality = Quality::Good;

    MeasurementHeader pressureHeader;
    pressureHeader.name = "baro_pressure";
    pressureHeader.units = "kPa";
    pressureHeader.timestampUs = 1736539200100000ULL;
    pressureHeader.type = MeasurementType::Pressure;
    pressureHeader.quality = Quality::Good;

    MeasurementHeader posHeader;
    posHeader.name = "gps_fix";
    posHeader.units = "deg";
    posHeader.timestampUs = 1736539200123456ULL;
    posHeader.type = MeasurementType::Position;
    posHeader.quality = Quality::Suspect;

    AnyMeasurement temp =
        AnyMeasurement::make(tempHeader, Temperature{23.5});

    AnyMeasurement pressure =
        AnyMeasurement::make(pressureHeader, Pressure{101.3});

    AnyMeasurement pos =
        AnyMeasurement::make(posHeader, Position{42.93, -77.54});

    std::cout << temp << "\n";
    std::cout << pressure << "\n";
    std::cout << pos << "\n";

    // Copy semantics
    AnyMeasurement copy = temp;
    std::cout << "copy:  " << copy << "\n";

    // Move semantics
    AnyMeasurement moved = std::move(pressure);
    std::cout << "moved: " << moved << "\n";

    // The moved-from object is valid but unspecified. We'll just show it.
    std::cout << "after move, pressure: " << pressure << "\n";

    return 0;
}

