#include "SensorContext.h"
#include <cassert>

namespace weather
{
static void unconfigured_destroy(void*) noexcept {}
static void unconfigured_move(void*, void*) noexcept { assert(false); }
static int unconfigured_fd(const void*) noexcept { assert(false); return -1; }
static Status unconfigured_read(void*) noexcept { assert(false); return Status::Ok(); }

static const SensorContext::Ops& UnconfiguredOps() noexcept
{
    static const SensorContext::Ops ops{
        &unconfigured_destroy,
        &unconfigured_move,
        &unconfigured_fd,
        &unconfigured_read
    };
    return ops;
}

SensorContext::SensorContext() noexcept : mOps(&UnconfiguredOps()) {}
SensorContext::~SensorContext() { destroy_to_unconfigured(); }

SensorContext::SensorContext(SensorContext&& other) noexcept : mOps(other.mOps)
{
    mOps->move_construct(mStorage.bytes, other.mStorage.bytes);
    other.mOps = &UnconfiguredOps();
}

SensorContext& SensorContext::operator=(SensorContext&& other) noexcept
{
    if (this != &other)
    {
        destroy_to_unconfigured();
        mOps = other.mOps;
        mOps->move_construct(mStorage.bytes, other.mStorage.bytes);
        other.mOps = &UnconfiguredOps();
    }
    return *this;
}

int SensorContext::fd() const noexcept
{
    return mOps->get_fd(mStorage.bytes);
}

Status SensorContext::on_readable() noexcept
{
    return mOps->on_readable(mStorage.bytes);
}

void SensorContext::destroy_to_unconfigured() noexcept
{
    mOps->destroy(mStorage.bytes);
    mOps = &UnconfiguredOps();
}
}
