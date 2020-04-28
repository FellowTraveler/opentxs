// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLIND_MINT_HPP
#define OPENTXS_BLIND_MINT_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated


#include <cstdint>
#include <ctime>

#if OT_CASH
#include "opentxs/core/Amount.hpp"
#include "opentxs/core/Contract.hpp"

namespace opentxs
{
namespace blind
{
class Mint : virtual public Contract
{
public:
    OPENTXS_EXPORT virtual OTIdentifier AccountID() const = 0;
    OPENTXS_EXPORT virtual bool Expired() const = 0;
    OPENTXS_EXPORT virtual OTAmount GetDenomination(
        std::int32_t nIndex) const = 0;
    OPENTXS_EXPORT virtual std::int32_t GetDenominationCount() const = 0;
    OPENTXS_EXPORT virtual Time GetExpiration() const = 0;
    OPENTXS_EXPORT virtual OTAmount GetLargestDenomination(
        const Amount& amount) const = 0;
    OPENTXS_EXPORT virtual bool GetPrivate(
        Armored& theArmor,
        const Amount& denomination) const = 0;
    OPENTXS_EXPORT virtual bool GetPublic(
        Armored& theArmor,
        const Amount& denomination) const = 0;
    OPENTXS_EXPORT virtual std::int32_t GetSeries() const = 0;
    OPENTXS_EXPORT virtual Time GetValidFrom() const = 0;
    OPENTXS_EXPORT virtual Time GetValidTo() const = 0;
    OPENTXS_EXPORT virtual const identifier::UnitDefinition&
    InstrumentDefinitionID() const = 0;

    OPENTXS_EXPORT virtual bool AddDenomination(
        const identity::Nym& theNotary,
        const Amount& denomination,
        const std::size_t keySize,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual void GenerateNewMint(
        const api::Wallet& wallet,
        const std::int32_t nSeries,
        const Time VALID_FROM,
        const Time VALID_TO,
        const Time MINT_EXPIRATION,
        const identifier::UnitDefinition& theInstrumentDefinitionID,
        const identifier::Server& theNotaryID,
        const identity::Nym& theNotary,
        const Amount& nDenom1,
        const Amount& nDenom2,
        const Amount& nDenom3,
        const Amount& nDenom4,
        const Amount& nDenom5,
        const Amount& nDenom6,
        const Amount& nDenom7,
        const Amount& nDenom8,
        const Amount& nDenom9,
        const Amount& nDenom10,
        const std::size_t keySize,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool LoadMint(const char* szAppend = nullptr) = 0;
    OPENTXS_EXPORT virtual void Release_Mint() = 0;
    OPENTXS_EXPORT virtual void ReleaseDenominations() = 0;
    OPENTXS_EXPORT virtual bool SaveMint(const char* szAppend = nullptr) = 0;
    OPENTXS_EXPORT virtual void SetInstrumentDefinitionID(
        const identifier::UnitDefinition& newID) = 0;
    OPENTXS_EXPORT virtual void SetSavePrivateKeys(bool bDoIt = true) = 0;
    OPENTXS_EXPORT virtual bool SignToken(
        const identity::Nym& notary,
        blind::Token& token,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool VerifyMint(
        const identity::Nym& theOperator) = 0;
    OPENTXS_EXPORT virtual bool VerifyToken(
        const identity::Nym& notary,
        const blind::Token& token,
        const PasswordPrompt& reason) = 0;

    OPENTXS_EXPORT ~Mint() override = default;

protected:
    Mint() = default;

private:
    Mint(const Mint&) = delete;
    Mint(Mint&&) = delete;
    Mint& operator=(const Mint&) = delete;
    Mint& operator=(Mint&&) = delete;
};
}  // namespace blind
}  // namespace opentxs
#endif  // OT_CASH
#endif
