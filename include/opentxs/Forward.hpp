// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_FORWARD_HPP
#define OPENTXS_FORWARD_HPP

// IWYU pragma: begin_exports
#include "opentxs/Version.hpp"  // IWYU pragma: associated
// IWYU pragma: end_exports

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
class BalanceList;
class BalanceNode;
class BalanceTree;
class Deterministic;
class Ethereum;
class HD;
class Imported;
class PaymentCode;
}  // namespace blockchain

class Activity;
class Blockchain;
class Contacts;
class Issuer;
class Manager;
class OTX;
class Pair;
class ServerAction;
class UI;
class Workflow;
}  // namespace client

namespace crypto
{
class Asymmetric;
class Config;
class Crypto;
class Encode;
class Hash;
class Symmetric;
class Util;
}  // namespace crypto

namespace internal
{
struct Core;
}

namespace network
{
class Dht;
class ZAP;
class ZMQ;
}  // namespace network

namespace server
{
class Manager;
}  // namespace server

namespace storage
{
class Storage;
}  // namespace storage

class Context;
class Core;
class Crypto;
class Endpoints;
class Factory;
class HDSeed;
class Legacy;
class Periodic;
class Settings;
class Wallet;
}  // namespace api

namespace blind
{
class Mint;
class Purse;
class Token;
}  // namespace blind

namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Block;
class Header;
class Input;
class Inputs;
class Output;
class Outputs;
class Script;
class Transaction;
struct Outpoint;
}  // namespace bitcoin

class Block;
class Header;
class Transaction;
}  // namespace block

namespace client
{
class BlockOracle;
class HeaderOracle;
}  // namespace client

namespace p2p
{
class Address;
class Peer;
}  // namespace p2p

class BloomFilter;
class Network;
class NumericHash;
class Work;
}  // namespace blockchain

namespace client
{
class ServerAction;
class Wallet;
}  // namespace client

namespace contract
{
namespace peer
{
namespace reply
{
class Acknowledgement;
class Bailment;
class Connection;
class Outbailment;
}  // namespace reply

namespace request
{
class Bailment;
class BailmentNotice;
class Connection;
class Outbailment;
class StoreSecret;
}  // namespace request

class Reply;
class Request;
}  // namespace peer

namespace unit
{
class Basket;
class Currency;
class Security;
}  // namespace unit

class Server;
class Signable;
class Unit;
}  // namespace contract

namespace crypto
{
namespace key
{
class Asymmetric;
class Ed25519;
class EllipticCurve;
class HD;
class Keypair;
class RSA;
class Secp256k1;
class Symmetric;
}  // namespace key

class AsymmetricProvider;
class Bip32;
class Bip39;
class Bitcoin;
class EcdsaProvider;
class EncodingProvider;
class Envelope;
class HashingProvider;
class OpenSSL;
class Secp256k1;
class Sodium;
class SymmetricProvider;
}  // namespace crypto

namespace identifier
{
class Account;
class Contact;
class Nym;
class Server;
class UnitDefinition;
}  // namespace identifier

namespace identity
{
namespace credential
{
class Base;
class Contact;
class Key;
class Primary;
class Secondary;
class Verification;
}  // namespace credential

namespace wot
{
namespace verification
{
class Group;
class Item;
class Nym;
class Set;
}  // namespace verification
}  // namespace wot

class Authority;
class Nym;
class Source;
}  // namespace identity

namespace network
{
namespace zeromq
{
namespace curve
{
class Client;
class Server;
}  // namespace curve

namespace socket
{
class Dealer;
class Pair;
class Publish;
class Pull;
class Push;
class Reply;
class Request;
class Router;
class Sender;
class Socket;
class Subscribe;
}  // namespace socket

namespace zap
{
class Callback;
class Handler;
class Reply;
class Request;
}  // namespace zap

class Context;
class Frame;
class FrameIterator;
class FrameSection;
class ListenCallback;
class Message;
class PairEventCallback;
class Pipeline;
class Proxy;
class ReplyCallback;
}  // namespace zeromq

class Dht;
class OpenDHT;
class ServerConnection;
class ZMQ;
}  // namespace network

namespace OTDB
{
class OTPacker;
class OfferListNym;
class TradeListMarket;
}  // namespace OTDB

namespace otx
{
class Reply;
class Request;
}  // namespace otx

namespace server
{
class MessageProcessor;
class Notary;
class Server;
class UserCommandProcessor;
}  // namespace server

namespace storage
{
class Root;
}  // namespace storage

class Account;
class Amount;
class AccountList;
class AccountVisitor;
class Armored;
class Basket;
class BasketItem;
class Cheque;
class Contact;
class ContactData;
class ContactGroup;
class ContactItem;
class ContactSection;
class Contract;
class Data;
class Factory;
class Flag;
class Identifier;
class Item;
class Ledger;
class Letter;
class ListenCallbackSwig;
class Message;
class NumList;
class NymData;
class NymFile;
class NymParameters;
class OTAPI_Exec;
class OTAgent;
class OTAgreement;
class OTBylaw;
class OTCallback;
class OTCaller;
class OTClause;
class OTClient;
class OTCron;
class OTCronItem;
class OTDataFolder;
class OTMarket;
class OTNym_or_SymmetricKey;
class OTOffer;
class OTParty;
class OTPartyAccount;
class OTPayment;
class OTPaymentPlan;
class OTScript;
class OTScriptable;
class OTSignatureMetadata;
class OTSignedFile;
class OTSmartContract;
class OTStash;
class OTTrackable;
class OTTrade;
class OTTransaction;
class OTTransactionType;
class OTVariable;
class OTWallet;
class OT_API;
class PairEventCallbackSwig;
class PasswordPrompt;
class PayDividendVisitor;
class PaymentCode;
class PeerObject;
class Signals;
class Signature;
class StorageDriver;
class StoragePlugin;
class String;
class StringXML;
class Tag;
class UserCommandProcessor;
}  // namespace opentxs
#endif
