#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

/**
 * @brief Immutable, non-owning view of a contiguous byte sequence.
 *
 * ByteView does not own the memory it references. It is a lightweight
 * `(pointer, size)` wrapper intended to express read-only access to
 * arbitrary binary data.
 *
 * This type is conceptually similar to `std::span<const std::byte>`,
 * but is available in C++17.
 *
 * Lifetime:
 *  - The caller is responsible for ensuring the referenced memory
 *    remains valid for the lifetime of the ByteView.
 *
 * Thread safety:
 *  - ByteView itself is trivially copyable and thread-safe.
 *  - The underlying memory is not synchronized.
 */
class ByteView
{
public:
    /// Constructs an empty ByteView.
    ByteView() noexcept
        : mData(nullptr)
        , mSize(0)
    {
    }

    /**
     * @brief Constructs a ByteView from raw memory.
     *
     * @param data Pointer to the first byte (may be nullptr if size == 0).
     * @param size Number of bytes in the view.
     */
    ByteView(const void* data, std::size_t size) noexcept
        : mData(static_cast<const std::byte*>(data))
        , mSize(size)
    {
    }

    /// @return Pointer to the first byte (read-only).
    const std::byte* data() const noexcept { return mData; }

    /// @return Number of bytes in the view.
    std::size_t size() const noexcept { return mSize; }

    /// @return True if the view contains no bytes.
    bool empty() const noexcept { return mSize == 0; }

    /// Read-only indexed access (no bounds checking).
    const std::byte& operator[](std::size_t i) const noexcept { return mData[i]; }

private:
    const std::byte* mData;
    std::size_t mSize;
};

/**
 * @brief Mutable, non-owning view of a contiguous byte sequence.
 *
 * MutableByteView expresses intent to modify the underlying memory.
 * It is otherwise identical to ByteView in structure and lifetime
 * requirements.
 *
 * MutableByteView is implicitly convertible to ByteView to support
 * APIs that require read-only access.
 */
class MutableByteView
{
public:
    /// Constructs an empty MutableByteView.
    MutableByteView() noexcept
        : mData(nullptr)
        , mSize(0)
    {
    }

    /**
     * @brief Constructs a MutableByteView from raw memory.
     *
     * @param data Pointer to the first byte (may be nullptr if size == 0).
     * @param size Number of bytes in the view.
     */
    MutableByteView(void* data, std::size_t size) noexcept
        : mData(static_cast<std::byte*>(data))
        , mSize(size)
    {
    }

    /// @return Pointer to the first byte (mutable).
    std::byte* data() const noexcept { return mData; }

    /// @return Number of bytes in the view.
    std::size_t size() const noexcept { return mSize; }

    /// @return True if the view contains no bytes.
    bool empty() const noexcept { return mSize == 0; }

    /// Mutable indexed access (no bounds checking).
    std::byte& operator[](std::size_t i) const noexcept { return mData[i]; }

    /**
     * @brief Implicit conversion to an immutable ByteView.
     *
     * Allows MutableByteView to be passed to read-only APIs
     * without copying or allocation.
     */
    operator ByteView() const noexcept
    {
        return ByteView(mData, mSize);
    }

private:
    std::byte* mData;
    std::size_t mSize;
};

// -----------------------------------------------------------------------------
// Convenience helpers.
// -----------------------------------------------------------------------------

/// Create an immutable ByteView from raw memory.
inline ByteView asBytes(const void* data, std::size_t size) noexcept
{
    return ByteView(data, size);
}

/// Create a mutable ByteView from raw memory.
inline MutableByteView asWritableBytes(void* data, std::size_t size) noexcept
{
    return MutableByteView(data, size);
}

/// Create an immutable ByteView from a std::array.
template<typename T, std::size_t N>
inline ByteView asBytes(const std::array<T, N>& a) noexcept
{
    static_assert(std::is_trivial<T>::value,
                  "asBytes(std::array<...>) requires a trivial element type");
    return ByteView(a.data(), N * sizeof(T));
}

/// Create a mutable ByteView from a std::array.
template<typename T, std::size_t N>
inline MutableByteView asWritableBytes(std::array<T, N>& a) noexcept
{
    static_assert(std::is_trivial<T>::value,
                  "asWritableBytes(std::array<...>) requires a trivial element type");
    return MutableByteView(a.data(), N * sizeof(T));
}
