// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CORE_QUIC_CRYPTO_SERVER_STREAM_H_
#define NET_QUIC_CORE_QUIC_CRYPTO_SERVER_STREAM_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/macros.h"
#include "net/quic/core/crypto/crypto_handshake.h"
#include "net/quic/core/crypto/quic_compressed_certs_cache.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/proto/source_address_token.pb.h"
#include "net/quic/core/quic_config.h"
#include "net/quic/core/quic_crypto_stream.h"
#include "net/quic/platform/api/quic_export.h"

namespace net {

class CachedNetworkParameters;
class CryptoHandshakeMessage;
class QuicCryptoServerConfig;
class QuicCryptoServerStreamBase;

namespace test {
class QuicCryptoServerStreamPeer;
}  // namespace test

// TODO(alyssar) see what can be moved out of QuicCryptoServerStream with
// various code and test refactoring.
class QUIC_EXPORT_PRIVATE QuicCryptoServerStreamBase : public QuicCryptoStream {
 public:
  explicit QuicCryptoServerStreamBase(QuicSession* session);

  ~QuicCryptoServerStreamBase() override {}

  // Cancel any outstanding callbacks, such as asynchronous validation of client
  // hello.
  virtual void CancelOutstandingCallbacks() = 0;

  // GetBase64SHA256ClientChannelID sets |*output| to the base64 encoded,
  // SHA-256 hash of the client's ChannelID key and returns true, if the client
  // presented a ChannelID. Otherwise it returns false.
  virtual bool GetBase64SHA256ClientChannelID(std::string* output) const = 0;

  virtual int NumServerConfigUpdateMessagesSent() const = 0;

  // Sends the latest server config and source-address token to the client.
  virtual void SendServerConfigUpdate(
      const CachedNetworkParameters* cached_network_params) = 0;

  // These are all accessors and setters to their respective counters.
  virtual uint8_t NumHandshakeMessages() const = 0;
  virtual uint8_t NumHandshakeMessagesWithServerNonces() const = 0;
  virtual bool UseStatelessRejectsIfPeerSupported() const = 0;
  virtual bool PeerSupportsStatelessRejects() const = 0;
  virtual bool ZeroRttAttempted() const = 0;
  virtual void SetPeerSupportsStatelessRejects(bool set) = 0;
  virtual const CachedNetworkParameters* PreviousCachedNetworkParams()
      const = 0;
  virtual void SetPreviousCachedNetworkParams(
      CachedNetworkParameters cached_network_params) = 0;

