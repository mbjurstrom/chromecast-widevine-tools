chromecast-tools
=========================

Some experimental tools that are meant to run on a rooted chromecast in order to interact with crypto libraries (libwvcdm, libGtvCa, etc).

Install build dependencies
--------------------------
```bash
apt-get install git subversion
```

Install test dependencies
-------------------------
```bash
apt-get install lxc qemu-system-arm qemu-user-static curl libxml2-utils \
  squashfs-tools binwalk pax
```

How to build
------------
```bash
make
```

Before compiling this project, some dependencies will be downloaded. These include:
- precompiled arm toolchain from the 'chromecast-mirrored-source' repo.
- chromium source tree from the 'chromecast-mirrored-source' repo.
- openssl which is needed but somehow not provided with the chromium source tree.
- protobuf which is required for some headers.

Note that the arm toolchain needs a x86_64 environment to run.

How to run tests
----------------
```bash
sudo tests/lxc-bootstrap    # setup the lxc environment (only needed once; this will take a while).
make tests                  # run tests.
```

In order to test some of the tools provided by this project, LXC is used to build a cross-architecture container in which the tools can be run without
a real chromecast. The latest chromecast OTA update will be downloaded during this process.

In order to refresh your filesystem with the latest update, simply delete the file `staging/current`.

Anything related to kernel drivers and other chromecast-specific library calls
must be mocked out in order to keep the tools working.

Usage
-----

This repo provide headers for private, otherwise undocumented API exposed by libraries on the chromecast. Using these headers, tools can be built to interact with these libraries.

**bin/cert_provisioning**

This is a small utility that make use of the libwvcdm (widevine) certificate provisioning code. This code is shippied with the chromecast's chromium source tree as a `.a` library already compiled and completely undocumented. You can find compatible headers, partially documented, in this project's source tree under `includes/wvcdm`.

The method `CertificateProvisioning::GetProvisioningRequest` is very interesting. It makes use of the wvcdm CryptoSession API (`wvcdm::CryptoSession::*`) in order to generate a valid (signed) certificate provisioning request. The method call also returns the address of the provisioning server, which surprisingly enough, is accessible over the internet.

If you run `bin/cert_provisioning`, the tool will print out the provisioning server URL and a "query string" which is a base64 blob containing the actual provisioning request. To send a certificate request to the provisioning server, run the following command:

```
curl https://www.googleapis.com/certificateprovisioning/v1/devicecertificates/create?key=AIzaSyB-5OLKTx2iU5mko18DfdwK5611JIjbUhE --data '{"signedRequest":"base64 encoded request goes here"}' -H 'Content-Type: application/json'
```

The response will look like this:

```
{
 "kind": "certificateprovisioning#certificateProvisioningResponse",
 "signedResponse": "base64 encoded response"
}
```

The data contained in the base64 blob is a `SignedProvisioningMessage`, the format of which is defined in the `license_protocol` protocol definition file at `chromium/src/out_arm_eureka/Release/pyproto/license_protocol/` in the chromium source tree.

What to do with this newly generated certificate is not clear.

**bin/gtv_ca_sign**

This tool will print a valid signature for a given sha1. The signature is produced using the private key of the provided keystore, or `/factory/client.key.bin` by default.

This signature is required if someone wanted to interact with the google chrome extension by impersonating a chromecast, for instance using [Leapcast](https://github.com/EiNSTeiN-/leapcast).

The python script `tools/make-cert.py` will generate a self-signed certificate and a new private key and print out the gtv_ca_sign command to run (on a chromecast) that will get you a valid signature for that self-signed certificate.

This signature is validated by the [Google Cast Chrome Extension](https://chrome.google.com/webstore/detail/google-cast/boadgeojelhgndaghljhdicfkmllpafd) (i.e. the one that runs in your browser) when the extension connects to the chromecast. You can find an implementation of the [signature verification code here](https://chromium.googlesource.com/chromium/chromium/+/HEAD/chrome/browser/extensions/api/cast_channel/cast_auth_util_nss.cc). Below we'll try to explain in detail the device authentication performed by the chrome extension.

When the extension connects to a chromecast, the connection is protected by TLS, using a self-signed certificate and corresponding private key (such as the one generated by `make-cert.py`). As part of the cast_v2 protocol, the extension sends a device auth request, to which the device replies with a DER-encoded copy of its "client certificate" and a valid signature. The client certificate for a chromecast is stored in `/factory/client.crt` on the chromecast itself, unfortunately the private key is encrypted, hence why we need to run `gtv_ca_sign` on the chromecast itself. The chrome extension's trust anchor is a "ICA Certificate" whose public key hardcoded in the cast_v2 protocol implementation (linked in the above paragraph). For a successful device authentication to be performed, the extension will first check that the device's "client certificate" was signed by the "ICA Certificate". Then it checks that the signature provided in the device auth response is a valid signature over the DER-encoded form of the self-signed certificate.



