// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace opentxs::implementation
{
class Identifier final : virtual public opentxs::Identifier,
                         virtual public opentxs::identifier::Nym,
                         virtual public opentxs::identifier::Server,
                         virtual public opentxs::identifier::UnitDefinition,
                         public Data
{
public:
    using ot_super = Data;

    using ot_super::operator==;
    bool operator==(const opentxs::Identifier& rhs) const final;
    using ot_super::operator!=;
    bool operator!=(const opentxs::Identifier& rhs) const final;
    using ot_super::operator>;
    bool operator>(const opentxs::Identifier& rhs) const final;
    using ot_super::operator<;
    bool operator<(const opentxs::Identifier& rhs) const final;
    using ot_super::operator<=;
    bool operator<=(const opentxs::Identifier& rhs) const final;
    using ot_super::operator>=;
    bool operator>=(const opentxs::Identifier& rhs) const final;

    void GetString(String& theStr) const final;
    std::string str() const final;
    const ID& Type() const final { return type_; }

    bool CalculateDigest(const ReadView bytes, const ID type = DefaultType)
        final;
    void SetString(const std::string& encoded) final;
    void SetString(const String& encoded) final;
    using ot_super::swap;
    void swap(opentxs::Identifier& rhs) final;

    ~Identifier() final = default;

private:
    friend opentxs::Identifier;
    friend opentxs::identifier::Nym;
    friend opentxs::identifier::Server;
    friend opentxs::identifier::UnitDefinition;

    static const ID DefaultType{ID::blake2b};
    static const std::size_t MinimumSize{10};

    ID type_{DefaultType};

    Identifier* clone() const final;

    static Identifier* contract_contents_to_identifier(const Contract& in);
    static Identifier* voucher_contents_to_identifier(const Cheque& in);
    static proto::HashType IDToHashType(const ID type);
    static OTData path_to_data(
        const proto::ContactItemType type,
        const proto::HDPath& path);

    explicit Identifier(const std::string& rhs);
    explicit Identifier(const String& rhs);
    explicit Identifier(const identity::Nym& nym);
    explicit Identifier(const Contract& contract);
    explicit Identifier(
        const Vector& data,
        const std::size_t size,
        const ID type);
    Identifier(const proto::ContactItemType type, const proto::HDPath& path);
    Identifier();
    Identifier(const Identifier& rhs) = delete;
    Identifier(Identifier&& rhs) = delete;
    Identifier& operator=(const Identifier& rhs) = delete;
    Identifier& operator=(Identifier&& rhs) = delete;
};
}  // namespace opentxs::implementation
