// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_AMOUNT_HPP
#define OPENTXS_CORE_AMOUNT_HPP

#include "opentxs/Forward.hpp"

#include <cstdint>
#include <string>
#include <vector>

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::Amount>::Pimpl(opentxs::Amount const &);
%ignore opentxs::Pimpl<opentxs::Amount>::operator opentxs::Amount&;
%ignore opentxs::Pimpl<opentxs::Amount>::operator const opentxs::Amount &;
%ignore operator==(OTAmount& lhs, const Amount& rhs);
%ignore operator!=(OTAmount& lhs, const Amount& rhs);
%ignore operator+=(OTAmount& lhs, const Amount& rhs);
%ignore operator-=(OTAmount& lhs, const Amount& rhs);
%ignore operator*=(OTAmount& lhs, const Amount& rhs);
%ignore operator/=(OTAmount& lhs, const Amount& rhs);
%rename(amountCompareEqual) opentxs::Amount::operator==(const Amount& rhs) const;
%rename(amountCompareNotEqual) opentxs::Amount::operator!=(const Amount& rhs) const;
%rename(amountPlusEqual) opentxs::Amount::operator+=(const Amount& rhs);
%rename(amountMinusEqual) opentxs::Amount::operator-=(const Amount& rhs);
%rename(amountTimesEqual) opentxs::Amount::operator*=(const Amount& rhs);
%rename(amountDivideEqual) opentxs::Amount::operator/=(const Amount& rhs);
%rename(assign) operator=(const opentxs::Amount&);
%rename (AmountFactory) opentxs::Amount::Factory;
%template(OTAmount) opentxs::Pimpl<opentxs::Amount>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
using OTAmount = Pimpl<Amount>;

OPENTXS_EXPORT bool operator==(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT bool operator!=(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT bool operator<(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT bool operator>(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT bool operator<=(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT bool operator>=(const OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT OTAmount& operator+=(OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT OTAmount& operator+=(OTAmount& lhs, const std::int8_t rhs);
OPENTXS_EXPORT OTAmount& operator+=(OTAmount& lhs, const std::int16_t rhs);
OPENTXS_EXPORT OTAmount& operator+=(OTAmount& lhs, const std::int32_t rhs);
OPENTXS_EXPORT OTAmount& operator+=(OTAmount& lhs, const std::int64_t rhs);
OPENTXS_EXPORT OTAmount& operator-=(OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT OTAmount& operator-=(OTAmount& lhs, const std::int8_t rhs);
OPENTXS_EXPORT OTAmount& operator-=(OTAmount& lhs, const std::int16_t rhs);
OPENTXS_EXPORT OTAmount& operator-=(OTAmount& lhs, const std::int32_t rhs);
OPENTXS_EXPORT OTAmount& operator-=(OTAmount& lhs, const std::int64_t rhs);
OPENTXS_EXPORT OTAmount& operator*=(OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT OTAmount& operator*=(OTAmount& lhs, const std::int8_t rhs);
OPENTXS_EXPORT OTAmount& operator*=(OTAmount& lhs, const std::int16_t rhs);
OPENTXS_EXPORT OTAmount& operator*=(OTAmount& lhs, const std::int32_t rhs);
OPENTXS_EXPORT OTAmount& operator*=(OTAmount& lhs, const std::int64_t rhs);
OPENTXS_EXPORT OTAmount& operator/=(OTAmount& lhs, const Amount& rhs);
OPENTXS_EXPORT OTAmount& operator/=(OTAmount& lhs, const std::int8_t rhs);
OPENTXS_EXPORT OTAmount& operator/=(OTAmount& lhs, const std::int16_t rhs);
OPENTXS_EXPORT OTAmount& operator/=(OTAmount& lhs, const std::int32_t rhs);
OPENTXS_EXPORT OTAmount& operator/=(OTAmount& lhs, const std::int64_t rhs);

class Amount
{
public:
    OPENTXS_EXPORT static Pimpl<opentxs::Amount> Factory();
    OPENTXS_EXPORT static Pimpl<opentxs::Amount> Factory(const Amount& rhs);
#ifndef SWIG
    OPENTXS_EXPORT static OTAmount Factory(const std::int8_t in);
    OPENTXS_EXPORT static OTAmount Factory(const std::int16_t in);
    OPENTXS_EXPORT static OTAmount Factory(const std::int32_t in);
    OPENTXS_EXPORT static OTAmount Factory(const std::int64_t in);
    OPENTXS_EXPORT static OTAmount Factory(const std::string in);
#endif

    OPENTXS_EXPORT virtual bool operator==(const Amount& rhs) const = 0;
    OPENTXS_EXPORT virtual bool operator!=(const Amount& rhs) const = 0;
    OPENTXS_EXPORT virtual bool operator<(const Amount& rhs) const = 0;
    OPENTXS_EXPORT virtual bool operator>(const Amount& rhs) const = 0;
    OPENTXS_EXPORT virtual bool operator<=(const Amount& rhs) const = 0;
    OPENTXS_EXPORT virtual bool operator>=(const Amount& rhs) const = 0;

    OPENTXS_EXPORT virtual OTAmount operator+(const Amount& obj) const = 0;
    OPENTXS_EXPORT virtual OTAmount operator-(const Amount& obj) const = 0;
    OPENTXS_EXPORT virtual OTAmount operator*(const Amount& obj) const = 0;
    OPENTXS_EXPORT virtual OTAmount operator/(const Amount& obj) const = 0;
    OPENTXS_EXPORT virtual OTAmount operator%(const Amount& obj) const = 0;

    OPENTXS_EXPORT virtual Amount& operator+=(const Amount& rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator+=(const std::int8_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator+=(const std::int16_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator+=(const std::int32_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator+=(const std::int64_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator-=(const Amount& rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator-=(const std::int8_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator-=(const std::int16_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator-=(const std::int32_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator-=(const std::int64_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator*=(const Amount& rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator*=(const std::int8_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator*=(const std::int16_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator*=(const std::int32_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator*=(const std::int64_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator/=(const Amount& rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator/=(const std::int8_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator/=(const std::int16_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator/=(const std::int32_t rhs) = 0;
    OPENTXS_EXPORT virtual Amount& operator/=(const std::int64_t rhs) = 0;

    OPENTXS_EXPORT virtual void Assign(const Amount& source) = 0;
    OPENTXS_EXPORT virtual void Assign(const std::int64_t source) = 0;

    OPENTXS_EXPORT virtual std::size_t size() const = 0;
    OPENTXS_EXPORT virtual std::string str() const = 0;
    OPENTXS_EXPORT virtual void swap(Amount&& rhs) = 0;

    OPENTXS_EXPORT virtual ~Amount() = default;

protected:
    Amount() = default;

private:
    friend OTAmount;

#ifdef _WIN32
public:
#endif
    OPENTXS_EXPORT virtual Amount* clone() const = 0;
#ifdef _WIN32
private:
#endif

    Amount(const Amount& rhs) = delete;
    Amount(Amount&& rhs) = delete;
    Amount& operator=(const Amount& rhs) = delete;
    Amount& operator=(Amount&& rhs) = delete;
};
}  // namespace opentxs

#ifndef SWIG
namespace std
{
template <>
struct less<opentxs::OTAmount> {
    OPENTXS_EXPORT bool operator()(
        const opentxs::OTAmount& lhs,
        const opentxs::OTAmount& rhs) const;
};
}  // namespace std
#endif
#endif // OPENTXS_CORE_AMOUNT_HPP