  // Checks the options on the handshake-message to see whether the
  // peer supports stateless-rejects.
  static bool DoesPeerSupportStatelessRejects(
      const CryptoHandshakeMessage& message);
};

class QUIC_EXPORT_PRIVATE QuicCryptoServerStream
    : public QuicCryptoServerStreamBase {
 public:
  class Helper {
   public:
    virtual ~Helper() {}

    // Given the current connection_id, generates a new ConnectionId to
    // be returned with a stateless reject.
    virtual QuicConnectionId GenerateConnectionIdForReject(
        QuicConnectionId connection_id) const = 0;

    // Returns true if |message|, which was received on |self_address| is
    // acceptable according to the visitor's policy. Otherwise, returns false
    // and populates |error_details|.
    virtual bool CanAcceptClientHello(const CryptoHandshakeMessage& message,
                                      const QuicSocketAddress& self_address,
                                      std::string* error_details) const = 0;
  };

  // |crypto_config| must outlive the stream.
  // |session| must outlive the stream.
  // |helper| must outlive the stream.
  QuicCryptoServerStream(const QuicCryptoServerConfig* crypto_config,
                         QuicCompressedCertsCache* compressed_certs_cache,
                         bool use_stateless_rejects_if_peer_supported,
                         QuicSession* session,
                         Helper* helper);

  ~QuicCryptoServerStream() override;

  // From QuicCryptoServerStreamBase
  void CancelOutstandingCallbacks() override;
  void OnHandshakeMessage(const CryptoHandshakeMessage& message) override;
  bool GetBase64SHA256ClientChannelID(std::string* output) const override;
  void SendServerConfigUpdate(
      const CachedNetworkParameters* cached_network_params) override;
  uint8_t NumHandshakeMessages() const override;
  uint8_t NumHandshakeMessagesWithServerNonces() const override;
  int NumServerConfigUpdateMessagesSent() const override;
  const CachedNetworkParameters* PreviousCachedNetworkParams() const override;
  bool UseStatelessRejectsIfPeerSupported() const override;
  bool PeerSupportsStatelessRejects() const override;
  bool ZeroRttAttempted() const override;
  void SetPeerSupportsStatelessRejects(
      bool peer_supports_stateless_rejects) override;
  void SetPreviousCachedNetworkParams(
      CachedNetworkParameters cached_network_params) override;

  // NOTE: Indicating that the Expect-CT header should be sent here presents
  // a layering violation to some extent. The Expect-CT header only applies to
  // HTTP connections, while this class can be used for non-HTTP applications.
  // However, it is exposed here because that is the only place where the
  // configuration for the certificate used in the connection is accessible.
  bool ShouldSendExpectCTHeader() const {
    return signed_config_->proof.send_expect_ct_header;
  }

  bool encryption_established() const override;
  bool handshake_confirmed() const override;
  const QuicCryptoNegotiatedParameters& crypto_negotiated_params()
      const override;

 protected:
  virtual void ProcessClientHello(
      QuicReferenceCountedPointer<ValidateClientHelloResultCallback::Result>
          result,
      std::unique_ptr<ProofSource::Details> proof_source_details,
      std::unique_ptr<ProcessClientHelloResultCallback> done_cb);

  // Hook that allows the server to set QuicConfig defaults just
  // before going through the parameter negotiation step.
  virtual void OverrideQuicConfigDefaults(QuicConfig* config);

  // Returns client address used to generate and validate source address token.
  virtual const QuicSocketAddress GetClientAddress();

 private:
  friend class test::QuicCryptoServerStreamPeer;

  class ValidateCallback : public ValidateClientHelloResultCallback {
   public:
    explicit ValidateCallback(QuicCryptoServerStream* parent);
    // To allow the parent to detach itself from the callback before deletion.
    void Cancel();

    // From ValidateClientHelloResultCallback
    void Run(QuicReferenceCountedPointer<Result> result,
             std::unique_ptr<ProofSource::Details> details) override;

   private:
    QuicCryptoServerStream* parent_;

    DISALLOW_COPY_AND_ASSIGN(ValidateCallback);
  };

  class SendServerConfigUpdateCallback
      : public BuildServerConfigUpdateMessageResultCallback {
   public:
    explicit SendServerConfigUpdateCallback(QuicCryptoServerStream* parent);
    SendServerConfigUpdateCallback(const SendServerConfigUpdateCallback&) =
        delete;
    void operator=(const SendServerConfigUpdateCallback&) = delete;

    // To allow the parent to detach itself from the callback before deletion.
    void Cancel();

    // From BuildServerConfigUpdateMessageResultCallback
    void Run(bool ok, const CryptoHandshakeMessage& message) override;

   private:
    QuicCryptoServerStream* parent_;
  };

  // Invoked by ValidateCallback::RunImpl once initial validation of
  // the client hello is complete.  Finishes processing of the client
  // hello message and handles handshake success/failure.
  void FinishProcessingHandshakeMessage(
      QuicReferenceCountedPointer<ValidateClientHelloResultCallback::Result>
          result,
      std::unique_ptr<ProofSource::Details> details);

  class ProcessClientHelloCallback;
  friend class ProcessClientHelloCallback;

  // Portion of FinishProcessingHandshakeMessage which executes after
  // ProcessClientHello has been called.
  void FinishProcessingHandshakeMessageAfterProcessClientHello(
      const ValidateClientHelloResultCallback::Result& result,
      QuicErrorCode error,
      const std::string& error_details,
      std::unique_ptr<CryptoHandshakeMessage> reply,
      std::unique_ptr<DiversificationNonce> diversification_nonce,
      std::unique_ptr<ProofSource::Details> proof_source_details);

  // Invoked by SendServerConfigUpdateCallback::RunImpl once the proof has been
  // received.  |ok| indicates whether or not the proof was successfully
  // acquired, and |message| holds the partially-constructed message from
  // SendServerConfigUpdate.
  void FinishSendServerConfigUpdate(bool ok,
                                    const CryptoHandshakeMessage& message);

  // Returns a new ConnectionId to be used for statelessly rejected connections
  // if |use_stateless_rejects| is true. Returns 0 otherwise.
  QuicConnectionId GenerateConnectionIdForReject(bool use_stateless_rejects);

  // crypto_config_ contains crypto parameters for the handshake.
  const QuicCryptoServerConfig* crypto_config_;

  // compressed_certs_cache_ contains a set of most recently compressed certs.
  // Owned by QuicDispatcher.
  QuicCompressedCertsCache* compressed_certs_cache_;

  // Server's certificate chain and signature of the server config, as provided
  // by ProofSource::GetProof.
  QuicReferenceCountedPointer<QuicSignedServerConfig> signed_config_;

  // Hash of the last received CHLO message which can be used for generating
  // server config update messages.
  std::string chlo_hash_;

  // Pointer to the helper for this crypto stream. Must outlive this stream.
  Helper* helper_;

  // Number of handshake messages received by this stream.
  uint8_t num_handshake_messages_;

  // Number of handshake messages received by this stream that contain
  // server nonces (indicating that this is a non-zero-RTT handshake
  // attempt).
  uint8_t num_handshake_messages_with_server_nonces_;

  // Pointer to the active callback that will receive the result of
  // BuildServerConfigUpdateMessage and forward it to
  // FinishSendServerConfigUpdate.  nullptr if no update message is currently
  // being built.
  SendServerConfigUpdateCallback* send_server_config_update_cb_;

  // Number of server config update (SCUP) messages sent by this stream.
  int num_server_config_update_messages_sent_;

  // If the client provides CachedNetworkParameters in the STK in the CHLO, then
  // store here, and send back in future STKs if we have no better bandwidth
  // estimate to send.
  std::unique_ptr<CachedNetworkParameters> previous_cached_network_params_;

  // Contains any source address tokens which were present in the CHLO.
  SourceAddressTokens previous_source_address_tokens_;

  // If true, the server should use stateless rejects, so long as the
  // client supports them, as indicated by
  // peer_supports_stateless_rejects_.
  bool use_stateless_rejects_if_peer_supported_;

  // Set to true, once the server has received information from the
  // client that it supports stateless reject.
  //  TODO(jokulik): Remove once client stateless reject support
  // becomes the default.
  bool peer_supports_stateless_rejects_;

  // True if client attempts 0-rtt handshake (which can succeed or fail). If
  // stateless rejects are used, this variable will be false for the stateless
  // rejected connection and true for subsequent connections.
  bool zero_rtt_attempted_;

  // Size of the packet containing the most recently received CHLO.
  QuicByteCount chlo_packet_size_;

  // Pointer to the active callback that will receive the result of the client
  // hello validation request and forward it to FinishProcessingHandshakeMessage
  // for processing.  nullptr if no handshake message is being validated.  Note
  // that this field is mutually exclusive with process_client_hello_cb_.
  ValidateCallback* validate_client_hello_cb_;

  // Pointer to the active callback which will receive the results of
  // ProcessClientHello and forward it to
  // FinishProcessingHandshakeMessageAfterProcessClientHello.  Note that this
  // field is mutually exclusive with validate_client_hello_cb_.
  ProcessClientHelloCallback* process_client_hello_cb_;

  bool encryption_established_;
  bool handshake_confirmed_;
  QuicReferenceCountedPointer<QuicCryptoNegotiatedParameters>
      crypto_negotiated_params_;

  DISALLOW_COPY_AND_ASSIGN(QuicCryptoServerStream);
};

}  // namespace net

#endif  // NET_QUIC_CORE_QUIC_CRYPTO_SERVER_STREAM_H_
