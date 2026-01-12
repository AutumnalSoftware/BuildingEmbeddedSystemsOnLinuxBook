#include "AnyMeasurement/MeasurementTypes.h"

#include <cctype>
#include <iostream>

namespace weather
{
    static bool iequals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
            if (ca != cb) return false;
        }
        return true;
    }

    std::string to_string(MeasurementKind kind)
    {
        switch (kind)
        {
            case MeasurementKind::Temperature: return "Temperature";
            case MeasurementKind::BarometricPressure: return "BarometricPressure";
            case MeasurementKind::Humidity: return "Humidity";
            case MeasurementKind::WindSpeed: return "WindSpeed";
            case MeasurementKind::WindDirection: return "WindDirection";
            case MeasurementKind::Precipitation: return "Precipitation";
            case MeasurementKind::Position: return "Position";
            default: return "Unknown";
        }
    }

    bool from_string(const std::string& s, MeasurementKind& out)
    {
        if (iequals(s, "Temperature")) { out = MeasurementKind::Temperature; return true; }
        if (iequals(s, "BarometricPressure")) { out = MeasurementKind::BarometricPressure; return true; }
        if (iequals(s, "Humidity")) { out = MeasurementKind::Humidity; return true; }
        if (iequals(s, "WindSpeed")) { out = MeasurementKind::WindSpeed; return true; }
        if (iequals(s, "WindDirection")) { out = MeasurementKind::WindDirection; return true; }
        if (iequals(s, "Precipitation")) { out = MeasurementKind::Precipitation; return true; }
        if (iequals(s, "Position")) { out = MeasurementKind::Position; return true; }
        return false;
    }

    std::ostream& operator<<(std::ostream& os, MeasurementKind kind)
    {
        os << to_string(kind);
        return os;
    }

    std::string to_string(SourceId src)
    {
        switch (src)
        {
            case SourceId::WeatherSensors: return "WeatherSensors";
            case SourceId::Gps: return "Gps";
            default: return "Unknown";
        }
    }

    bool from_string(const std::string& s, SourceId& out)
    {
        if (iequals(s, "WeatherSensors")) { out = SourceId::WeatherSensors; return true; }
        if (iequals(s, "Gps")) { out = SourceId::Gps; return true; }
        return false;
    }

    std::ostream& operator<<(std::ostream& os, SourceId src)
    {
        os << to_string(src);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const MeasurementHeaderV1& h)
    {
        os << "{rxTime=" << h.rxTime
           << " eventTime=" << h.eventTime
           << " kind=" << h.kind
           << " source=" << h.source
           << " flags=0x" << std::hex << h.flags << std::dec
           << "}";
        return os;
    }
} // namespace weather
