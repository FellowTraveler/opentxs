// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "internal/api/Api.hpp"

#include "opentxs/core/Log.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include "Amount.hpp"

namespace std
{
bool less<opentxs::Pimpl<opentxs::Amount>>::operator()(
    const opentxs::OTAmount& lhs,
    const opentxs::OTAmount& rhs) const
{
    return lhs.get() < rhs.get();
}
}  // namespace std

namespace opentxs
{

template <>
struct make_blank<OTAmount> {
    static OTAmount value(const api::Core& api)
    {
        return Amount::Factory();
//        return {api.Factory().Amount()};
    }
};

OTAmount Amount::Factory()
{
    return OTAmount(new implementation::Amount());
}

OTAmount Amount::Factory(const Amount& rhs)
{
    return OTAmount(new implementation::Amount(rhs.getValue()));
}

OTAmount Amount::Factory(const std::int8_t in)
{
    return OTAmount(new implementation::Amount(in));
}

OTAmount Amount::Factory(const std::int16_t in)
{
    return OTAmount(new implementation::Amount(in));
}

OTAmount Amount::Factory(const std::int32_t in)
{
    return OTAmount(new implementation::Amount(in));
}

OTAmount Amount::Factory(const std::int64_t in)
{
    return OTAmount(new implementation::Amount(in));
}

OTAmount Amount::Factory(const std::string in)
{
    return OTAmount(new implementation::Amount(in));
}

bool operator==(const OTAmount& lhs, const Amount& rhs) { return lhs.get() == rhs; }

bool operator!=(const OTAmount& lhs, const Amount& rhs) { return lhs.get() != rhs; }

bool operator<(const OTAmount& lhs, const Amount& rhs) { return lhs.get() < rhs; }

bool operator>(const OTAmount& lhs, const Amount& rhs) { return lhs.get() > rhs; }

bool operator<=(const OTAmount& lhs, const Amount& rhs) { return lhs.get() <= rhs; }

bool operator>=(const OTAmount& lhs, const Amount& rhs) { return lhs.get() >= rhs; }

OTAmount operator+(const OTAmount& lhs, const Amount& rhs)
{
//    return Factory::Amount(lhs.get() + rhs);

    return new implementation::Amount(lhs->getValue() + rhs->getValue());
}

OTAmount operator-(const OTAmount& lhs, const Amount& rhs)
{
//    return Factory::Amount(lhs.get() - rhs);
    return new implementation::Amount(lhs->getValue() - rhs->getValue());
}

OTAmount operator*(const OTAmount& lhs, const Amount& rhs)
{
//    return Factory::Amount(lhs.get() * rhs);
    return new implementation::Amount(lhs->getValue() * rhs->getValue());
}

OTAmount operator/(const OTAmount& lhs, const Amount& rhs)
{
//    return Factory::Amount(lhs.get() / rhs);
    return new implementation::Amount(lhs->getValue() / rhs->getValue());
}

OTAmount operator%(const OTAmount& lhs, const Amount& rhs)
{
//    return Factory::Amount(lhs.get() % rhs);
    return new implementation::Amount(lhs->getValue() % rhs->getValue());
}

OTAmount& operator+=(OTAmount& lhs, const Amount& rhs)
{
    lhs.get() += rhs;

    return lhs;
}

OTAmount& operator+=(OTAmount& lhs, const std::int8_t rhs)
{
    lhs.get() += rhs;

    return lhs;
}

OTAmount& operator+=(OTAmount& lhs, const std::int16_t rhs)
{
    lhs.get() += rhs;

    return lhs;
}

OTAmount& operator+=(OTAmount& lhs, const std::int32_t rhs)
{
    lhs.get() += rhs;

    return lhs;
}

OTAmount& operator+=(OTAmount& lhs, const std::int64_t rhs)
{
    lhs.get() += rhs;

    return lhs;
}

OTAmount& operator-=(OTAmount& lhs, const Amount& rhs)
{
    lhs.get() -= rhs;

    return lhs;
}

OTAmount& operator-=(OTAmount& lhs, const std::int8_t rhs)
{
    lhs.get() -= rhs;

    return lhs;
}

OTAmount& operator-=(OTAmount& lhs, const std::int16_t rhs)
{
    lhs.get() -= rhs;

    return lhs;
}

OTAmount& operator-=(OTAmount& lhs, const std::int32_t rhs)
{
    lhs.get() -= rhs;

    return lhs;
}

OTAmount& operator-=(OTAmount& lhs, const std::int64_t rhs)
{
    lhs.get() -= rhs;

    return lhs;
}

OTAmount& operator*=(OTAmount& lhs, const Amount& rhs)
{
    lhs.get() *= rhs;

    return lhs;
}

OTAmount& operator*=(OTAmount& lhs, const std::int8_t rhs)
{
    lhs.get() *= rhs;

    return lhs;
}

OTAmount& operator*=(OTAmount& lhs, const std::int16_t rhs)
{
    lhs.get() *= rhs;

    return lhs;
}

OTAmount& operator*=(OTAmount& lhs, const std::int32_t rhs)
{
    lhs.get() *= rhs;

    return lhs;
}

OTAmount& operator*=(OTAmount& lhs, const std::int64_t rhs)
{
    lhs.get() *= rhs;

    return lhs;
}

OTAmount& operator/=(OTAmount& lhs, const Amount& rhs)
{
    lhs.get() /= rhs;

    return lhs;
}

OTAmount& operator/=(OTAmount& lhs, const std::int8_t rhs)
{
    lhs.get() /= rhs;

    return lhs;
}

OTAmount& operator/=(OTAmount& lhs, const std::int16_t rhs)
{
    lhs.get() /= rhs;

    return lhs;
}

OTAmount& operator/=(OTAmount& lhs, const std::int32_t rhs)
{
    lhs.get() /= rhs;

    return lhs;
}

OTAmount& operator/=(OTAmount& lhs, const std::int64_t rhs)
{
    lhs.get() /= rhs;

    return lhs;
}


namespace implementation
{
Amount(const mp::cpp_int& value) : value_(value) { }
Amount(const std::int8_t in) : value_(in) { }
Amount(const std::int16_t in) : value_(in) { }
Amount(const std::int32_t in) : value_(in)  { }
Amount(const std::int64_t in) : value_(in)  { }
Amount(const std::string in) : value_(in)  { }

OTAmount Amount::operator+(const Amount& obj) const
{
    return new implementation::Amount(this->value_ + obj.getValue());
}

OTAmount Amount::operator-(const Amount& obj) const
{
    return new implementation::Amount(this->value_ - obj.getValue());
}

OTAmount Amount::operator*(const Amount& obj) const
{
    return new implementation::Amount(this->value_ * obj.getValue());
}

OTAmount Amount::operator/(const Amount& obj) const
{
    return new implementation::Amount(this->value_ / obj.getValue());
}

OTAmount Amount::operator%(const Amount& obj) const
{
    return new implementation::Amount(this->value_ % obj.getValue());
}

bool Amount::operator==(const opentxs::Amount& in) const
{
    return (this->value_ == in.getValue());
}

bool Amount::operator!=(const opentxs::Amount& in) const
{
    return (this->value_ != in.getValue());
}

bool Amount::operator<(const opentxs::Amount& in) const
{
    return (this->value_ < in.getValue());
}

bool Amount::operator>(const opentxs::Amount& in) const
{
    return (this->value_ > in.getValue());
}

bool Amount::operator<=(const opentxs::Amount& in) const
{
    return (this->value_ <= in.getValue());
}

bool Amount::operator>=(const opentxs::Amount& in) const
{
    return (this->value_ >= in.getValue());
}

Amount& Amount::operator+=(const opentxs::Amount& rhs)
{
    this->value_ += rhs.getValue();

    return *this;
}

Amount& Amount::operator+=(const std::int8_t rhs)
{
    this->value_ += mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator+=(const std::int16_t rhs)
{
    this->value_ += mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator+=(const std::int32_t rhs)
{
    this->value_ += mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator+=(const std::int64_t rhs)
{
    this->value_ += mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator-=(const opentxs::Amount& rhs)
{
    this->value_ -= rhs.getValue();

    return *this;
}

Amount& Amount::operator-=(const std::int8_t rhs)
{
    this->value_ -= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator-=(const std::int16_t rhs)
{
    this->value_ -= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator-=(const std::int32_t rhs)
{
    this->value_ -= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator-=(const std::int64_t rhs)
{
    this->value_ -= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator*=(const opentxs::Amount& rhs)
{
    this->value_ *= rhs.getValue();

    return *this;
}

Amount& Amount::operator*=(const std::int8_t rhs)
{
    this->value_ *= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator*=(const std::int16_t rhs)
{
    this->value_ *= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator*=(const std::int32_t rhs)
{
    this->value_ *= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator*=(const std::int64_t rhs)
{
    this->value_ *= mp::cpp_int(rhs);

    return *this;
}


Amount& Amount::operator/=(const opentxs::Amount& rhs)
{
    this->value_ /= rhs.getValue();

    return *this;
}

Amount& Amount::operator/=(const std::int8_t rhs)
{
    this->value_ /= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator/=(const std::int16_t rhs)
{
    this->value_ /= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator/=(const std::int32_t rhs)
{
    this->value_ /= mp::cpp_int(rhs);

    return *this;
}

Amount& Amount::operator/=(const std::int64_t rhs)
{
    this->value_ /= mp::cpp_int(rhs);

    return *this;
}

void Amount::Assign(const opentxs::Amount& rhs)
{
    // can't assign to self.
    if (&dynamic_cast<const Amount&>(rhs) == this) { return; }

    this->value_ = rhs.getValue();
}

void Amount::Assign(const std::int64_t rhs)
{
    this->value_ = rhs;
}

std::size_t Amount::size() const
{
    return this->value_.size();
}

std::string Amount::str() const
{
    return this->value_.str();
}

void Amount::swap(opentxs::Amount&& rhs)
{
    auto& in = dynamic_cast<Amount&>(rhs);
    std::swap(this->value_, in.getValue());
}

}  // namespace implementation
}  // namespace opentxs
