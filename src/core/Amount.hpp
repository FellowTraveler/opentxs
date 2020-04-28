// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace opentxs::implementation
{
using mp = boost::multiprecision;

class Amount : virtual public opentxs::Amount
{
public:
    bool operator==(const opentxs::Amount& rhs) const final;
    bool operator!=(const opentxs::Amount& rhs) const final;
    bool operator<(const opentxs::Amount& rhs) const final;
    bool operator>(const opentxs::Amount& rhs) const final;
    bool operator<=(const opentxs::Amount& rhs) const final;
    bool operator>=(const opentxs::Amount& rhs) const final;

    OTAmount operator+(const opentxs::Amount& obj) const final;
    OTAmount operator-(const opentxs::Amount& obj) const final;
    OTAmount operator*(const opentxs::Amount& obj) const final;
    OTAmount operator/(const opentxs::Amount& obj) const final;
    OTAmount operator%(const opentxs::Amount& obj) const final;

    Amount& operator+=(const opentxs::Amount& rhs) final;
    Amount& operator+=(const std::int8_t rhs) final;
    Amount& operator+=(const std::int16_t rhs) final;
    Amount& operator+=(const std::int32_t rhs) final;
    Amount& operator+=(const std::int64_t rhs) final;
    Amount& operator-=(const opentxs::Amount& rhs) final;
    Amount& operator-=(const std::int8_t rhs) final;
    Amount& operator-=(const std::int16_t rhs) final;
    Amount& operator-=(const std::int32_t rhs) final;
    Amount& operator-=(const std::int64_t rhs) final;
    Amount& operator*=(const opentxs::Amount& rhs) final;
    Amount& operator*=(const std::int8_t rhs) final;
    Amount& operator*=(const std::int16_t rhs) final;
    Amount& operator*=(const std::int32_t rhs) final;
    Amount& operator*=(const std::int64_t rhs) final;
    Amount& operator/=(const opentxs::Amount& rhs) final;
    Amount& operator/=(const std::int8_t rhs) final;
    Amount& operator/=(const std::int16_t rhs) final;
    Amount& operator/=(const std::int32_t rhs) final;
    Amount& operator/=(const std::int64_t rhs) final;

    void Assign(const opentxs::Amount& source) final;
    void Assign(const std::int64_t source) final;

    std::size_t size() const final { return data_.size(); }
    std::string str() const override;
    void swap(opentxs::Amount&& rhs) final;

    ~Amount() override = default;

    const mp::cpp_int& getValue() const { return value_; }

protected:
    mp::cpp_int value_;

    Amount() = default;

private:
    friend opentxs::Amount;

    Amount* clone() const override
    {
        auto pAmount = new implementation::Amount();
        *pAmount += *this;
        return pAmount;
    }

    Amount(const mp::cpp_int& value);
    Amount(const std::int8_t in);
    Amount(const std::int16_t in);
    Amount(const std::int32_t in);
    Amount(const std::int64_t in);
    Amount(const std::string in);
    
    Amount(const Amount& rhs) = delete;
    Amount(Amount&& rhs) = delete;
    Amount& operator=(const Amount& rhs) = delete;
    Amount& operator=(Amount&& rhs) = delete;
};
}  // namespace opentxs::implementation
