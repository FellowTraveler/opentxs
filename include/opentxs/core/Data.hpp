// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_DATA_HPP
#define OPENTXS_CORE_DATA_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>
#include <vector>

#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/iterator/Bidirectional.hpp"

#ifdef SWIG
// clang-format off
%ignore opentxs::Data::at;
%ignore opentxs::Data::begin;
%ignore opentxs::Data::cbegin;
%ignore opentxs::Data::cend;
%ignore opentxs::Data::Concatenate;
%ignore opentxs::Data::end;
%ignore opentxs::Data::GetPointer;
%ignore opentxs::Data::OTfread;
%ignore opentxs::Pimpl<opentxs::Data>::Pimpl(opentxs::Data const &);
%ignore opentxs::Pimpl<opentxs::Data>::operator opentxs::Data&;
%ignore opentxs::Pimpl<opentxs::Data>::operator const opentxs::Data &;
%ignore operator==(OTData& lhs, const Data& rhs);
%ignore operator!=(OTData& lhs, const Data& rhs);
%ignore operator+=(OTData& lhs, const OTData& rhs);
%rename(dataCompareEqual) opentxs::Data::operator==(const Data& rhs) const;
%rename(dataCompareNotEqual) opentxs::Data::operator!=(const Data& rhs) const;
%rename(dataPlusEqual) opentxs::Data::operator+=(const Data& rhs);
%rename(assign) operator=(const opentxs::Data&);
%rename (DataFactory) opentxs::Data::Factory;
%template(OTData) opentxs::Pimpl<opentxs::Data>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Frame;
}  // namespace zeromq
}  // namespace network

class Armored;
class Data;

using OTData = Pimpl<Data>;
}  // namespace opentxs

namespace opentxs
{
OPENTXS_EXPORT bool operator==(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT bool operator!=(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT bool operator<(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT bool operator>(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT bool operator<=(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT bool operator>=(const OTData& lhs, const Data& rhs) noexcept;
OPENTXS_EXPORT OTData& operator+=(OTData& lhs, const OTData& rhs);
OPENTXS_EXPORT OTData& operator+=(OTData& lhs, const std::uint8_t rhs);
OPENTXS_EXPORT OTData& operator+=(OTData& lhs, const std::uint16_t rhs);
OPENTXS_EXPORT OTData& operator+=(OTData& lhs, const std::uint32_t rhs);
OPENTXS_EXPORT OTData& operator+=(OTData& lhs, const std::uint64_t rhs);
}  // namespace opentxs

namespace opentxs
{
class OPENTXS_EXPORT Data
{
public:
    enum class Mode : bool { Hex = true, Raw = false };

    using iterator = opentxs::iterator::Bidirectional<Data, std::byte>;
    using const_iterator =
        opentxs::iterator::Bidirectional<const Data, const std::byte>;

    static Pimpl<opentxs::Data> Factory();
    static Pimpl<opentxs::Data> Factory(const Data& rhs);
#ifndef SWIG
    static Pimpl<opentxs::Data> Factory(const void* data, std::size_t size);
    static OTData Factory(const Armored& source);
    static OTData Factory(const std::vector<unsigned char>& source);
    static OTData Factory(const std::vector<std::byte>& source);
    static OTData Factory(const network::zeromq::Frame& message);
    static OTData Factory(const std::uint8_t in);
    /// Bytes will be stored in big endian order
    static OTData Factory(const std::uint16_t in);
    /// Bytes will be stored in big endian order
    static OTData Factory(const std::uint32_t in);
    /// Bytes will be stored in big endian order
    static OTData Factory(const std::uint64_t in);
    static OTData Factory(const std::string in, const Mode mode);
#endif

    virtual bool operator==(const Data& rhs) const noexcept = 0;
    virtual bool operator!=(const Data& rhs) const noexcept = 0;
    virtual bool operator<(const Data& rhs) const noexcept = 0;
    virtual bool operator>(const Data& rhs) const noexcept = 0;
    virtual bool operator<=(const Data& rhs) const noexcept = 0;
    virtual bool operator>=(const Data& rhs) const noexcept = 0;
    virtual std::string asHex() const = 0;
    virtual const std::byte& at(const std::size_t position) const = 0;
    virtual const_iterator begin() const = 0;
    virtual ReadView Bytes() const noexcept = 0;
    virtual const_iterator cbegin() const = 0;
    virtual const_iterator cend() const = 0;
    virtual const void* data() const = 0;
    virtual bool empty() const = 0;
    virtual const_iterator end() const = 0;
    virtual bool Extract(
        const std::size_t amount,
        Data& output,
        const std::size_t pos = 0) const = 0;
    virtual bool Extract(std::uint8_t& output, const std::size_t pos = 0)
        const = 0;
    virtual bool Extract(std::uint16_t& output, const std::size_t pos = 0)
        const = 0;
    virtual bool Extract(std::uint32_t& output, const std::size_t pos = 0)
        const = 0;
    virtual bool Extract(std::uint64_t& output, const std::size_t pos = 0)
        const = 0;
#ifndef SWIG
    [[deprecated]] virtual const void* GetPointer() const = 0;
#endif
#ifndef SWIG
    [[deprecated]] virtual std::size_t GetSize() const = 0;
#endif
#ifndef SWIG
    [[deprecated]] virtual bool IsEmpty() const = 0;
#endif
    virtual bool IsNull() const = 0;
    virtual std::size_t size() const = 0;
    virtual std::string str() const = 0;

    virtual Data& operator+=(const Data& rhs) = 0;
    virtual Data& operator+=(const std::uint8_t rhs) = 0;
    /// Bytes will be stored in big endian order
    virtual Data& operator+=(const std::uint16_t rhs) = 0;
    /// Bytes will be stored in big endian order
    virtual Data& operator+=(const std::uint32_t rhs) = 0;
    /// Bytes will be stored in big endian order
    virtual Data& operator+=(const std::uint64_t rhs) = 0;
    virtual void Assign(const Data& source) = 0;
    virtual void Assign(const ReadView source) = 0;
#ifndef SWIG
    virtual void Assign(const void* data, const std::size_t& size) = 0;
#endif
    virtual std::byte& at(const std::size_t position) = 0;
    virtual iterator begin() = 0;
#ifndef SWIG
    virtual void* data() = 0;
    virtual bool DecodeHex(const std::string& hex) = 0;
#endif
    virtual void Concatenate(const ReadView data) = 0;
#ifndef SWIG
    virtual void Concatenate(const void* data, const std::size_t& size) = 0;
#endif
    virtual iterator end() = 0;
    virtual bool Randomize(const std::size_t& size) = 0;
    virtual void Release() = 0;
    virtual void resize(const std::size_t size) = 0;
    virtual void SetSize(const std::size_t size) = 0;
    virtual void swap(Data&& rhs) = 0;
    virtual AllocateOutput WriteInto() noexcept = 0;
    virtual void zeroMemory() = 0;

    virtual ~Data() = default;

protected:
    Data() = default;

private:
    friend OTData;

#ifdef _WIN32
public:
#endif
    virtual Data* clone() const = 0;
#ifdef _WIN32
private:
#endif

    Data(const Data& rhs) = delete;
    Data(Data&& rhs) = delete;
    Data& operator=(const Data& rhs) = delete;
    Data& operator=(Data&& rhs) = delete;
};
}  // namespace opentxs

#ifndef SWIG
namespace std
{
template <>
struct OPENTXS_EXPORT less<opentxs::OTData> {
    bool operator()(const opentxs::OTData& lhs, const opentxs::OTData& rhs)
        const;
};
}  // namespace std
#endif
#endif
