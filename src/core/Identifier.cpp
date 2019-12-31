// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/OT.hpp"

#include "Data.hpp"

#include <set>
#include <map>

#include "Identifier.hpp"

template class opentxs::Pimpl<opentxs::Identifier>;
template class std::set<opentxs::OTIdentifier>;
template class std::map<opentxs::OTIdentifier, std::set<opentxs::OTIdentifier>>;

namespace std
{
bool less<opentxs::Pimpl<opentxs::Identifier>>::operator()(
    const opentxs::OTIdentifier& lhs,
    const opentxs::OTIdentifier& rhs) const
{
    return lhs.get() < rhs.get();
}
}  // namespace std

namespace opentxs
{
bool operator==(const OTIdentifier& lhs, const Identifier& rhs)
{
    return lhs.get().operator==(rhs);
}

bool operator==(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator==(rhs);
}

bool operator==(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator==(rhs);
}

bool operator==(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator==(rhs);
}

bool operator!=(const OTIdentifier& lhs, const Identifier& rhs)
{
    return lhs.get().operator!=(rhs);
}

bool operator!=(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator!=(rhs);
}

bool operator!=(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator!=(rhs);
}

bool operator!=(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator!=(rhs);
}

bool operator<(const OTIdentifier& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<(rhs);
}

bool operator<(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<(rhs);
}

bool operator<(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<(rhs);
}

bool operator<(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<(rhs);
}

bool operator>(const OTIdentifier& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>(rhs);
}

bool operator>(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>(rhs);
}

bool operator>(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>(rhs);
}

bool operator>(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>(rhs);
}

bool operator<=(const OTIdentifier& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<=(rhs);
}

bool operator<=(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<=(rhs);
}

bool operator<=(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<=(rhs);
}

bool operator<=(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator<=(rhs);
}

bool operator>=(const OTIdentifier& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>=(rhs);
}

bool operator>=(const OTNymID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>=(rhs);
}

bool operator>=(const OTServerID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>=(rhs);
}

bool operator>=(const OTUnitID& lhs, const opentxs::Identifier& rhs)
{
    return lhs.get().operator>=(rhs);
}

OTIdentifier Identifier::Factory()
{
    return OTIdentifier(new implementation::Identifier());
}

OTNymID identifier::Nym::Factory()
{
    return OTNymID(new implementation::Identifier());
}

OTServerID identifier::Server::Factory()
{
    return OTServerID(new implementation::Identifier());
}

OTUnitID identifier::UnitDefinition::Factory()
{
    return OTUnitID(new implementation::Identifier());
}

OTIdentifier Identifier::Factory(const Identifier& rhs)
{
    return OTIdentifier(
#ifndef _WIN32
        rhs.clone()
#else
        dynamic_cast<Identifier*>(rhs.clone())
#endif
    );
}

OTIdentifier Identifier::Factory(const std::string& rhs)
{
    return OTIdentifier(new implementation::Identifier(rhs));
}

OTNymID identifier::Nym::Factory(const std::string& rhs)
{
    return OTNymID(new implementation::Identifier(rhs));
}

OTServerID identifier::Server::Factory(const std::string& rhs)
{
    return OTServerID(new implementation::Identifier(rhs));
}

OTUnitID identifier::UnitDefinition::Factory(const std::string& rhs)
{
    return OTUnitID(new implementation::Identifier(rhs));
}

OTIdentifier Identifier::Factory(const String& rhs)
{
    return OTIdentifier(new implementation::Identifier(rhs));
}

OTNymID identifier::Nym::Factory(const String& rhs)
{
    return OTNymID(new implementation::Identifier(rhs));
}

OTServerID identifier::Server::Factory(const String& rhs)
{
    return OTServerID(new implementation::Identifier(rhs));
}

OTUnitID identifier::UnitDefinition::Factory(const String& rhs)
{
    return OTUnitID(new implementation::Identifier(rhs));
}

OTIdentifier Identifier::Factory(const identity::Nym& nym)
{
    return OTIdentifier(new implementation::Identifier(nym));
}

OTNymID identifier::Nym::Factory(const identity::Nym& rhs)
{
    return OTNymID(new implementation::Identifier(rhs));
}

OTIdentifier Identifier::Factory(const Contract& contract)
{
    return OTIdentifier(new implementation::Identifier(contract));
}

OTIdentifier Identifier::Factory(const Cheque& cheque)
{
    if (check.hasRemitter()) {

        return OTIdentifier(
            implementation::Identifier::voucher_contents_to_identifier(cheque));
    }

    return OTIdentifier(
        implementation::Identifier::contract_contents_to_identifier(cheque));
}

OTIdentifier Identifier::Factory(const Item& item)
{
    return OTIdentifier(
        implementation::Identifier::contract_contents_to_identifier(item));
}

OTIdentifier Identifier::Factory(
    const proto::ContactItemType type,
    const proto::HDPath& path)
{
    return OTIdentifier(new implementation::Identifier(type, path));
}

OTIdentifier Identifier::Random()
{
    OTIdentifier output{new implementation::Identifier};
    auto nonce = Data::Factory();
    Context().Crypto().Encode().Nonce(32, nonce);

    OT_ASSERT(32 == nonce->size());

    output->CalculateDigest(nonce->Bytes());

    OT_ASSERT(false == output->empty());

    return output;
}

bool Identifier::Validate(const std::string& input)
{
    if (input.empty()) { return false; }

    const auto id = Factory(input);

    return (0 < id->size());
}
}  // namespace opentxs

