// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WEBCRYPTO_WEBCRYPTO_UTIL_H_
#define CONTENT_RENDERER_WEBCRYPTO_WEBCRYPTO_UTIL_H_

#include <string>
#include <vector>
#include "base/basictypes.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebArrayBuffer.h"
#include "third_party/WebKit/public/platform/WebCrypto.h" // TODO(eroman): delete
#include "third_party/WebKit/public/platform/WebCryptoAlgorithm.h"
#include "third_party/WebKit/public/platform/WebCryptoKey.h"

namespace content {

namespace webcrypto {

// TODO(eroman): Move Status class to a separate file

// Status indicates whether an operation completed successfully, or with an
// error. The error is used for verification in unit-tests, as well as for
// display to the user.
//
// As such, it is important that errors DO NOT reveal any sensitive material
// (like key bytes).
//
// Care must be taken with what errors are reported back to blink when doing
// compound operations like unwrapping a JWK key. In this case, errors
// generated by the JWK import are not appropriate to report since the wrapped
// JWK is not visible to the caller.
class CONTENT_EXPORT Status {
 public:
  // Returns true if the Status represents an error (any one of them).
  bool IsError() const;

  // Returns true if the Status represent success.
  bool IsSuccess() const;

  // Returns true if the Status contains a non-empty error message.
  bool HasErrorDetails() const;

  // Returns a UTF-8 error message (non-localized) describing the error. This
  // message is intended to be displayed in the dev tools console.
  std::string ToString() const;

  // Constructs a status representing success.
  static Status Success();

  // Constructs a status representing a generic error. It contains no extra
  // details.
  static Status Error();

  // ------------------------------------
  // Errors when importing a JWK formatted key
  // ------------------------------------

  // The key bytes could not parsed as JSON dictionary. This either
  // means there was a parsing error, or the JSON object was not
  // convertable to a dictionary.
  static Status ErrorJwkNotDictionary();

  // The required property |property| was missing.
  static Status ErrorJwkPropertyMissing(const std::string& property);

  // The property |property| was not of type |expected_type|.
  static Status ErrorJwkPropertyWrongType(const std::string& property,
                                          const std::string& expected_type);

  // The property |property| was a string, however could not be successfully
  // base64 decoded.
  static Status ErrorJwkBase64Decode(const std::string& property);

  // The "extractable" parameter was specified but was
  // incompatible with the value requested by the Web Crypto call.
  static Status ErrorJwkExtractableInconsistent();

  // The "alg" parameter could not be converted to an equivalent
  // WebCryptoAlgorithm. Either it was malformed or unrecognized.
  static Status ErrorJwkUnrecognizedAlgorithm();

  // The "alg" parameter is incompatible with the (optional) Algorithm
  // specified by the Web Crypto import operation.
  static Status ErrorJwkAlgorithmInconsistent();

  // The "alg" parameter was not provided, however neither was an algorithm
  // provided by the Web Crypto import operation.
  static Status ErrorJwkAlgorithmMissing();

  // The "use" parameter was specified, however it couldn't be converted to an
  // equivalent Web Crypto usage.
  static Status ErrorJwkUnrecognizedUsage();

  // The "use" parameter was specified, however it is incompatible with that
  // specified by the Web Crypto import operation.
  static Status ErrorJwkUsageInconsistent();

  // TODO(eroman): Private key import through JWK is not yet supported.
  static Status ErrorJwkRsaPrivateKeyUnsupported();

  // The "kty" parameter was given and was a string, however it was
  // unrecognized.
  static Status ErrorJwkUnrecognizedKty();

  // The amount of key data provided was incompatible with the selected
  // algorithm. For instance if the algorith name was A128CBC then EXACTLY
  // 128-bits of key data must have been provided. If 192-bits of key data were
  // given that is an error.
  static Status ErrorJwkIncorrectKeyLength();

  // ------------------------------------
  // Other errors
  // ------------------------------------

  // No key data was provided when importing an spki, pkcs8, or jwk formatted
  // key. This does not apply to raw format, since it is possible to have empty
  // key data there.
  static Status ErrorImportEmptyKeyData();

  // The wrong key was used for the operation. For instance, a public key was
  // used to verify a RsaSsaPkcs1v1_5 signature, or tried exporting a private
  // key using spki format.
  static Status ErrorUnexpectedKeyType();

  // When doing an AES-CBC encryption/decryption, the "iv" parameter was not 16
  // bytes.
  static Status ErrorIncorrectSizeAesCbcIv();

  // The data provided to an encrypt/decrypt/sign/verify operation was too
  // large. This can either represent an internal limitation (for instance
  // representing buffer lengths as uints), or an algorithm restriction (for
  // instance RSAES can operation on messages relative to the length of the
  // key's modulus).
  static Status ErrorDataTooLarge();

  // Something was unsupported or unimplemented. This can mean the algorithm in
  // question was unsupported, some parameter combination was unsupported, or
  // something has not yet been implemented.
  static Status ErrorUnsupported();

