/*
 * Copyright (c) 2024 d0p1 <contact@d0p1.eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FCP_REQUEST_HPP_
#define FCP_REQUEST_HPP_

#include <boost/asio/ip/tcp.hpp>
#include <fcp++/node.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace fcp::protocol {

/**
 * All stuff related to Client->Node
 */
class Request
{
public:
  Request(std::string_view name) : mName(name) {};
  virtual ~Request() = default;

  void SetAttribute(std::string aKey, std::string aValue)
  {
    this->mAttributes[aKey] = aValue;
  };

  std::string ToString()
  {
    std::string str(this->mName);
    str += "\n";
    for (auto& it : this->mAttributes) {
      str += it.first;
      str += "=";
      str += it.second;
      str += "\n";
    }
    str += "EndMessage\n";

    return str;
  }

private:
  std::string_view mName;
  std::unordered_map<std::string, std::string> mAttributes;


public:

/**
 * First message sent from the client to the node. Node will response with \ref Response::NodeHello
 *
 * \code{.unparsed}
 * ClientHello
 * Name=My Freenet Client
 * ExpectedVersion=2.0
 * EndMessage
 * \endcode
 */
struct ClientHello
{
  /** A unique name to identify client to the node */
  std::string Name;
  /** Expected FCP version, must be "2.0" */
  std::string ExpectedVersion;

  ClientHello(std::string_view name)
    : Name(name)
    , ExpectedVersion("2.0")
  {
  }

  Request ToRequest()
  {
    Request req("ClientHello");

    req.SetAttribute("Name", this->Name);
    req.SetAttribute("ExpectedVersion", this->ExpectedVersion);

    return req;
  }
};

struct ListPeer
{
  std::string NodeIdentifier;
  std::optional<bool> WithMetaData;
  std::optional<bool> WithVolatile;

  ListPeer(std::string_view ident)
    : NodeIdentifier(ident)
  {
  }

  Request ToRequest()
  {
    Request req("ListPeer");

    req.SetAttribute("NodeIdentifier", this->NodeIdentifier);
    if (this->WithMetaData.has_value()) {
      req.SetAttribute("WithMetaData", to_string(this->WithMetaData.value()));
    }

    if (this->WithVolatile.has_value()) {
      req.SetAttribute("WithVolatile", to_string(this->WithVolatile.value()));
    }

    return req;
  }
};

struct ListPeers
{
  std::optional<std::string> Identifier;
  std::optional<bool> WithMetaData;
  std::optional<bool> WithVolatile;

  Request ToRequest()
  {
    Request req("ListPeers");

    if (this->Identifier.has_value()) {
      req.SetAttribute("Identifier", this->Identifier.value());
    }

    if (this->WithMetaData.has_value()) {
      req.SetAttribute("WithMetaData", to_string(this->WithMetaData.value()));
    }

    if (this->WithVolatile.has_value()) {
      req.SetAttribute("WithVolatile", to_string(this->WithVolatile.value()));
    }

    return req;
  }
};

struct ListPeerNotes
{
  std::string NodeIdentifier;

  ListPeerNotes(std::string_view ident)
    : NodeIdentifier(ident)
  {
  }

  Request ToReq()
  {
    Request req("ListPeerNotes");

    req.SetAttribute("NodeIdentifier", this->NodeIdentifier);

    return req;
  }
};

struct AddPeer
{
  Node::Trust Trust;
  Node::Visibility Visibility;

  Request ToReq()
  {
    Request req("AddPeer");
    req.SetAttribute("Trust", to_string(this->Trust));
    req.SetAttribute("Visibility", to_string(this->Visibility));

    return req;
  }
};

struct ModifyPeer
{};

struct Disconnect
{
  Request ToRequest() { return Request("Disconnect"); }
};

/**
 * Shutdown request.
 *
 * \code{.unparsed}
 * Shutdown
 * EndMessage
 * \endcode
 */
struct Shutdown
{
  Request ToRequest() { return Request("Shutdown"); }
};

struct Probe
{
  enum class Type {
    BANDWIDTH,
    BUILD,
    IDENTIFIER,
    LINK_LENGTHS,
    LOCATION,
    REJECT_STATS,
    STORE_SIZE,
    UPTIME_48H,
    UPTIME_7D
  } Type;
};

};
}

#endif // !FCP_REQUEST_HPP_
