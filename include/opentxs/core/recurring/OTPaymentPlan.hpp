// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_RECURRING_OTPAYMENTPLAN_HPP
#define OPENTXS_CORE_RECURRING_OTPAYMENTPLAN_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include "opentxs/core/Amount.hpp"

#include <irrxml/irrXML.hpp>
#include <chrono>
#include <cstdint>

#include "opentxs/Types.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/recurring/OTAgreement.hpp"

namespace opentxs
{
namespace api
{
namespace implementation
{
class Factory;
}  // namespace implementation

namespace internal
{
struct Core;
}  // namespace internal

class Wallet;
}  // namespace api

namespace identifier
{
class Nym;
class Server;
class UnitDefinition;
}  // namespace identifier

namespace identity
{
class Nym;
}  // namespace identity

namespace otx
{
namespace context
{
class Client;
}  // namespace context
}  // namespace otx

class Identifier;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
/*
 OTPaymentPlan

 This instrument is signed by two parties or more (the first one, I think...)

 While processing payment, BOTH parties to a payment plan will be loaded up and
 their signatures will be checked against the original plan, which is saved as a
 cron receipt.

 There is also a "current version" of the payment plan, which contains updated
 info
 from processing, and is signed by the server.

 BOTH the original version, and the updated version, are sent to EACH user
 whenever
 a payment is processed, as his receipt. This way you have the user's signature
 on
 the terms, and the server's signature whenever it carries out the terms. A
 receipt
 with both is placed in the inbox of both users after any action.

 As with cheques, the server can use the receipts in the inboxes, plus the last
 agreed
 balance, to prove the current balance of any account. The user removes the
 receipt from
 his inbox by accepting it and, in the process, performing a new balance
 agreement.

 THIS MEANS that the OT server can carry out the terms of contracts!  So far, at
 least,
 cheques, trades, payment plans... as long as everything is signed off, we're
 free and
 clear under the same triple-signed system that account transfer uses. (The
 Users cannot
 repudiate their own signatures later, and the server can prove all balances
 with the
 user's own signature.)

 Of course, either side is free to CANCEL a payment plan, or to leave their
 account bereft
 of funds and prone to failed payments. But if they cancel, their signature will
 appear
 on the cancellation request, and the recipient gets a copy of it in his inbox.
 And if
 the funds are insufficient, the plan will keep trying to charge, leaving
 failure notices
 in both inboxes when such things occur.

 You could even have the server manage an issuer account, backed in payment plan
 revenue,
 that would form a new instrument definition that can then be traded on markets.
 (The same
 as you can
 have the server manage the issuer account for a basket currency now, which is
 backed with
 reserve accounts managed by the server, and you can then trade the basket
 currency on markets.)
 */
class OTPaymentPlan : public OTAgreement
{
public:
    // *********** Methods for generating a payment plan: ****************
    // From parent:  (This must be called first, before the other two methods
    // below can be called.)
    //
    //  bool        SetAgreement(const std::int64_t& lTransactionNum, const
    //  OTString&
    //  strConsideration,
    //                           const Time VALID_FROM=0,   const Time
    //                           VALID_TO=0);

    // Then call one (or both) of these:

    OPENTXS_EXPORT bool SetInitialPayment(
        const Amount& lAmount,
        const std::chrono::seconds tTimeUntilInitialPayment = {});  // default:
                                                                    // now.

    // These two methods (above and below) can be called independent of each
    // other.
    //
    // Meaning: You can have an initial payment AND/OR a payment plan.

    OPENTXS_EXPORT bool SetPaymentPlan(
        const Amount& lPaymentAmount,
        const std::chrono::seconds tTimeUntilPlanStart =
            std::chrono::hours{24 * 30},
        const std::chrono::seconds tBetweenPayments =
            std::chrono::hours{24 * 30},  // Default: 30
                                          // days.
        const std::chrono::seconds tPlanLength = {},
        const std::int32_t nMaxPayments = 0);

    // VerifyAgreement()
    // This function verifies both Nyms and both signatures. Due to the
    // peculiar nature of how OTAgreement/OTPaymentPlan works, there are two
    // signed copies stored. The merchant signs first, adding his
    // transaction numbers (2), and then he sends it to the customer, who
    // also adds two numbers and signs. (Also resetting the creation date.)
    // The problem is, adding the additional transaction numbers invalidates
    // the first (merchant's) signature. The solution is, when the customer
    // confirms the agreement, he stores an internal copy of the merchant's
    // signed version.  This way later, in VERIFY AGREEMENT, the internal
    // copy can be loaded, and BOTH Nyms can be checked to verify that BOTH
    // transaction numbers are valid for each. The two versions of the
    // contract can also be compared to each other, to make sure that none
    // of the vital terms, values, clauses, etc are different between the two.
    //
    bool VerifyAgreement(
        const otx::context::Client& recipient,
        const otx::context::Client& sender) const override;
    bool CompareAgreement(const OTAgreement& rh) const override;

