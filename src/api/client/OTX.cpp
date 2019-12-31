// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/Pair.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/client/OT_API.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/client/OTClient.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/core/contract/peer/BailmentNotice.hpp"
#include "opentxs/core/contract/peer/BailmentReply.hpp"
#include "opentxs/core/contract/peer/BailmentRequest.hpp"
#include "opentxs/core/contract/peer/ConnectionReply.hpp"
#include "opentxs/core/contract/peer/ConnectionRequest.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/contract/peer/NoticeAcknowledgement.hpp"
#include "opentxs/core/contract/peer/OutBailmentReply.hpp"
#include "opentxs/core/contract/peer/OutBailmentRequest.hpp"
#include "opentxs/core/contract/peer/StoreSecret.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/ext/OTPayment.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Pull.hpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/network/zeromq/socket/Subscribe.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/otx/Reply.hpp"
#include "opentxs/Proto.tpp"

#include "internal/api/client/Client.hpp"
#include "otx/client/StateMachine.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <map>
#include <thread>
#include <tuple>

#include "OTX.hpp"

#define CHECK_NYM(a)                                                           \
    {                                                                          \
        if (a.empty()) {                                                       \
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid ")(#a)(".").Flush(); \
                                                                               \
            return error_task();                                               \
        }                                                                      \
    }

#define CHECK_SERVER(a, b)                                                     \
    {                                                                          \
        CHECK_NYM(a)                                                           \
                                                                               \
        if (b.empty()) {                                                       \
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid ")(#b)(".").Flush(); \
                                                                               \
            return error_task();                                               \
        }                                                                      \
    }

#define CHECK_ARGS(a, b, c)                                                    \
    {                                                                          \
        CHECK_SERVER(a, b)                                                     \
                                                                               \
        if (c.empty()) {                                                       \
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid ")(#c)(".").Flush(); \
                                                                               \
            return error_task();                                               \
        }                                                                      \
    }

#define SHUTDOWN()                                                             \
    {                                                                          \
        YIELD(50);                                                             \
    }

#define YIELD(a)                                                               \
    {                                                                          \
        if (!running_) { return false; }                                       \
                                                                               \
        Sleep(std::chrono::milliseconds(a));                                   \
    }

#define CONTACT_REFRESH_DAYS 1
#define INTRODUCTION_SERVER_KEY "introduction_server_id"
#define MASTER_SECTION "Master"

#define OT_METHOD "opentxs::api::client::implementation::OTX::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs
{
api::client::OTX* Factory::OTX(
    const Flag& running,
    const api::client::internal::Manager& client,
    OTClient& otclient,
    const ContextLockCallback& lockCallback)
{
    return new api::client::implementation::OTX(
        running, client, otclient, lockCallback);
}
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
const std::string OTX::DEFAULT_INTRODUCTION_SERVER =
    R"(-----BEGIN OT ARMORED SERVER CONTRACT-----
Version: Open Transactions 0.99.1-113-g2b3acf5
Comment: http://opentransactions.org

CAESI290b20xcHFmREJLTmJLR3RiN0NBa0ZodFRXVFVOTHFIRzIxGiNvdHVkd3p4cWF0UHh4
bmh4VFV3RUo3am5HenE2RkhGYTRraiIMU3Rhc2ggQ3J5cHRvKr8NCAESI290dWR3enhxYXRQ
eHhuaHhUVXdFSjdqbkd6cTZGSEZhNGtqGAIoATJTCAEQAiJNCAESIQI9MywLxxKfOtai26pj
JbxKtCCPhM/DbvX08iwbW2qYqhoga6Ccvp6CABGAFj/RdWNjtg5uzIRHT5Dn+fUzdAM9SUSA
AQCIAQA6vAwIARIjb3R1ZHd6eHFhdFB4eG5oeFRVd0VKN2puR3pxNkZIRmE0a2oaI290dXdo
ZzNwb2kxTXRRdVkzR3hwYWpOaXp5bmo0NjJ4Z2RIIAIymgQIARIjb3R1d2hnM3BvaTFNdFF1
WTNHeHBhak5penluajQ2MnhnZEgYAiABKAIyI290dWR3enhxYXRQeHhuaHhUVXdFSjdqbkd6
cTZGSEZhNGtqQl0IARJTCAEQAiJNCAESIQI9MywLxxKfOtai26pjJbxKtCCPhM/DbvX08iwb
W2qYqhoga6Ccvp6CABGAFj/RdWNjtg5uzIRHT5Dn+fUzdAM9SUSAAQCIAQAaBAgBEAJKiAEI
ARACGioIARAEGAIgASogZ6MtTp4aTEDLxFfhnsGo+Esp5B4hkgjWEejNPt5J6C0aKggBEAQY
AiACKiAhqJjWf2Ugqbg6z6ps59crAx9lHwTuT6Eq4x6JmkBlGBoqCAEQBBgCIAMqII2Vps1F
C2YUMbB4kE9XsHt1jrVY6pMPV6KWc5sH3VvTem0IARIjb3R1d2hnM3BvaTFNdFF1WTNHeHBh
ak5penluajQ2MnhnZEgYASAFKkDQLsszAol/Ih56MomuBKV8zpKaw5+ry7Kse1+5nPwJlP8f
72OAgTegBlmv31K4JgLVs52EKJTBpjnV+v0pxzUOem0IARIjb3R1ZHd6eHFhdFB4eG5oeFRV
d0VKN2puR3pxNkZIRmE0a2oYAyAFKkAJZ0LTVM+XBrGbRdiZsEQSbvwqg+mqGwHD5MQ+D4h0
fPQaUrdB6Pp/HM5veox02LBKg05hVNQ64tcU+LAxK+VHQuQDCAESI290clA2dDJXY2hYMjYz
ZVpiclRuVzZyY2FCZVNQb2VqSzJnGAIgAigCMiNvdHVkd3p4cWF0UHh4bmh4VFV3RUo3am5H
enE2RkhGYTRrajonCAESI290dXdoZzNwb2kxTXRRdVkzR3hwYWpOaXp5bmo0NjJ4Z2RISogB
CAEQAhoqCAEQBBgCIAEqIDpwlCrxHNWvvtFt6k8ocB5NBo7vjkGO/mRuSOQ/j/9WGioIARAE
GAIgAiog6Dw0+AWok4dENWWc/3qhykA7NNybWecqMGs5fL8KLLYaKggBEAQYAiADKiD+s/iq
37NrYI4/xdHOYtO/ocR0YqDVz09IaDNGVEdBtnptCAESI290clA2dDJXY2hYMjYzZVpiclRu
VzZyY2FCZVNQb2VqSzJnGAEgBSpATbHtakma53Na35Be+rGvW+z1H6EtkHlljv9Mo8wfies3
in9el1Ejb4BDbGCN5ABl3lQpfedZnR+VYv2X6Y1yBnptCAESI290dXdoZzNwb2kxTXRRdVkz
R3hwYWpOaXp5bmo0NjJ4Z2RIGAEgBSpAeptEmgdqgkGUcOJCqG0MsiChEREUdDzH/hRj877u
WDIHoRHsf/k5dCOHfDct4TDszasVhGFhRdNunpgQJcp0DULnAwgBEiNvdHd6ZWd1dTY3cENI
RnZhYjZyS2JYaEpXelNvdlNDTGl5URgCIAIoAjIjb3R1ZHd6eHFhdFB4eG5oeFRVd0VKN2pu
R3pxNkZIRmE0a2o6JwgBEiNvdHV3aGczcG9pMU10UXVZM0d4cGFqTml6eW5qNDYyeGdkSEqL
AQgBEAIaKwgBEAMYAiABKiEC5p36Ivxs4Wb6CjKTnDA1MFtX3Mx2UBlrmloSt+ffXz0aKwgB
EAMYAiACKiECtMkEo4xsefeevzrBb62ll98VYZy8PipgbrPWqGUNxQMaKwgBEAMYAiADKiED
W1j2DzOZemB9OOZ/pPrFroKDfgILYu2IOtiRFfi0vDB6bQgBEiNvdHd6ZWd1dTY3cENIRnZh
YjZyS2JYaEpXelNvdlNDTGl5URgBIAUqQJYd860/Ybh13GtW+grxWtWjjmzPifHE7bTlgUWl
3bX+ZuWNeEotA4yXQvFNog4PTAOF6dbvCr++BPGepBEUEEx6bQgBEiNvdHV3aGczcG9pMU10
UXVZM0d4cGFqTml6eW5qNDYyeGdkSBgBIAUqQH6GXnKCCDDgDvcSt8dLWuVMlr75zVkHy85t
tccoy2oLHNevDvKrLfUk/zuICyaSIvDy0Kb2ytOuh/O17yabxQ8yHQgBEAEYASISb3Quc3Rh
c2hjcnlwdG8ubmV0KK03MiEIARADGAEiFnQ1NGxreTJxM2w1ZGt3bnQub25pb24orTcyRwgB
EAQYASI8b3ZpcDZrNWVycXMzYm52cjU2cmgzZm5pZ2JuZjJrZWd1cm5tNWZpYnE1NWtqenNv
YW54YS5iMzIuaTJwKK03Op8BTWVzc2FnaW5nLW9ubHkgc2VydmVyIHByb3ZpZGVkIGZvciB0
aGUgY29udmllbmllbmNlIG9mIFN0YXNoIENyeXB0byB1c2Vycy4gU2VydmljZSBpcyBwcm92
aWRlZCBhcyBpcyB3aXRob3V0IHdhcnJhbnR5IG9mIGFueSBraW5kLCBlaXRoZXIgZXhwcmVz
c2VkIG9yIGltcGxpZWQuQiCK4L5cnecfUFz/DQyvAklKC2pTmWQtxt9olQS5/0hUHUptCAES
I290clA2dDJXY2hYMjYzZVpiclRuVzZyY2FCZVNQb2VqSzJnGAUgBSpA1/bep0NTbisZqYns
MCL/PCUJ6FIMhej+ROPk41604x1jeswkkRmXRNjzLlVdiJ/pQMxG4tJ0UQwpxHxrr0IaBA==
-----END OT ARMORED SERVER CONTRACT-----)";

OTX::OTX(
    const Flag& running,
    const api::client::internal::Manager& client,
    OTClient& otclient,
    const ContextLockCallback& lockCallback)
    : lock_callback_(lockCallback)
    , running_(running)
    , client_(client)
    , ot_client_(otclient)
    , introduction_server_lock_()
    , nym_fetch_lock_()
    , task_status_lock_()
    , refresh_counter_(0)
    , operations_()
    , server_nym_fetch_()
    , missing_nyms_()
    , outdated_nyms_()
    , missing_servers_()
    , missing_unit_definitions_()
    , introduction_server_id_()
    , task_status_()
    , task_message_id_()
    , account_subscriber_callback_(zmq::ListenCallback::Factory(
          [this](const zmq::Message& message) -> void {
              this->process_account(message);
          }))
    , account_subscriber_(
          client_.ZeroMQ().SubscribeSocket(account_subscriber_callback_.get()))
    , notification_listener_callback_(zmq::ListenCallback::Factory(
          [this](const zmq::Message& message) -> void {
              this->process_notification(message);
          }))
    , notification_listener_(client_.ZeroMQ().PullSocket(
          notification_listener_callback_,
          zmq::socket::Socket::Direction::Bind))
    , find_nym_callback_(zmq::ListenCallback::Factory(
          [this](const zmq::Message& message) -> void {
              this->find_nym(message);
          }))
    , find_nym_listener_(client_.ZeroMQ().PullSocket(
          find_nym_callback_,
          zmq::socket::Socket::Direction::Bind))
    , find_server_callback_(zmq::ListenCallback::Factory(
          [this](const zmq::Message& message) -> void {
              this->find_server(message);
          }))
    , find_server_listener_(client_.ZeroMQ().PullSocket(
          find_server_callback_,
          zmq::socket::Socket::Direction::Bind))
    , find_unit_callback_(zmq::ListenCallback::Factory(
          [this](const zmq::Message& message) -> void {
              this->find_unit(message);
          }))
    , find_unit_listener_(client_.ZeroMQ().PullSocket(
          find_unit_callback_,
          zmq::socket::Socket::Direction::Bind))
    , task_finished_(client_.ZeroMQ().PublishSocket())
    , auto_process_inbox_(Flag::Factory(true))
    , next_task_id_(0)
    , shutdown_(false)
    , shutdown_lock_()
    , reason_(client_.Factory().PasswordPrompt("Background notary operation"))
{
    // WARNING: do not access client_.Wallet() during construction
    const auto endpoint = client_.Endpoints().AccountUpdate();
    LogDetail(OT_METHOD)(__FUNCTION__)(": Connecting to ")(endpoint).Flush();
    auto listening = account_subscriber_->Start(endpoint);

    OT_ASSERT(listening)

    listening = notification_listener_->Start(
        client_.Endpoints().InternalProcessPushNotification());

    OT_ASSERT(listening)

    const auto publishing =
        task_finished_->Start(client_.Endpoints().TaskComplete());

    OT_ASSERT(publishing)

    listening = find_nym_listener_->Start(client_.Endpoints().FindNym());

    OT_ASSERT(listening)

    listening = find_server_listener_->Start(client_.Endpoints().FindServer());

    OT_ASSERT(listening)

    listening =
        find_unit_listener_->Start(client_.Endpoints().FindUnitDefinition());

    OT_ASSERT(listening)
}

OTX::BackgroundTask OTX::AcknowledgeBailment(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const Identifier& requestID,
    const std::string& instructions,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, targetNymID)
    CHECK_NYM(requestID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);
    auto time = std::time_t{0};
    auto serializedRequest = client_.Wallet().PeerRequest(
        nym->ID(), requestID, StorageBox::INCOMINGPEERREQUEST, time);

    if (false == bool(serializedRequest)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load request.").Flush();

        return error_task();
    }

    auto recipientNym = client_.Wallet().Nym(targetNymID);

    try {
        auto instantiatedRequest =
            client_.Factory().BailmentRequest(recipientNym, *serializedRequest);
        auto peerreply = client_.Factory().BailmentReply(
            nym,
            instantiatedRequest->Initiator(),
            requestID,
            serverID,
            instructions,
            reason_);

        if (setID) { setID(peerreply->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerReplyTask>(
            {targetNymID,
             peerreply.as<contract::peer::Reply>(),
             instantiatedRequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::AcknowledgeConnection(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& recipientID,
    const Identifier& requestID,
    const bool ack,
    const std::string& url,
    const std::string& login,
    const std::string& password,
    const std::string& key,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, recipientID)
    CHECK_NYM(requestID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    if (ack) {
        if (url.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: url is empty.")
                .Flush();
        }

        if (login.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: login is empty.")
                .Flush();
        }

        if (password.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: password is empty.")
                .Flush();
        }

        if (key.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: key is empty.")
                .Flush();
        }
    }

    auto time = std::time_t{0};
    auto serializedRequest = client_.Wallet().PeerRequest(
        nym->ID(), requestID, StorageBox::INCOMINGPEERREQUEST, time);

    if (false == bool(serializedRequest)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load request.").Flush();

        return error_task();
    }

    auto recipientNym = client_.Wallet().Nym(recipientID);

    try {
        auto instantiatedRequest =
            client_.Factory().BailmentRequest(recipientNym, *serializedRequest);
        auto peerreply = client_.Factory().ConnectionReply(
            nym,
            instantiatedRequest->Initiator(),
            requestID,
            serverID,
            ack,
            url,
            login,
            password,
            key,
            reason_);

        if (setID) { setID(peerreply->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerReplyTask>(
            {recipientID,
             peerreply.as<contract::peer::Reply>(),
             instantiatedRequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::AcknowledgeNotice(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& recipientID,
    const Identifier& requestID,
    const bool ack,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, recipientID)
    CHECK_NYM(requestID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);
    std::time_t time{0};
    auto serializedRequest = client_.Wallet().PeerRequest(
        nym->ID(), requestID, StorageBox::INCOMINGPEERREQUEST, time);

    if (false == bool(serializedRequest)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load request.").Flush();

        return error_task();
    }

    auto recipientNym = client_.Wallet().Nym(recipientID);

    try {
        auto instantiatedRequest =
            client_.Factory().BailmentRequest(recipientNym, *serializedRequest);
        auto peerreply = client_.Factory().ReplyAcknowledgement(
            nym,
            instantiatedRequest->Initiator(),
            requestID,
            serverID,
            instantiatedRequest->Type(),
            ack,
            reason_);

        if (setID) { setID(peerreply->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerReplyTask>(
            {recipientID,
             peerreply.as<contract::peer::Reply>(),
             instantiatedRequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::AcknowledgeOutbailment(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& recipientID,
    const Identifier& requestID,
    const std::string& details,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, recipientID)
    CHECK_NYM(requestID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);
    std::time_t time{0};
    auto serializedRequest = client_.Wallet().PeerRequest(
        nym->ID(), requestID, StorageBox::INCOMINGPEERREQUEST, time);

    if (false == bool(serializedRequest)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load request.").Flush();

        return error_task();
    }

    auto recipientNym = client_.Wallet().Nym(recipientID);

    try {
        auto instantiatedRequest =
            client_.Factory().BailmentRequest(recipientNym, *serializedRequest);
        auto peerreply = client_.Factory().OutbailmentReply(
            nym,
            instantiatedRequest->Initiator(),
            requestID,
            serverID,
            details,
            reason_);

        if (setID) { setID(peerreply->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerReplyTask>(
            {recipientID,
             peerreply.as<contract::peer::Reply>(),
             instantiatedRequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::add_task(
    const TaskID taskID,
    const ThreadStatus status,
    Finish finish /*={}*/) const
{
    Lock lock(task_status_lock_);

    if (0 != task_status_.count(taskID)) { return error_task(); }

    std::promise<Result> promise{};
    [[maybe_unused]] auto [it, added] = task_status_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(taskID),
        std::forward_as_tuple(status, std::move(promise), finish));

    OT_ASSERT(added);

    return {it->first, std::get<1>(it->second).get_future()};
}

void OTX::associate_message_id(const Identifier& messageID, const TaskID taskID)
    const
{
    Lock lock(task_status_lock_);
    task_message_id_.emplace(taskID, messageID);
}

Depositability OTX::can_deposit(
    const OTPayment& payment,
    const identifier::Nym& recipient,
    const Identifier& accountIDHint,
    identifier::Server& depositServer,
    identifier::UnitDefinition& unitID,
    Identifier& depositAccount) const
{
    auto nymID = identifier::Nym::Factory();

    if (false == extract_payment_data(payment, nymID, depositServer, unitID)) {

        return Depositability::INVALID_INSTRUMENT;
    }

    auto output = valid_recipient(payment, nymID, recipient);

    if (Depositability::WRONG_RECIPIENT == output) { return output; }

    const bool registered =
        client_.OTAPI().IsNym_RegisteredAtServer(recipient, depositServer);

    if (false == registered) {
        schedule_download_nymbox(recipient, depositServer);
        LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient nym ")(recipient)(
            " not registered on server ")(depositServer)(".")
            .Flush();

        return Depositability::NOT_REGISTERED;
    }

    output = valid_account(
        payment,
        recipient,
        depositServer,
        unitID,
        accountIDHint,
        depositAccount);

    switch (output) {
        case Depositability::ACCOUNT_NOT_SPECIFIED: {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Multiple valid accounts exist. "
                "This payment can not be automatically deposited.")
                .Flush();
        } break;
        case Depositability::WRONG_ACCOUNT: {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": The specified account is not valid for this payment.")
                .Flush();
        } break;
        case Depositability::NO_ACCOUNT: {

            LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient ")(recipient)(
                " needs an account for ")(unitID)(" on server ")(depositServer)(
                ".")
                .Flush();
        } break;
        case Depositability::READY: {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Payment can be deposited.")
                .Flush();
        } break;
        default: {
            OT_FAIL
        }
    }

    return output;
}

Messagability OTX::can_message(
    const identifier::Nym& senderNymID,
    const Identifier& recipientContactID,
    identifier::Nym& recipientNymID,
    identifier::Server& serverID) const
{
    auto senderNym = client_.Wallet().Nym(senderNymID);

    if (false == bool(senderNym)) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Unable to load sender nym ")(
            senderNymID)
            .Flush();

        return Messagability::MISSING_SENDER;
    }

    const bool canSign = senderNym->HasCapability(NymCapability::SIGN_MESSAGE);

    if (false == canSign) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Sender nym ")(senderNymID)(
            " can not sign messages (no private key).")
            .Flush();

        return Messagability::INVALID_SENDER;
    }

    const auto contact = client_.Contacts().Contact(recipientContactID);

    if (false == bool(contact)) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient contact ")(
            recipientContactID)(" does not exist.")
            .Flush();

        return Messagability::MISSING_CONTACT;
    }

    const auto nyms = contact->Nyms();

    if (0 == nyms.size()) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient contact ")(
            recipientContactID)(" does not have a nym.")
            .Flush();

        return Messagability::CONTACT_LACKS_NYM;
    }

    Nym_p recipientNym{nullptr};

    for (const auto& it : nyms) {
        recipientNym = client_.Wallet().Nym(it);

        if (recipientNym) {
            recipientNymID.Assign(it);
            break;
        }
    }

    if (false == bool(recipientNym)) {
        for (const auto& nymID : nyms) {
            outdated_nyms_.Push(next_task_id(), nymID);
        }

        LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient contact ")(
            recipientContactID)(" credentials not available.")
            .Flush();

        return Messagability::MISSING_RECIPIENT;
    }

    OT_ASSERT(recipientNym)

    const auto& claims = recipientNym->Claims();
    serverID.Assign(claims.PreferredOTServer());

    // TODO maybe some of the other nyms in this contact do specify a server
    if (serverID.empty()) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Recipient contact ")(
            recipientContactID)(", nym ")(recipientNymID)(
            ": credentials do not specify a server.")
            .Flush();
        outdated_nyms_.Push(next_task_id(), recipientNymID);

        return Messagability::NO_SERVER_CLAIM;
    }

    const bool registered =
        client_.OTAPI().IsNym_RegisteredAtServer(senderNymID, serverID);

    if (false == registered) {
        schedule_download_nymbox(senderNymID, serverID);
        LogDetail(OT_METHOD)(__FUNCTION__)(": Sender nym ")(senderNymID)(
            " not registered on server ")(serverID)
            .Flush();

        return Messagability::UNREGISTERED;
    }

    return Messagability::READY;
}

Depositability OTX::CanDeposit(
    const identifier::Nym& recipientNymID,
    const OTPayment& payment) const
{
    auto accountHint = Identifier::Factory();

    return CanDeposit(recipientNymID, accountHint, payment);
}

Depositability OTX::CanDeposit(
    const identifier::Nym& recipientNymID,
    const Identifier& accountIDHint,
    const OTPayment& payment) const
{
    auto serverID = identifier::Server::Factory();
    auto unitID = identifier::UnitDefinition::Factory();
    auto accountID = Identifier::Factory();

    return can_deposit(
        payment, recipientNymID, accountIDHint, serverID, unitID, accountID);
}

Messagability OTX::CanMessage(
    const identifier::Nym& senderNymID,
    const Identifier& recipientContactID,
    const bool startIntroductionServer) const
{
    if (startIntroductionServer) { start_introduction_server(senderNymID); }

    if (senderNymID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid sender.").Flush();

        return Messagability::INVALID_SENDER;
    }

    if (recipientContactID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid recipient.").Flush();

        return Messagability::MISSING_CONTACT;
    }

    auto nymID = identifier::Nym::Factory();
    auto serverID = identifier::Server::Factory();

    return can_message(senderNymID, recipientContactID, nymID, serverID);
}

bool OTX::CheckTransactionNumbers(
    const identifier::Nym& nym,
    const identifier::Server& serverID,
    const std::size_t quantity) const
{
    auto context = client_.Wallet().ServerContext(nym, serverID);

    if (false == bool(context)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym is not registered").Flush();

        return false;
    }

    const auto available = context->AvailableNumbers();

    if (quantity <= available) { return true; }

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Asking server for more numbers.")
        .Flush();

    try {
        auto& queue = get_operations({nym, serverID});
        const auto output =
            queue.StartTask<otx::client::GetTransactionNumbersTask>({});
        const auto& taskID = output.first;

        if (0 == taskID) { return false; }

        auto status = Status(taskID);

        while (ThreadStatus::RUNNING == status) {
            Sleep(std::chrono::milliseconds(100));
            status = Status(taskID);
        }

        if (ThreadStatus::FINISHED_SUCCESS == status) { return true; }

        return false;
    } catch (...) {

        return false;
    }
}

OTX::Finished OTX::ContextIdle(
    const identifier::Nym& nym,
    const identifier::Server& server) const
{
    try {
        auto& queue = get_operations({nym, server});

        return queue.Wait();
    } catch (...) {
        std::promise<void> empty{};
        auto output = empty.get_future();
        empty.set_value();

        return std::move(output);
    }
}

std::size_t OTX::DepositCheques(const identifier::Nym& nymID) const
{
    std::size_t output{0};
    const auto workflows = client_.Workflow().List(
        nymID,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED);

    for (const auto& id : workflows) {
        const auto chequeState =
            client_.Workflow().LoadChequeByWorkflow(nymID, id);
        const auto& [state, cheque] = chequeState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(cheque)

        if (queue_cheque_deposit(nymID, *cheque)) { ++output; }
    }

    return output;
}

std::size_t OTX::DepositCheques(
    const identifier::Nym& nymID,
    const std::set<OTIdentifier>& chequeIDs) const
{
    std::size_t output{0};

    if (chequeIDs.empty()) { return DepositCheques(nymID); }

    for (const auto& id : chequeIDs) {
        const auto chequeState = client_.Workflow().LoadCheque(nymID, id);
        const auto& [state, cheque] = chequeState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(cheque)

        if (queue_cheque_deposit(nymID, *cheque)) { ++output; }
    }

    return {};
}

std::size_t OTX::PayInvoices(const identifier::Nym& nymID) const
{
    std::size_t output{0};
    const auto workflows = client_.Workflow().List(
        nymID,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED);

    for (const auto& id : workflows) {
        const auto invoiceState =
            client_.Workflow().LoadInvoiceByWorkflow(nymID, id, reason_);
        const auto& [state, invoice] = invoiceState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(invoice)

        if (queue_cheque_deposit(nymID, *invoice)) { ++output; }
    }

    return output;
}

std::size_t OTX::PayInvoices(
    const identifier::Nym& nymID,
    const std::set<OTIdentifier>& invoiceIDs) const
{
    std::size_t output{0};

    if (invoiceIDs.empty()) { return PayInvoices(nymID); }

    for (const auto& id : invoiceIDs) {
        const auto invoiceState =
            client_.Workflow().LoadInvoice(nymID, id, reason_);
        const auto& [state, invoice] = invoiceState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(invoice)

        if (queue_cheque_deposit(nymID, *invoice)) { ++output; }
    }

    return {};
}

std::size_t OTX::DepositVouchers(const identifier::Nym& nymID) const
{
    std::size_t output{0};
    const auto workflows = client_.Workflow().List(
        nymID,
        proto::PAYMENTWORKFLOWTYPE_INCOMINGVOUCHER,
        proto::PAYMENTWORKFLOWSTATE_CONVEYED);

    for (const auto& id : workflows) {
        const auto voucherState =
            client_.Workflow().LoadVoucherByWorkflow(nymID, id, reason_);
        const auto& [state, voucher] = voucherState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(voucher)

        if (queue_cheque_deposit(nymID, *voucher)) { ++output; }
    }

    return output;
}

std::size_t OTX::DepositVouchers(
    const identifier::Nym& nymID,
    const std::set<OTIdentifier>& voucherIDs) const
{
    std::size_t output{0};

    if (voucherIDs.empty()) { return DepositVouchers(nymID); }

    for (const auto& id : voucherIDs) {
        const auto voucherState =
            client_.Workflow().LoadVoucher(nymID, id, reason_);
        const auto& [state, voucher] = voucherState;

        if (proto::PAYMENTWORKFLOWSTATE_CONVEYED != state) { continue; }

        OT_ASSERT(voucher)

        if (queue_cheque_deposit(nymID, *voucher)) { ++output; }
    }

    return {};
}

OTX::BackgroundTask OTX::DepositPayment(
    const identifier::Nym& recipientNymID,
    const std::shared_ptr<const OTPayment>& payment) const
{
    auto notUsed = Identifier::Factory();

    return DepositPayment(recipientNymID, notUsed, payment);
}

OTX::BackgroundTask OTX::DepositPayment(
    const identifier::Nym& recipientNymID,
    const Identifier& accountIDHint,
    const std::shared_ptr<const OTPayment>& payment) const
{
    OT_ASSERT(payment)

    if (recipientNymID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid recipient.").Flush();

        return error_task();
    }

    auto serverID = identifier::Server::Factory();
    auto unitID = identifier::UnitDefinition::Factory();
    auto accountID = Identifier::Factory();
    const auto status = can_deposit(
        *payment, recipientNymID, accountIDHint, serverID, unitID, accountID);

    try {
        switch (status) {
            case Depositability::READY:
            case Depositability::NOT_REGISTERED:
            case Depositability::NO_ACCOUNT: {
                start_introduction_server(recipientNymID);
                auto& queue = get_operations({recipientNymID, serverID});

                return queue.payment_tasks_.Queue({unitID, accountID, payment});
            }
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Unable to queue payment for download (")(
                    static_cast<std::int8_t>(status))(")")
                    .Flush();

                return error_task();
            }
        }
    } catch (...) {
        return error_task();
    }
}

void OTX::DisableAutoaccept() const { auto_process_inbox_->Off(); }

#if OT_CASH
OTX::BackgroundTask OTX::DownloadMint(
    const identifier::Nym& nym,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit) const
{
    CHECK_ARGS(nym, server, unit);

    try {
        start_introduction_server(nym);
        auto& queue = get_operations({nym, server});

        return queue.StartTask<otx::client::DownloadMintTask>({unit, 0});
    } catch (...) {

        return error_task();
    }
}
#endif

OTX::BackgroundTask OTX::DownloadNym(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID) const
{
    CHECK_ARGS(localNymID, serverID, targetNymID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::CheckNymTask>(targetNymID);
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::DownloadNymbox(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID) const
{
    return schedule_download_nymbox(localNymID, serverID);
}

OTX::BackgroundTask OTX::DownloadServerContract(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Server& contractID) const
{
    CHECK_ARGS(localNymID, serverID, contractID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::DownloadContractTask>({contractID});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::DownloadUnitDefinition(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& contractID) const
{
    CHECK_ARGS(localNymID, serverID, contractID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::DownloadUnitDefinitionTask>(
            {contractID});
    } catch (...) {

        return error_task();
    }
}

bool OTX::extract_payment_data(
    const OTPayment& payment,
    identifier::Nym& nymID,
    identifier::Server& serverID,
    identifier::UnitDefinition& unitID) const
{
    if (false == payment.GetRecipientNymID(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load recipient nym from instrument.")
            .Flush();

        return false;
    }

    if (false == payment.GetNotaryID(serverID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load recipient nym from instrument.")
            .Flush();

        return false;
    }

    OT_ASSERT(false == serverID.empty())

    if (false == payment.GetInstrumentDefinitionID(unitID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load recipient nym from instrument.")
            .Flush();

        return false;
    }

    OT_ASSERT(false == unitID.empty())

    return true;
}

OTX::BackgroundTask OTX::error_task()
{
    BackgroundTask output{0, Future{}};

    return output;
}

void OTX::find_nym(const opentxs::network::zeromq::Message& message) const
{
    const auto body = message.Body();

    if (1 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

        return;
    }

    auto id = identifier::Nym::Factory();
    id->SetString(body.at(0));

    if (id->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid id").Flush();

        return;
    }

    const auto taskID{next_task_id()};
    missing_nyms_.Push(taskID, id);
    trigger_all();
}

void OTX::find_server(const opentxs::network::zeromq::Message& message) const
{
    const auto body = message.Body();

    if (1 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

        return;
    }

    auto id = identifier::Server::Factory();
    id->SetString(body.at(0));

    if (id->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid id").Flush();

        return;
    }

    try {
        client_.Wallet().Server(id);
    } catch (...) {
        const auto taskID{next_task_id()};
        missing_servers_.Push(taskID, id);
        trigger_all();
    }
}

void OTX::find_unit(const opentxs::network::zeromq::Message& message) const
{
    const auto body = message.Body();

    if (1 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

        return;
    }

    auto id = identifier::UnitDefinition::Factory();
    id->SetString(body.at(0));

    if (id->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid id").Flush();

        return;
    }

    try {
        client_.Wallet().UnitDefinition(id);

        return;
    } catch (...) {
        const auto taskID{next_task_id()};
        missing_unit_definitions_.Push(taskID, id);
        trigger_all();
    }
}

OTX::BackgroundTask OTX::FindNym(const identifier::Nym& nymID) const
{
    CHECK_NYM(nymID)

    const auto taskID{next_task_id()};
    auto output = start_task(taskID, missing_nyms_.Push(taskID, nymID));
    trigger_all();

    return output;
}

OTX::BackgroundTask OTX::FindNym(
    const identifier::Nym& nymID,
    const identifier::Server& serverIDHint) const
{
    CHECK_NYM(nymID)

    auto& serverQueue = get_nym_fetch(serverIDHint);
    const auto taskID{next_task_id()};
    auto output = start_task(taskID, serverQueue.Push(taskID, nymID));

    return output;
}

OTX::BackgroundTask OTX::FindServer(const identifier::Server& serverID) const
{
    CHECK_NYM(serverID)

    const auto taskID{next_task_id()};
    auto output = start_task(taskID, missing_servers_.Push(taskID, serverID));

    return output;
}

OTX::BackgroundTask OTX::FindUnitDefinition(
    const identifier::UnitDefinition& unit) const
{
    CHECK_NYM(unit)

    const auto taskID{next_task_id()};
    auto output =
        start_task(taskID, missing_unit_definitions_.Push(taskID, unit));

    return output;
}

bool OTX::finish_task(const TaskID taskID, const bool success, Result&& result)
    const
{
    if (success) {
        update_task(taskID, ThreadStatus::FINISHED_SUCCESS, std::move(result));
    } else {
        update_task(taskID, ThreadStatus::FINISHED_FAILED, std::move(result));
    }

    return success;
}

OTServerID OTX::get_introduction_server(const Lock& lock) const
{
    OT_ASSERT(CheckLock(lock, introduction_server_lock_))

    bool keyFound = false;
    auto serverID = String::Factory();
    const bool config = client_.Config().Check_str(
        String::Factory(MASTER_SECTION),
        String::Factory(INTRODUCTION_SERVER_KEY),
        serverID,
        keyFound);

    if (!config || !keyFound || !serverID->Exists()) {

        return import_default_introduction_server(lock);
    }

    return identifier::Server::Factory(serverID);
}

UniqueQueue<OTNymID>& OTX::get_nym_fetch(
    const identifier::Server& serverID) const
{
    Lock lock(nym_fetch_lock_);

    return server_nym_fetch_[serverID];
}

otx::client::implementation::StateMachine& OTX::get_operations(
    const ContextID& id) const noexcept(false)
{
    Lock lock(shutdown_lock_);

    if (shutdown_.load()) { throw; }

    return get_task(id);
}

otx::client::implementation::StateMachine& OTX::get_task(
    const ContextID& id) const
{
    Lock lock(lock_);
    auto it = operations_.find(id);

    if (operations_.end() == it) {
        auto added = operations_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(
                client_,
                *this,
                running_,
                client_,
                id,
                next_task_id_,
                missing_nyms_,
                outdated_nyms_,
                missing_servers_,
                missing_unit_definitions_,
                reason_));
        it = std::get<0>(added);
    }

    return it->second;
}

OTServerID OTX::import_default_introduction_server(const Lock& lock) const
{
    OT_ASSERT(CheckLock(lock, introduction_server_lock_))

    const auto serialized = proto::StringToProto<proto::ServerContract>(
        String::Factory(DEFAULT_INTRODUCTION_SERVER.c_str()));

    return set_introduction_server(lock, client_.Wallet().Server(serialized));
}

OTX::BackgroundTask OTX::InitiateBailment(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const identifier::UnitDefinition& instrumentDefinitionID,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, instrumentDefinitionID)
    CHECK_NYM(targetNymID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    try {
        auto peerrequest = client_.Factory().BailmentRequest(
            nym, targetNymID, instrumentDefinitionID, serverID, reason_);

        if (setID) { setID(peerrequest->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerRequestTask>(
            {targetNymID, peerrequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::InitiateOutbailment(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const identifier::UnitDefinition& instrumentDefinitionID,
    const Amount amount,
    const std::string& message,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, instrumentDefinitionID)
    CHECK_NYM(targetNymID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    try {
        auto peerrequest = client_.Factory().OutbailmentRequest(
            nym,
            targetNymID,
            instrumentDefinitionID,
            serverID,
            amount,
            message,
            reason_);

        if (setID) { setID(peerrequest->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerRequestTask>(
            {targetNymID, peerrequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::InitiateRequestConnection(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const proto::ConnectionInfoType& type,
    const SetID setID) const
{
    CHECK_SERVER(localNymID, serverID)
    CHECK_NYM(targetNymID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    try {
        auto peerrequest = client_.Factory().ConnectionRequest(
            nym, targetNymID, type, serverID, reason_);

        if (setID) { setID(peerrequest->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerRequestTask>(
            {targetNymID, peerrequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::InitiateStoreSecret(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const proto::SecretType& type,
    const std::string& primary,
    const std::string& secondary,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, targetNymID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    if (primary.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: primary is empty.")
            .Flush();
    }

    if (secondary.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: secondary is empty.")
            .Flush();
    }

    try {
        auto peerrequest = client_.Factory().StoreSecret(
            nym, targetNymID, type, primary, secondary, serverID, reason_);

        if (setID) { setID(peerrequest->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerRequestTask>(
            {targetNymID, peerrequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

const identifier::Server& OTX::IntroductionServer() const
{
    Lock lock(introduction_server_lock_);

    if (false == bool(introduction_server_id_)) {
        load_introduction_server(lock);
    }

    OT_ASSERT(introduction_server_id_)

    return *introduction_server_id_;
}

OTX::BackgroundTask OTX::IssueUnitDefinition(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& unitID,
    const proto::ContactItemType advertise,
    const std::string& label) const
{
    CHECK_ARGS(localNymID, serverID, unitID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::IssueUnitDefinitionTask>(
            {unitID, label, advertise});
    } catch (...) {

        return error_task();
    }
}

void OTX::load_introduction_server(const Lock& lock) const
{
    OT_ASSERT(CheckLock(lock, introduction_server_lock_))

    introduction_server_id_.reset(
        new OTServerID(get_introduction_server(lock)));
}

OTX::BackgroundTask OTX::MessageContact(
    const identifier::Nym& senderNymID,
    const Identifier& contactID,
    const std::string& message,
    const SetID setID) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) { return error_task(); }

    OT_ASSERT(false == serverID->empty())
    OT_ASSERT(false == recipientNymID->empty())

    try {
        auto& queue = get_operations({senderNymID, serverID});

        return queue.StartTask<otx::client::MessageTask>(
            {recipientNymID, message, std::make_shared<SetID>(setID)});
    } catch (...) {

        return error_task();
    }
}

std::pair<ThreadStatus, OTX::MessageID> OTX::MessageStatus(
    const TaskID taskID) const
{
    std::pair<ThreadStatus, MessageID> output{{}, Identifier::Factory()};
    auto& [threadStatus, messageID] = output;
    Lock lock(task_status_lock_);
    threadStatus = status(lock, taskID);

    if (threadStatus == ThreadStatus::FINISHED_SUCCESS) {
        auto it = task_message_id_.find(taskID);

        if (task_message_id_.end() != it) {
            messageID = it->second;
            task_message_id_.erase(it);
        }
    }

    return output;
}

OTX::BackgroundTask OTX::NotifyBailment(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::Nym& targetNymID,
    const identifier::UnitDefinition& instrumentDefinitionID,
    const Identifier& requestID,
    const std::string& txid,
    const Amount amount,
    const SetID setID) const
{
    CHECK_ARGS(localNymID, serverID, instrumentDefinitionID)
    CHECK_NYM(targetNymID)
    CHECK_NYM(requestID)

    start_introduction_server(localNymID);
    const auto nym = client_.Wallet().Nym(localNymID);

    try {
        auto peerrequest = client_.Factory().BailmentNotice(
            nym,
            targetNymID,
            instrumentDefinitionID,
            serverID,
            requestID,
            txid,
            amount,
            reason_);

        if (setID) { setID(peerrequest->ID()); }

        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::PeerRequestTask>(
            {targetNymID, peerrequest.as<contract::peer::Request>()});
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return error_task();
    }
}

OTX::BackgroundTask OTX::PayContact(
    const identifier::Nym& senderNymID,
    const Identifier& contactID,
    std::shared_ptr<const OTPayment> payment) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) { return error_task(); }

    OT_ASSERT(false == serverID->empty())
    OT_ASSERT(false == recipientNymID->empty())

    try {
        auto& queue = get_operations({senderNymID, serverID});

        return queue.StartTask<otx::client::PaymentTask>(
            {recipientNymID, std::shared_ptr<const OTPayment>(payment)});
    } catch (...) {

        return error_task();
    }
}

#if OT_CASH
OTX::BackgroundTask OTX::PayContactCash(
    const identifier::Nym& senderNymID,
    const Identifier& contactID,
    const Identifier& workflowID) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) { return error_task(); }

    OT_ASSERT(false == serverID->empty())
    OT_ASSERT(false == recipientNymID->empty())

    try {
        auto& queue = get_operations({senderNymID, serverID});

        return queue.StartTask<otx::client::PayCashTask>(
            {recipientNymID, workflowID});
    } catch (...) {

        return error_task();
    }
}
#endif  // OT_CASH

OTX::BackgroundTask OTX::ProcessInbox(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const Identifier& accountID) const
{
    CHECK_ARGS(localNymID, serverID, accountID)

    try {
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::ProcessInboxTask>({accountID});
    } catch (...) {

        return error_task();
    }
}

void OTX::process_account(const zmq::Message& message) const
{
    OT_ASSERT(2 == message.Body().size())

    const std::string id(*message.Body().begin());
    const auto& balance = message.Body().at(1);

    OT_ASSERT(balance.size() == sizeof(Amount))

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Account ")(id)(" balance: ")(
        *static_cast<const Amount*>(balance.data()))
        .Flush();
}

void OTX::process_notification(const zmq::Message& message) const
{
    OT_ASSERT(0 < message.Body().size())

    const auto& frame = message.Body().at(0);
    const auto notification =
        otx::Reply::Factory(client_, proto::Factory<proto::ServerReply>(frame));
    const auto& nymID = notification->Recipient();
    const auto& serverID = notification->Server();

    if (false == valid_context(nymID, serverID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": No context available to handle notification.")
            .Flush();

        return;
    }

    auto context =
        client_.Wallet().mutable_ServerContext(nymID, serverID, reason_);

    switch (notification->Type()) {
        case proto::SERVERREPLY_PUSH: {
            context.get().ProcessNotification(client_, notification, reason_);
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unsupported server reply type: ")(notification->Type())(".")
                .Flush();
        }
    }
}

bool OTX::publish_server_registration(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const bool forcePrimary) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    auto nym = client_.Wallet().mutable_Nym(nymID, reason_);

    return nym.AddPreferredOTServer(serverID.str(), forcePrimary, reason_);
}

OTX::BackgroundTask OTX::PublishServerContract(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const Identifier& contractID) const
{
    CHECK_ARGS(localNymID, serverID, contractID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});
        // TODO server id type

        return queue.StartTask<otx::client::PublishServerContractTask>(
            {client_.Factory().ServerID(contractID.str()), false});
    } catch (...) {

        return error_task();
    }
}

bool OTX::queue_cheque_deposit(
    const identifier::Nym& nymID,
    const Cheque& cheque) const
{
    auto payment{client_.Factory().Payment(String::Factory(cheque))};

    OT_ASSERT(false != bool(payment));

    payment->SetTempValuesFromCheque(cheque);

    if (cheque.GetRecipientNymID().empty()) {
        payment->SetTempRecipientNymID(nymID);
    }

    std::shared_ptr<OTPayment> ppayment{payment.release()};
    const auto task = DepositPayment(nymID, ppayment);
    const auto taskID = std::get<0>(task);

    return (0 != taskID);
}

void OTX::Refresh() const
{
    refresh_accounts();
    refresh_contacts();
    ++refresh_counter_;
    trigger_all();
}

std::uint64_t OTX::RefreshCount() const { return refresh_counter_.load(); }

bool OTX::refresh_accounts() const
{
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Begin").Flush();
    const auto serverList = client_.Wallet().ServerList();
    const auto accounts = client_.Storage().AccountList();

    for (const auto& server : serverList) {
        SHUTDOWN()

        const auto serverID = identifier::Server::Factory(server.first);
        LogDetail(OT_METHOD)(__FUNCTION__)(": Considering server ")(serverID)
            .Flush();

        for (const auto& nymID : client_.Wallet().LocalNyms()) {
            SHUTDOWN()
            auto logStr = String::Factory(": Nym ");
            logStr->Concatenate("%s", nymID->str().c_str());
            const bool registered =
                client_.OTAPI().IsNym_RegisteredAtServer(nymID, serverID);

            if (registered) {
                logStr->Concatenate(" %s ", "is");
                try {
                    auto& queue = get_operations({nymID, serverID});
                    queue.StartTask<otx::client::DownloadNymboxTask>({});
                } catch (...) {

                    return false;
                }
            } else {
                logStr->Concatenate(" %s ", "is not");
            }

            logStr->Concatenate("%s", " registered here.");
            LogDetail(OT_METHOD)(__FUNCTION__)(logStr).Flush();
        }
    }

    SHUTDOWN()

    for (const auto& it : accounts) {
        SHUTDOWN()
        const auto accountID = Identifier::Factory(it.first);
        const auto nymID = client_.Storage().AccountOwner(accountID);
        const auto serverID = client_.Storage().AccountServer(accountID);
        LogDetail(OT_METHOD)(__FUNCTION__)(": Account ")(accountID)(": ")(
            "  * Owned by nym: ")(nymID)("  * On server: ")(serverID)
            .Flush();
        try {
            auto& queue = get_operations({nymID, serverID});

            if (0 == queue.StartTask<otx::client::ProcessInboxTask>({accountID})
                         .first) {

                return false;
            }
        } catch (...) {

            return false;
        }
    }

    LogVerbose(OT_METHOD)(__FUNCTION__)(": End").Flush();

    return true;
}

bool OTX::refresh_contacts() const
{
    for (const auto& it : client_.Contacts().ContactList()) {
        SHUTDOWN()

        const auto& contactID = it.first;
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Considering contact: ")(
            contactID)
            .Flush();
        const auto contact =
            client_.Contacts().Contact(Identifier::Factory(contactID));

        OT_ASSERT(contact);

        const auto now = std::time(nullptr);
        const std::chrono::seconds interval(now - contact->LastUpdated());
        const std::chrono::hours limit(24 * CONTACT_REFRESH_DAYS);
        const auto nymList = contact->Nyms();

        if (nymList.empty()) {
            LogVerbose(OT_METHOD)(__FUNCTION__)(
                ": No nyms associated with this contact.")
                .Flush();

            continue;
        }

        for (const auto& nymID : nymList) {
            SHUTDOWN()

            const auto nym = client_.Wallet().Nym(nymID);
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Considering nym: ")(nymID)
                .Flush();

            if (nym) {
                client_.Contacts().Update(nym->asPublicNym());
            } else {
                LogVerbose(OT_METHOD)(__FUNCTION__)(
                    ": We don't have credentials for this nym. "
                    " Will search on all servers.")
                    .Flush();
                const auto taskID{next_task_id()};
                missing_nyms_.Push(taskID, nymID);

                continue;
            }

            if (interval > limit) {
                LogVerbose(OT_METHOD)(__FUNCTION__)(
                    ": Hours since last update (")(interval.count())(
                    ") exceeds the limit (")(limit.count())(")")
                    .Flush();
                // TODO add a method to Contact that returns the list of
                // servers
                const auto data = contact->Data();

                if (false == bool(data)) { continue; }

                const auto serverGroup = data->Group(
                    proto::CONTACTSECTION_COMMUNICATION,
                    proto::CITEMTYPE_OPENTXS);

                if (false == bool(serverGroup)) {

                    const auto taskID{next_task_id()};
                    outdated_nyms_.Push(taskID, nymID);
                    continue;
                }

                for (const auto& [claimID, item] : *serverGroup) {
                    SHUTDOWN()
                    OT_ASSERT(item)

                    const auto& notUsed [[maybe_unused]] = claimID;
                    const auto serverID =
                        identifier::Server::Factory(item->Value());

                    if (serverID->empty()) { continue; }

                    LogVerbose(OT_METHOD)(__FUNCTION__)(": Will download nym ")(
                        nymID)(" from server ")(serverID)
                        .Flush();
                    auto& serverQueue = get_nym_fetch(serverID);
                    const auto taskID{next_task_id()};
                    serverQueue.Push(taskID, nymID);
                }
            } else {
                LogVerbose(OT_METHOD)(__FUNCTION__)(
                    ": No need to update this nym.")
                    .Flush();
            }
        }
    }

    return true;
}

OTX::BackgroundTask OTX::RegisterAccount(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& unitID,
    const std::string& label) const
{
    return schedule_register_account(localNymID, serverID, unitID, label);
}

OTX::BackgroundTask OTX::RegisterNym(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const bool resync) const
{
    CHECK_SERVER(localNymID, serverID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::RegisterNymTask>({resync});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::RegisterNymPublic(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const bool setContactData,
    const bool forcePrimary,
    const bool resync) const
{
    CHECK_SERVER(nymID, serverID)

    start_introduction_server(nymID);

    if (setContactData) {
        publish_server_registration(nymID, serverID, forcePrimary);
    }

    return RegisterNym(nymID, serverID, resync);
}

OTServerID OTX::SetIntroductionServer(const contract::Server& contract) const
{
    Lock lock(introduction_server_lock_);

    return set_introduction_server(lock, contract);
}

OTX::BackgroundTask OTX::schedule_download_nymbox(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID) const
{
    CHECK_SERVER(localNymID, serverID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::DownloadNymboxTask>({});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::schedule_register_account(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& unitID,
    const std::string& label) const
{
    CHECK_ARGS(localNymID, serverID, unitID)

    try {
        start_introduction_server(localNymID);
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::RegisterAccountTask>(
            {label, unitID});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::SendCheque(
    const identifier::Nym& localNymID,
    const Identifier& sourceAccountID,
    const Identifier& recipientContactID,
    const Amount value,
    const std::string& memo,
    const Time validFrom,
    const Time validTo) const
{
    CHECK_ARGS(localNymID, sourceAccountID, recipientContactID)

    start_introduction_server(localNymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(localNymID, recipientContactID, recipientNymID, serverID);
    const bool closeEnough = (Messagability::READY == canMessage) ||
                             (Messagability::UNREGISTERED == canMessage);

    if (false == closeEnough) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to message contact.")
            .Flush();

        return error_task();
    }

    if (0 >= value) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid amount.").Flush();

        return error_task();
    }

    auto account = client_.Wallet().Account(sourceAccountID);

    if (false == bool(account)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return error_task();
    }

    try {
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::SendChequeTask>(
            {sourceAccountID, recipientNymID, value, memo, validFrom, validTo});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::SendInvoice(
    const identifier::Nym& localNymID,
    const Identifier& sourceAccountID,
    const Identifier& recipientContactID,
    const Amount value,
    const std::string& memo,
    const Time validFrom,
    const Time validTo) const
{
    CHECK_ARGS(localNymID, sourceAccountID, recipientContactID)

    start_introduction_server(localNymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(localNymID, recipientContactID, recipientNymID, serverID);
    const bool closeEnough = (Messagability::READY == canMessage) ||
                             (Messagability::UNREGISTERED == canMessage);

    if (false == closeEnough) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to message contact.")
            .Flush();

        return error_task();
    }

    if (0 >= value) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid amount.").Flush();

        return error_task();
    }

    auto account = client_.Wallet().Account(sourceAccountID, reason_);

    if (false == bool(account)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return error_task();
    }

    try {
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::SendInvoiceTask>(
            {sourceAccountID, recipientNymID, value, memo, validFrom, validTo});
    } catch (...) {

        return error_task();
    }
}

BackgroundTask OTX::WithdrawVoucher(
    const identifier::Nym& nymID,
    const Identifier& sourceAccountID,
    const Identifier& recipientContactID,
    const Amount value,
    const std::string& memo,
    const Time validFrom,
    const Time validTo) const
{
    CHECK_ARGS(nymID, sourceAccountID, recipientContactID)

    start_introduction_server(nymID);
    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(nymID, recipientContactID, recipientNymID, serverID);
    const bool closeEnough = (Messagability::READY == canMessage) ||
                             (Messagability::UNREGISTERED == canMessage);

    if (false == closeEnough) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to message contact.")
            .Flush();

        return error_task();
    }

    if (0 >= value) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid amount.").Flush();

        return error_task();
    }

    auto account = client_.Wallet().Account(sourceAccountID, reason_);

    if (false == bool(account)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return error_task();
    }

    try {
        auto& queue = get_operations({nymID, serverID});

        return queue.StartTask<otx::client::WithdrawVoucherTask>(
            {sourceAccountID, recipientNymID, value, memo, validFrom, validTo});
    } catch (...) {

        return error_task();
    }
}

/*
 In the api:client:OTX class you must have added a function that starts off the
 process.

 Chris Odom, [Dec 23, 2019 at 9:30:56 PM]:
 yes I did

 Justus Ranvier, [Dec 23, 2019 at 9:31:08 PM]:
 That function returned a task id to the caller. That task id must be the one
 that is finished when the voucher is successfuly sent, or failed to send.

 Otherwise the caller will wait forever on a future whose value will never be
 set

 Chris Odom, [Dec 23, 2019 at 9:31:32 PM]:
 okay

 Justus Ranvier, [Dec 23, 2019 at 9:33:03 PM]:
 But basically if you want a high level "withdraw and send voucher" function,
 the first thing that has to happen is that you queue a withdraw voucher task.
 When that task is complete then you can queue a send message task.

 Chris Odom, [Dec 23, 2019 at 9:33:37 PM]:
 So there is a function that does both of those things

 just not in state machine?

 Justus Ranvier, [Dec 23, 2019 at 9:34:30 PM]:
 The ideal way to solve the problem would be if you could register a lambda with
 a task that gets called when the finish_task executes. If that doesn't already
 exist then it should be added.

 Chris Odom, [Dec 23, 2019 at 9:35:36 PM]:
 okay

 Justus Ranvier, [Dec 23, 2019 at 9:37:03 PM]:
 The OTX class is where you get the queue and push the withdraw_voucher task,
 right? At that spot would be the place to call a function that sets a lambda on
 that task. The lambda just needs the ability to look at the reply message and
 then push to the send_message queue

 Although this needs to be done carefully

 The send_message task should reuse the task id from the withdraw_voucher task,
 because that's the number the caller is looking at.
 */

OTX::BackgroundTask OTX::SendVoucher(
    const identifier::Nym& localNymID,
    const Identifier& sourceAccountID,
    const Identifier& recipientContactID,
    const Amount value,
    const std::string& memo,
    const Time validFrom,
    const Time validTo) const
{
    CHECK_ARGS(localNymID, sourceAccountID, recipientContactID)

    start_introduction_server(localNymID);

    auto serverID = identifier::Server::Factory();
    auto recipientNymID = identifier::Nym::Factory();
    const auto canMessage =
        can_message(localNymID, recipientContactID, recipientNymID, serverID);
    const bool closeEnough = (Messagability::READY == canMessage) ||
                             (Messagability::UNREGISTERED == canMessage);

    if (false == closeEnough) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to message contact.")
            .Flush();

        return error_task();
    }

    if (0 >= value) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid amount.").Flush();

        return error_task();
    }

    auto account = client_.Wallet().Account(sourceAccountID, reason_);

    if (false == bool(account)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return error_task();
    }
    // ------------------------------------------------------------
    //  using Finish = std::function<bool(const TaskID, const bool, Result&&)>;
    auto finish = [&, this, workflowID, localNymID, reason_](
                      const auto taskID,
                      const auto success,
                      const auto&& result) -> bool {
        // lookup the workflow by id
        //
        // using Cheque = std::pair<proto::PaymentWorkflowState,
        // std::unique_ptr<opentxs::Cheque>>;
        //
        const auto voucherState = client_.Workflow().LoadVoucherByWorkflow(
            localNymID, workflowID, reason_);
        const auto& [state, pVoucher] = voucherState;

        // make sure the voucher was withdrawn successfully
        //
        if (false == bool(pVoucher) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Voucher not found on workflow")
                .Flush();

            return error_task();
        }

        // construct a payment message (just like sending a cheque)
        //


        //resume

        // call the appropriate function to queue the message (same function
        // used for sending cheques, cash, or text messages)
        //
        try {
            auto& queue = get_operations({localNymID, serverID});

            return queue.StartTask<otx::client::SendVoucherTask>(
                {sourceAccountID,
                 recipientNymID,
                 value,
                 memo,
                 validFrom,
                 validTo});
        } catch (...) {
            return error_task();
        }
    };
    // ------------------------------------------------------------
    BackgroundTask bgTask{};
    try {
        auto& queue = get_operations({nymID, serverID});

        auto& [taskID, future] = bgTask;
        bgTask = queue.StartTask<otx::client::WithdrawVoucherTask>(
            {sourceAccountID, recipientNymID, value, memo, validFrom, validTo},
            finish(taskID));

    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::SendExternalTransfer(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const Identifier& sourceAccountID,
    const Identifier& targetAccountID,
    const std::int64_t value,
    const std::string& memo) const
{
    CHECK_ARGS(localNymID, serverID, targetAccountID)
    CHECK_NYM(sourceAccountID)

    auto sourceAccount = client_.Wallet().Account(sourceAccountID);

    if (false == bool(sourceAccount)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid source account.").Flush();

        return error_task();
    }

    if (sourceAccount.get().GetNymID() != localNymID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong owner on source account.")
            .Flush();

        return error_task();
    }

    if (sourceAccount.get().GetRealNotaryID() != serverID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong notary on source account.")
            .Flush();

        return error_task();
    }

    try {
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::SendTransferTask>(
            {sourceAccountID, targetAccountID, value, memo});
    } catch (...) {

        return error_task();
    }
}

OTX::BackgroundTask OTX::SendTransfer(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID,
    const Identifier& sourceAccountID,
    const Identifier& targetAccountID,
    const std::int64_t value,
    const std::string& memo) const
{
    CHECK_ARGS(localNymID, serverID, targetAccountID)
    CHECK_NYM(sourceAccountID)

    auto sourceAccount = client_.Wallet().Account(sourceAccountID);

    if (false == bool(sourceAccount)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid source account.").Flush();

        return error_task();
    }

    if (sourceAccount.get().GetNymID() != localNymID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong owner on source account.")
            .Flush();

        return error_task();
    }

    if (sourceAccount.get().GetRealNotaryID() != serverID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong notary on source account.")
            .Flush();

        return error_task();
    }

    try {
        auto& queue = get_operations({localNymID, serverID});

        return queue.StartTask<otx::client::SendTransferTask>(
            {sourceAccountID, targetAccountID, value, memo});
    } catch (...) {

        return error_task();
    }
}

void OTX::set_contact(
    const identifier::Nym& nymID,
    const identifier::Server& serverID) const
{
    auto nym = client_.Wallet().mutable_Nym(nymID, reason_);
    const auto server = nym.PreferredOTServer();

    if (server.empty()) {
        nym.AddPreferredOTServer(serverID.str(), true, reason_);
    }
}

OTServerID OTX::set_introduction_server(
    const Lock& lock,
    const contract::Server& contract) const
{
    OT_ASSERT(CheckLock(lock, introduction_server_lock_));

    try {
        const auto instantiated =
            client_.Wallet().Server(contract.PublicContract());
        const auto id = identifier::Server::Factory(
            instantiated->ID()->str());  // TODO conversion
        introduction_server_id_.reset(new OTServerID(id));

        OT_ASSERT(introduction_server_id_)

        bool dontCare = false;
        const bool set = client_.Config().Set_str(
            String::Factory(MASTER_SECTION),
            String::Factory(INTRODUCTION_SERVER_KEY),
            String::Factory(id),
            dontCare);

        OT_ASSERT(set)

        client_.Config().Save();

        return id;
    } catch (...) {
        return client_.Factory().ServerID();
    }
}

void OTX::start_introduction_server(const identifier::Nym& nymID) const
{
    auto& serverID = IntroductionServer();

    if (serverID.empty()) { return; }

    try {
        auto& queue = get_operations({nymID, serverID});
        queue.StartTask<otx::client::DownloadNymboxTask>({});
    } catch (...) {

        return;
    }
}

// Justus: TODO
// The OTX version of this function needs to be modified to accept the new
// Finish parameter and store it in the same map where it's storing the task id.
OTX::BackgroundTask OTX::start_task(
    const TaskID taskID,
    bool success,
    Finish finish /*={}*/) const
{
    if (0 == taskID) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Empty task ID").Flush();

        return error_task();
    }

    if (false == success) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Task already queued").Flush();

        return error_task();
    }

    return add_task(taskID, ThreadStatus::RUNNING, finish);
}

void OTX::StartIntroductionServer(const identifier::Nym& localNymID) const
{
    start_introduction_server(localNymID);
}

ThreadStatus OTX::status(const Lock& lock, const TaskID taskID) const
{
    OT_ASSERT(CheckLock(lock, task_status_lock_))

    if (!running_) { return ThreadStatus::SHUTDOWN; }

    auto it = task_status_.find(taskID);

    if (task_status_.end() == it) { return ThreadStatus::Error; }

    const auto output = std::get<0>(it->second);
    const bool success = (ThreadStatus::FINISHED_SUCCESS == output);
    const bool failed = (ThreadStatus::FINISHED_FAILED == output);
    const bool finished = (success || failed);

    if (finished) { task_status_.erase(it); }

    return output;
}

ThreadStatus OTX::Status(const TaskID taskID) const
{
    Lock lock(task_status_lock_);

    return status(lock, taskID);
}

void OTX::trigger_all() const
{
    Lock lock(shutdown_lock_);

    for (const auto& [id, queue] : operations_) {
        if (false == queue.Trigger()) { return; }
    }
}

void OTX::update_task(
    const TaskID taskID,
    const ThreadStatus status,
    Result&& result) const noexcept
{
    if (0 == taskID) { return; }

    Lock lock(task_status_lock_);

    if (0 == task_status_.count(taskID)) { return; }

    try {
        auto& row = task_status_.at(taskID);
        auto& [state, promise, future] = row;
        state = status;
        bool value{false};
        bool publish{false};

        switch (status) {
            case ThreadStatus::FINISHED_SUCCESS: {
                value = true;
                publish = true;
                promise.set_value(std::move(result));
            } break;
            case ThreadStatus::FINISHED_FAILED: {
                value = false;
                publish = true;
                promise.set_value(std::move(result));
            } break;
            case ThreadStatus::SHUTDOWN: {
                Result cancel{proto::LASTREPLYSTATUS_UNKNOWN, nullptr};
                promise.set_value(std::move(cancel));
            } break;
            case ThreadStatus::Error:
            case ThreadStatus::RUNNING:
            default: {
            }
        }

        if (publish) {
            auto message = zmq::Message::Factory();
            message->PrependEmptyFrame();
            message->AddFrame(std::to_string(taskID));
            message->AddFrame(Data::Factory(&value, sizeof(value)));
            task_finished_->Send(message);
        }
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Tried to finish an already-finished task (")(taskID)(")")
            .Flush();
    }
}

Depositability OTX::valid_account(
    const OTPayment& payment,
    const identifier::Nym& recipient,
    const identifier::Server& paymentServerID,
    const identifier::UnitDefinition& paymentUnitID,
    const Identifier& accountIDHint,
    Identifier& depositAccount) const
{
    std::set<OTIdentifier> matchingAccounts{};

    for (const auto& it : client_.Storage().AccountList()) {
        const auto accountID = Identifier::Factory(it.first);
        const auto nymID = client_.Storage().AccountOwner(accountID);
        const auto serverID = client_.Storage().AccountServer(accountID);
        const auto unitID = client_.Storage().AccountContract(accountID);

        if (nymID != recipient) { continue; }

        if (serverID != paymentServerID) { continue; }

        if (unitID != paymentUnitID) { continue; }

        matchingAccounts.emplace(accountID);
    }

    if (accountIDHint.empty()) {
        if (0 == matchingAccounts.size()) {

            return Depositability::NO_ACCOUNT;
        } else if (1 == matchingAccounts.size()) {
            depositAccount.Assign(*matchingAccounts.begin());

            return Depositability::READY;
        } else {

            return Depositability::ACCOUNT_NOT_SPECIFIED;
        }
    }

    if (0 == matchingAccounts.size()) {

        return Depositability::NO_ACCOUNT;
    } else if (1 == matchingAccounts.count(accountIDHint)) {
        depositAccount.Assign(accountIDHint);

        return Depositability::READY;
    } else {

        return Depositability::WRONG_ACCOUNT;
    }
}

bool OTX::valid_context(
    const identifier::Nym& nymID,
    const identifier::Server& serverID) const
{
    const auto nyms = client_.Wallet().LocalNyms();

    if (0 == nyms.count(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(
            " does not belong to this wallet.")
            .Flush();

        return false;
    }

    if (serverID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid server.").Flush();

        return false;
    }

    const auto context = client_.Wallet().ServerContext(nymID, serverID);

    if (false == bool(context)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Context does not exist.").Flush();

        return false;
    }

    if (0 == context->Request()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Nym is not registered at this server.")
            .Flush();

        return false;
    }

    return true;
}

Depositability OTX::valid_recipient(
    const OTPayment& payment,
    const identifier::Nym& specified,
    const identifier::Nym& recipient) const
{
    if (specified.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Payment can be accepted by any nym.")
            .Flush();

        return Depositability::READY;
    }

    if (recipient == specified) { return Depositability::READY; }

    return Depositability::WRONG_RECIPIENT;
}

#if OT_CASH
OTX::BackgroundTask OTX::WithdrawCash(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const Identifier& account,
    const Amount amount) const
{
    CHECK_ARGS(nymID, serverID, account)

    try {
        start_introduction_server(nymID);
        auto& queue = get_operations({nymID, serverID});

        return queue.StartTask<otx::client::WithdrawCashTask>(
            {account, amount});
    } catch (...) {

        return error_task();
    }
}
#endif

OTX::~OTX()
{
    account_subscriber_->Close();
    notification_listener_->Close();
    find_unit_listener_->Close();
    find_server_listener_->Close();
    find_nym_listener_->Close();

    Lock lock(shutdown_lock_);
    shutdown_.store(true);
    lock.unlock();
    std::vector<otx::client::implementation::StateMachine::WaitFuture>
        futures{};

    for (const auto& [id, queue] : operations_) {
        futures.emplace_back(queue.Stop());
    }

    for (const auto& future : futures) { future.get(); }

    for (auto& it : task_status_) {
        auto& promise = std::get<1>(it.second);

        try {
            promise.set_value(error_result());
        } catch (...) {
        }
    }
}
}  // namespace opentxs::api::client::implementation
