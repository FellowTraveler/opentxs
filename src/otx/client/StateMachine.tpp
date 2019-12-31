// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "StateMachine.hpp"

namespace opentxs::otx::client::implementation
{
template <>
CheckNymTask& StateMachine::get_param()
{
    return param_.check_nym_;
}
template <>
DepositPaymentTask& StateMachine::get_param()
{
    return param_.deposit_payment_;
}
template <>
DownloadContractTask& StateMachine::get_param()
{
    return param_.download_contract_;
}
#if OT_CASH
template <>
DownloadMintTask& StateMachine::get_param()
{
    return param_.download_mint_;
}
#endif
template <>
DownloadNymboxTask& StateMachine::get_param()
{
    return param_.download_nymbox_;
}
template <>
DownloadUnitDefinitionTask& StateMachine::get_param()
{
    return param_.download_unit_definition_;
}
template <>
GetTransactionNumbersTask& StateMachine::get_param()
{
    return param_.get_transaction_numbers_;
}
template <>
IssueUnitDefinitionTask& StateMachine::get_param()
{
    return param_.issue_unit_definition_;
}
template <>
MessageTask& StateMachine::get_param()
{
    return param_.send_message_;
}
#if OT_CASH
template <>
PayCashTask& StateMachine::get_param()
{
    return param_.send_cash_;
}
#endif
template <>
PaymentTask& StateMachine::get_param()
{
    return param_.send_payment_;
}
template <>
PeerReplyTask& StateMachine::get_param()
{
    return param_.peer_reply_;
}
template <>
PeerRequestTask& StateMachine::get_param()
{
    return param_.peer_request_;
}
template <>
ProcessInboxTask& StateMachine::get_param()
{
    return param_.process_inbox_;
}
template <>
PublishServerContractTask& StateMachine::get_param()
{
    return param_.publish_server_contract_;
}
template <>
RegisterAccountTask& StateMachine::get_param()
{
    return param_.register_account_;
}
template <>
RegisterNymTask& StateMachine::get_param()
{
    return param_.register_nym_;
}
template <>
SendChequeTask& StateMachine::get_param()
{
    return param_.send_cheque_;
}
template <>
SendInvoiceTask& StateMachine::get_param()
{
    return param_.send_invoice_;
}
template <>
SendVoucherTask& StateMachine::get_param()
{
    return param_.send_voucher_;
}
template <>
SendTransferTask& StateMachine::get_param()
{
    return param_.send_transfer_;
}
#if OT_CASH
template <>
WithdrawCashTask& StateMachine::get_param()
{
    return param_.withdraw_cash_;
}
#endif
template <>
WithdrawVoucherTask& StateMachine::get_param()
{
    return param_.withdraw_voucher_;
}
template <>
const UniqueQueue<CheckNymTask>& StateMachine::get_task() const
{
    return check_nym_;
}
template <>
const UniqueQueue<DepositPaymentTask>& StateMachine::get_task() const
{
    return deposit_payment_;
}
template <>
const UniqueQueue<DownloadContractTask>& StateMachine::get_task() const
{
    return download_contract_;
}
#if OT_CASH
template <>
const UniqueQueue<DownloadMintTask>& StateMachine::get_task() const
{
    return download_mint_;
}
#endif
template <>
const UniqueQueue<DownloadNymboxTask>& StateMachine::get_task() const
{
    return download_nymbox_;
}
template <>
const UniqueQueue<DownloadUnitDefinitionTask>& StateMachine::get_task() const
{
    return download_unit_definition_;
}
template <>
const UniqueQueue<GetTransactionNumbersTask>& StateMachine::get_task() const
{
    return get_transaction_numbers_;
}
template <>
const UniqueQueue<IssueUnitDefinitionTask>& StateMachine::get_task() const
{
    return issue_unit_definition_;
}
template <>
const UniqueQueue<MessageTask>& StateMachine::get_task() const
{
    return send_message_;
}
#if OT_CASH
template <>
const UniqueQueue<PayCashTask>& StateMachine::get_task() const
{
    return send_cash_;
}
#endif
template <>
const UniqueQueue<PaymentTask>& StateMachine::get_task() const
{
    return send_payment_;
}
template <>
const UniqueQueue<PeerReplyTask>& StateMachine::get_task() const
{
    return peer_reply_;
}
template <>
const UniqueQueue<PeerRequestTask>& StateMachine::get_task() const
{
    return peer_request_;
}
template <>
const UniqueQueue<ProcessInboxTask>& StateMachine::get_task() const
{
    return process_inbox_;
}
template <>
const UniqueQueue<PublishServerContractTask>& StateMachine::get_task() const
{
    return publish_server_contract_;
}
template <>
const UniqueQueue<RegisterAccountTask>& StateMachine::get_task() const
{
    return register_account_;
}
template <>
const UniqueQueue<RegisterNymTask>& StateMachine::get_task() const
{
    return register_nym_;
}
template <>
const UniqueQueue<SendChequeTask>& StateMachine::get_task() const
{
    return send_cheque_;
}
template <>
const UniqueQueue<SendInvoiceTask>& StateMachine::get_task() const
{
    return send_invoice_;
}
template <>
const UniqueQueue<SendVoucherTask>& StateMachine::get_task() const
{
    return send_voucher_;
}
template <>
const UniqueQueue<SendTransferTask>& StateMachine::get_task() const
{
    return send_transfer_;
}
#if OT_CASH
template <>
const UniqueQueue<WithdrawCashTask>& StateMachine::get_task() const
{
    return withdraw_cash_;
}
#endif
template <>
const UniqueQueue<WithdrawVoucherTask>& StateMachine::get_task() const
{
    return withdraw_voucher_;
}

template <>
bool StateMachine::load_contract<CheckNymTask>(const identifier::Nym& id) const
{
    return bool(client_.Wallet().Nym(id));
}
template <>
bool StateMachine::load_contract<DownloadContractTask>(
    const identifier::Server& id) const
{
    try {
        client_.Wallet().Server(id);

        return true;
    } catch (...) {

        return false;
    }
}

template <>
bool StateMachine::load_contract<DownloadUnitDefinitionTask>(
    const identifier::UnitDefinition& id) const
{
    try {
        client_.Wallet().UnitDefinition(id);

        return true;
    } catch (...) {

        return false;
    }
}

template StateMachine::BackgroundTask StateMachine::StartTask(
    const CheckNymTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const DepositPaymentTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const DownloadContractTask& params) const;
#if OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const DownloadMintTask& params) const;
#endif  // OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const DownloadNymboxTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const DownloadUnitDefinitionTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const GetTransactionNumbersTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const IssueUnitDefinitionTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const MessageTask& params) const;
#if OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const PayCashTask& params) const;
#endif  // OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const PaymentTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const PeerReplyTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const PeerRequestTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const ProcessInboxTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const PublishServerContractTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const RegisterAccountTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const RegisterNymTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const SendChequeTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const SendInvoiceTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const SendVoucherTask& params) const;
template StateMachine::BackgroundTask StateMachine::StartTask(
    const SendTransferTask& params) const;
#if OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const WithdrawCashTask& params) const;
#endif  // OT_CASH
template StateMachine::BackgroundTask StateMachine::StartTask(
    const WithdrawVoucherTask& params) const;
}  // namespace opentxs::otx::client::implementation