    bool VerifyMerchantSignature(const identity::Nym& RECIPIENT_NYM) const;
    bool VerifyCustomerSignature(const identity::Nym& SENDER_NYM) const;

    // ************ "INITIAL PAYMENT" public GET METHODS **************
    inline bool HasInitialPayment() const { return m_bInitialPayment; }
    inline const Time GetInitialPaymentDate() const
    {
        return m_tInitialPaymentDate;
    }
    inline const Amount& GetInitialPaymentAmount() const
    {
        return m_lInitialPaymentAmount;
    }
    inline bool IsInitialPaymentDone() const { return m_bInitialPaymentDone; }

    inline const Time GetInitialPaymentCompletedDate() const
    {
        return m_tInitialPaymentCompletedDate;
    }
    inline const Time GetLastFailedInitialPaymentDate() const
    {
        return m_tFailedInitialPaymentDate;
    }
    inline std::int32_t GetNoInitialFailures() const
    {
        return m_nNumberInitialFailures;
    }

    // ************ "PAYMENT PLAN" public GET METHODS ****************
    inline bool HasPaymentPlan() const { return m_bPaymentPlan; }
    inline const Amount& GetPaymentPlanAmount() const
    {
        return m_lPaymentPlanAmount;
    }
    inline const std::chrono::seconds GetTimeBetweenPayments() const
    {
        return m_tTimeBetweenPayments;
    }
    inline const Time GetPaymentPlanStartDate() const
    {
        return m_tPaymentPlanStartDate;
    }
    inline const std::chrono::seconds GetPaymentPlanLength() const
    {
        return m_tPaymentPlanLength;
    }
    inline std::int32_t GetMaximumNoPayments() const
    {
        return m_nMaximumNoPayments;
    }

    inline const Time GetDateOfLastPayment() const
    {
        return m_tDateOfLastPayment;
    }
    inline const Time GetDateOfLastFailedPayment() const
    {
        return m_tDateOfLastFailedPayment;
    }

    inline std::int32_t GetNoPaymentsDone() const { return m_nNoPaymentsDone; }
    inline std::int32_t GetNoFailedPayments() const
    {
        return m_nNoFailedPayments;
    }

    // Return True if should stay on OTCron's list for more processing.
    // Return False if expired or otherwise should be removed.
    bool ProcessCron(const PasswordPrompt& reason) override;  // OTCron calls
                                                              // this regularly,
                                                              // which is my
                                                              // chance to
                                                              // expire, etc.
    void InitPaymentPlan();
    void Release() override;
    void Release_PaymentPlan();
    // return -1 if error, 0 if nothing, and 1 if the node was processed.
    std::int32_t ProcessXMLNode(irr::io::IrrXMLReader*& xml) override;
    void UpdateContents(const PasswordPrompt& reason)
        override;  // Before transmission or serialization,
                   // this
                   // is where the ledger saves its contents

    OPENTXS_EXPORT ~OTPaymentPlan() override;

private:
    friend api::implementation::Factory;

    OTPaymentPlan(const api::internal::Core& core);
    OTPaymentPlan(
        const api::internal::Core& core,
        const identifier::Server& NOTARY_ID,
        const identifier::UnitDefinition& INSTRUMENT_DEFINITION_ID);
    OTPaymentPlan(
        const api::internal::Core& core,
        const identifier::Server& NOTARY_ID,
        const identifier::UnitDefinition& INSTRUMENT_DEFINITION_ID,
        const Identifier& SENDER_ACCT_ID,
        const identifier::Nym& SENDER_NYM_ID,
        const Identifier& RECIPIENT_ACCT_ID,
        const identifier::Nym& RECIPIENT_NYM_ID);

protected:
    // "INITIAL PAYMENT" protected SET METHODS
    inline void SetInitialPaymentDate(const Time tInitialPaymentDate)
    {
        m_tInitialPaymentDate = tInitialPaymentDate;
    }
    void SetInitialPaymentAmount(const Amount& amount);

    // Sets the bool that officially the initial payment has been done. (Checks
    // first to make sure not already done.)
    bool SetInitialPaymentDone();