  // Something unexpected happened in the code, which implies there is a
  // source-level bug. These should not happen, but safer to fail than simply
  // DCHECK.
  static Status ErrorUnexpected();

  // The authentication tag length specified for AES-GCM encrypt/decrypt was
  // not 32, 64, 96, 104, 112, 120, or 128.
  static Status ErrorInvalidAesGcmTagLength();

  // The "publicExponent" used to generate a key was invalid: either no bytes
  // were specified, or the number was too large to fit into an "unsigned long"
  // (implemention limitation), or the exponent was zero.
  static Status ErrorGenerateKeyPublicExponent();

  // The algorithm was null when importing a raw-formatted key. In this case it
  // is required.
  static Status ErrorMissingAlgorithmImportRawKey();

  // The modulus bytes were empty when importing an RSA public key.
  static Status ErrorImportRsaEmptyModulus();

  // The the modulus length was zero bits when generating an RSA public key.
  static Status ErrorGenerateRsaZeroModulus();

  // The exponent bytes were empty when importing an RSA public key.
  static Status ErrorImportRsaEmptyExponent();

  // An unextractable key was used by an operation which exports the key data.
  static Status ErrorKeyNotExtractable();

  // The key length specified when generating a key was invalid. Either it was
  // zero, or it was not a multiple of 8 bits.
  static Status ErrorGenerateKeyLength();

 private:
  enum Type { TYPE_ERROR, TYPE_SUCCESS };

  // Constructs an error with the specified message.
  explicit Status(const std::string& error_details_utf8);

  // Constructs a success or error without any details.
  explicit Status(Type type);

  Type type_;
  std::string error_details_;
};

// Returns a pointer to the start of |data|, or NULL if it is empty. This is a
// convenience function for getting the pointer, and should not be used beyond
// the expected lifetime of |data|.
CONTENT_EXPORT const uint8* Uint8VectorStart(const std::vector<uint8>& data);

// Shrinks a WebArrayBuffer to a new size.
// TODO(eroman): This works by re-allocating a new buffer. It would be better if
//               the WebArrayBuffer could just be truncated instead.
void ShrinkBuffer(blink::WebArrayBuffer* buffer, unsigned int new_size);

// Creates a WebArrayBuffer from a uint8 byte array
blink::WebArrayBuffer CreateArrayBuffer(const uint8* data,
                                        unsigned int data_size);

// TODO(eroman): Move this to JWK file.
// This function decodes unpadded 'base64url' encoded data, as described in
// RFC4648 (http://www.ietf.org/rfc/rfc4648.txt) Section 5.
// In Web Crypto, this type of encoding is only used inside JWK.
bool Base64DecodeUrlSafe(const std::string& input, std::string* output);

CONTENT_EXPORT bool IsHashAlgorithm(blink::WebCryptoAlgorithmId alg_id);

// Returns the "hash" param for an algorithm if it exists, otherwise returns
// a null algorithm.
blink::WebCryptoAlgorithm GetInnerHashAlgorithm(
    const blink::WebCryptoAlgorithm& algorithm);

// Creates a WebCryptoAlgorithm without any parameters.
CONTENT_EXPORT blink::WebCryptoAlgorithm CreateAlgorithm(
    blink::WebCryptoAlgorithmId id);

// Creates an HMAC import algorithm whose inner hash algorithm is determined by
// the specified algorithm ID. It is an error to call this method with a hash
// algorithm that is not SHA*.
CONTENT_EXPORT blink::WebCryptoAlgorithm CreateHmacImportAlgorithm(
    blink::WebCryptoAlgorithmId hash_id);

// Creates an RSASSA-PKCS1-v1_5 algorithm. It is an error to call this with a
// hash_id that is not a SHA*.
blink::WebCryptoAlgorithm CreateRsaSsaImportAlgorithm(
    blink::WebCryptoAlgorithmId hash_id);

// Creates an RSA-OAEP algorithm. It is an error to call this with a hash_id
// that is not a SHA*.
blink::WebCryptoAlgorithm CreateRsaOaepImportAlgorithm(
    blink::WebCryptoAlgorithmId hash_id);

// TODO(eroman): Move to shared_crypto.cc
// Returns the internal block size for SHA-*
unsigned int ShaBlockSizeBytes(blink::WebCryptoAlgorithmId hash_id);

#ifdef WEBCRYPTO_HAS_KEY_ALGORITHM
bool CreateSecretKeyAlgorithm(const blink::WebCryptoAlgorithm& algorithm,
                              unsigned keylen_bytes,
                              blink::WebCryptoKeyAlgorithm* key_algorithm);
#else
bool CreateSecretKeyAlgorithm(const blink::WebCryptoAlgorithm& algorithm,
                              unsigned keylen_bytes,
                              blink::WebCryptoAlgorithm* key_algorithm);
#endif

}  // namespace webcrypto

}  // namespace content

#endif  // CONTENT_RENDERER_WEBCRYPTO_WEBCRYPTO_UTIL_H_