namespace opentxs::implementation
{
Identifier::Identifier()
    : ot_super()
{
}

Identifier::Identifier(const std::string& theStr)
    : ot_super()
{
    SetString(theStr);
}

Identifier::Identifier(const String& theStr)
    : ot_super()
{
    SetString(theStr);
}

Identifier::Identifier(const Contract& theContract)
    : ot_super()  // Get the contract's ID into this identifier.
{
    (const_cast<Contract&>(theContract)).GetIdentifier(*this);
}

Identifier::Identifier(const identity::Nym& theNym)
    : ot_super()  // Get the Nym's ID into this identifier.
{
    (const_cast<identity::Nym&>(theNym)).GetIdentifier(*this);
}

Identifier::Identifier(
    const Vector& data,
    const std::size_t size,
    const ID type)
    : ot_super(data, size)
    , type_(type)
{
}

Identifier::Identifier(
    const proto::ContactItemType type,
    const proto::HDPath& path)
    : ot_super()
{
    CalculateDigest(path_to_data(type, path)->Bytes(), DefaultType);
}

bool Identifier::operator==(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return ots1->Compare(ots2);
}

bool Identifier::operator!=(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return !(ots1->Compare(ots2));
}

bool Identifier::operator>(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return ots1->operator>(ots2);
}

bool Identifier::operator<(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return ots1->operator<(ots2);
}

bool Identifier::operator<=(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return ots1->operator<=(ots2);
}

bool Identifier::operator>=(const opentxs::Identifier& s2) const
{
    const auto ots1 = String::Factory(*this), ots2 = String::Factory(s2);
    return ots1->operator>=(ots2);
}

bool Identifier::CalculateDigest(const ReadView bytes, const ID type)
{
    type_ = type;

    return Context().Crypto().Hash().Digest(
        IDToHashType(type_), bytes, WriteInto());
}

Identifier* Identifier::clone() const
{
    return new Identifier(data_, position_, type_);
}

Identifier* Identifier::contract_contents_to_identifier(const Contract& in)
{
    auto output = new Identifier();

    OT_ASSERT(nullptr != output);

    output->CalculateDigest(String::Factory(in)->Bytes());

    return output;
}

Identifier* Identifier::voucher_contents_to_identifier(const Cheque& in)
{
    auto output = new Identifier();
    OT_ASSERT(nullptr != output)

    auto strVoucher = in.GetVoucherIdText();
    output->CalculateDigest(strVoucher);

    return output;
}

// This Identifier is stored in binary form.
// But what if you want a pretty string version of it?
// Just call this function.
void Identifier::GetString(String& id) const
{
    auto data = Data::Factory();
    data->Assign(&type_, sizeof(type_));

    OT_ASSERT(1 == data->size());

    if (0 == size()) { return; }

    data->Concatenate(this->data(), size());

    auto output = String::Factory("ot");
    output->Concatenate(String::Factory(
        Context().Crypto().Encode().IdentifierEncode(data).c_str()));
    id.swap(output);
}

proto::HashType Identifier::IDToHashType(const ID type)
{
    switch (type) {
        case (ID::sha256): {
            return proto::HASHTYPE_SHA256;
        }
        case (ID::blake2b): {
            return proto::HASHTYPE_BLAKE2B160;
        }
        default: {
            return proto::HASHTYPE_NONE;
        }
    }
}

OTData Identifier::path_to_data(
    const proto::ContactItemType type,
    const proto::HDPath& path)
{
    auto output = Data::Factory(static_cast<const void*>(&type), sizeof(type));
    output += Data::Factory(path.root().c_str(), path.root().size());

    for (const auto& child : path.child()) {
        output += Data::Factory(&child, sizeof(child));
    }

    return output;
}

// SET (binary id) FROM ENCODED STRING
void Identifier::SetString(const String& encoded)
{
    return SetString(std::string(encoded.Get()));
}

void Identifier::SetString(const std::string& encoded)
{
    empty();

    if (MinimumSize > encoded.size()) { return; }

    if ('o' != encoded.at(0)) { return; }
    if ('t' != encoded.at(1)) { return; }

    std::string input(encoded.data() + 2, encoded.size() - 2);
    auto data = Context().Crypto().Encode().IdentifierDecode(input);

    if (!data.empty()) {
        type_ = static_cast<ID>(data[0]);

        switch (type_) {
            case (ID::sha256): {
            } break;
            case (ID::blake2b): {
            } break;
            default: {
                type_ = ID::invalid;

                return;
            }
        }

        Assign(
            (reinterpret_cast<const std::uint8_t*>(data.data()) + 1),
            (data.size() - 1));
    }
}

std::string Identifier::str() const
{
    auto output = String::Factory();
    GetString(output);

    return output->Get();
}

void Identifier::swap(opentxs::Identifier& rhs)
{
    auto& input = dynamic_cast<Identifier&>(rhs);
    ot_super::swap(std::move(input));
    std::swap(type_, input.type_);
}
}  // namespace opentxs::implementation