    inline void SetInitialPaymentCompletedDate(const Time tInitialPaymentDate)
    {
        m_tInitialPaymentCompletedDate = tInitialPaymentDate;
    }
    inline void SetLastFailedInitialPaymentDate(
        const Time tFailedInitialPaymentDate)
    {
        m_tFailedInitialPaymentDate = tFailedInitialPaymentDate;
    }

    inline void SetNoInitialFailures(const std::int32_t& nNoFailures)
    {
        m_nNumberInitialFailures = nNoFailures;
    }
    inline void IncrementNoInitialFailures() { m_nNumberInitialFailures++; }

    // "PAYMENT PLAN" protected SET METHODS
    void SetPaymentPlanAmount(const Amount& amount);

    inline void SetTimeBetweenPayments(const std::chrono::seconds tTimeBetween)
    {
        m_tTimeBetweenPayments = tTimeBetween;
    }
    inline void SetPaymentPlanStartDate(const Time tPlanStartDate)
    {
        m_tPaymentPlanStartDate = tPlanStartDate;
    }
    inline void SetPaymentPlanLength(const std::chrono::seconds tPlanLength)
    {
        m_tPaymentPlanLength = tPlanLength;
    }
    inline void SetMaximumNoPayments(std::int32_t nMaxNoPayments)
    {
        m_nMaximumNoPayments = nMaxNoPayments;
    }

    inline void SetDateOfLastPayment(const Time tDateOfLast)
    {
        m_tDateOfLastPayment = tDateOfLast;
    }
    inline void SetDateOfLastFailedPayment(const Time tDateOfLast)
    {
        m_tDateOfLastFailedPayment = tDateOfLast;
    }

    inline void SetNoPaymentsDone(std::int32_t nNoPaymentsDone)
    {
        m_nNoPaymentsDone = nNoPaymentsDone;
    }
    inline void SetNoFailedPayments(std::int32_t nNoFailed)
    {
        m_nNoFailedPayments = nNoFailed;
    }

    inline void IncrementNoPaymentsDone() { m_nNoPaymentsDone++; }
    inline void IncrementNoFailedPayments() { m_nNoFailedPayments++; }

    bool ProcessPayment(
        const api::Wallet& wallet,
        const Amount& amount,
        const PasswordPrompt& reason);
    void ProcessInitialPayment(
        const api::Wallet& wallet,
        const PasswordPrompt& reason);
    void ProcessPaymentPlan(
        const api::Wallet& wallet,
        const PasswordPrompt& reason);

private:
    using ot_super = OTAgreement;

    // "INITIAL PAYMENT" private MEMBERS
    bool m_bInitialPayment;      // Will there be an initial payment?
    Time m_tInitialPaymentDate;  // Date of the initial payment, measured
                                 // seconds after creation.
    Time m_tInitialPaymentCompletedDate;   // Date the initial payment was
                                           // finally transacted.
    Time m_tFailedInitialPaymentDate;      // Date of the last failed
                                           // payment, measured seconds after
                                           // creation.
    OTAmount m_lInitialPaymentAmount;  // Amount of the initial payment.
    bool m_bInitialPaymentDone;            // Has the initial payment been made?
    std::int32_t m_nNumberInitialFailures;  // If we've tried to process this
                                            // multiple times, we'll know.
    // "PAYMENT PLAN" private MEMBERS
    bool m_bPaymentPlan;                // Will there be a payment plan?
    OTAmount m_lPaymentPlanAmount;  // Amount of each payment.
    std::chrono::seconds m_tTimeBetweenPayments;  // How much time between
                                                  // each payment?
    Time m_tPaymentPlanStartDate;  // Date for the first payment plan
                                   // payment.
    std::chrono::seconds m_tPaymentPlanLength;  // Optional. Plan length
                                                // measured in seconds since
                                                // plan start.
    std::int32_t m_nMaximumNoPayments;          // Optional. The most number of
                                                // payments that are authorized.

    Time m_tDateOfLastPayment;         // Recording of date of the last payment.
    Time m_tDateOfLastFailedPayment;   // Recording of date of the last
                                       // failed payment.
    std::int32_t m_nNoPaymentsDone;    // Recording of the number of payments
                                       // already processed.
    std::int32_t m_nNoFailedPayments;  // Every time a payment fails, we
                                       // record that here.
    // These are NOT stored as part of the payment plan. They are merely used
    // during execution.
    bool m_bProcessingInitialPayment;
    bool m_bProcessingPaymentPlan;

    OTPaymentPlan() = delete;
};
}  // namespace opentxs
#endif
