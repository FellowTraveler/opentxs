// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blind/mint/Lucre.cpp"

#pragma once

#include <cstdint>
#include <iosfwd>

#include "blind/Mint.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace blind
{
class Mint;
class Token;
}  // namespace blind

namespace identity
{
class Nym;
}  // namespace identity

class Factory;
class PasswordPrompt;
class String;
}  // namespace opentxs

namespace opentxs::blind::mint::implementation
{
class Lucre final : Mint
{
public:
    auto AddDenomination(
        const identity::Nym& theNotary,
        const Amount& denomination,
        const std::size_t keySize,
        const PasswordPrompt& reason) -> bool final;

    auto SignToken(
        const identity::Nym& notary,
        blind::Token& token,
        const PasswordPrompt& reason) -> bool final;
    auto VerifyToken(
        const identity::Nym& notary,
        const blind::Token& token,
        const PasswordPrompt& reason) -> bool final;

    ~Lucre() final = default;

private:
    friend opentxs::Factory;

    Lucre(const api::internal::Core& core);
    Lucre(
        const api::internal::Core& core,
        const String& strNotaryID,
        const String& strInstrumentDefinitionID);
    Lucre(
        const api::internal::Core& core,
        const String& strNotaryID,
        const String& strServerNymID,
        const String& strInstrumentDefinitionID);
};
}  // namespace opentxs::blind::mint::implementation
