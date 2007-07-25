#ifndef NODE_H__
#define NODE_H__

#include <string>
#include <memory>
#include <map>

#include "zthread/Thread.h"
#include "zthread/ThreadedExecutor.h"

#include "FCPResult.h"
#include "TQueue.h"
#include "NodeThread.h"
#include "AdditionalFields.h"

#include "sha256.h"

namespace FCPLib {
class Node {
  std::string name;
  ZThread::CountedPtr< JobTicketQueue > clientReqQueue;
  NodeThread* nodeThread;
  ZThread::ThreadedExecutor executor;

  int globalCommandsTimeout;

  static std::string _getUniqueId();

  void checkProtocolError(Response &resp);
  Message::Ptr nodeHelloMessage;

public:
  Node(std::string name, std::string host, int port);
  ~Node();

  int getGlobalCommandsTimeout() const {
    return globalCommandsTimeout;
  }

  Node& setGlobalCommandsTimeout(int t) {
    globalCommandsTimeout = t; return *this;
  }

  void shutdown();

  bool isAlive() const {
    return nodeThread->isAlive();
  }

  std::exception getFailure() const {
    return *nodeThread->getFailure();
  }

  const Message::Ptr getNodeHelloMessage() const;

  Message::Ptr listPeer(const std::string &);
  MessagePtrContainer listPeers(const AdditionalFields& = AdditionalFields());
  PeerNoteContainer listPeerNotes(const std::string&);
  Message::Ptr addPeer(const std::string &, bool isURL);
  Message::Ptr addPeer(const std::map<std::string, std::string> &message);
  Message::Ptr modifyPeer(const std::string &, const AdditionalFields& = AdditionalFields());
  PeerNote modifyPeerNote(const std::string &, const std::string &, int = 1);
  Message::Ptr removePeer(const std::string &);

  Message::Ptr getNode(const AdditionalFields& = AdditionalFields());
  Message::Ptr getConfig(const AdditionalFields& = AdditionalFields());
  Message::Ptr modifyConfig(Message::Ptr m);

  TestDDAReplyResponse::Ptr testDDARequest(std::string dir, bool read, bool write);
  TestDDAResponse testDDAResponse(std::string dir, std::string readContent = "");
  TestDDAResponse testDDA(std::string dir, bool read, bool write);

  Message::Ptr generateSSK(std::string identifier);
  JobTicket::Ptr putData(const std::string , // URI
                                  std::istream*, // Data Stream
                                  int, // dataLength
                                  const std::string = "", // Identifier
                                  const AdditionalFields& = AdditionalFields()
                                  );
  JobTicket::Ptr putRedirect(const std::string , // URI
                                      const std::string , // Target
                                      const std::string = "", // Identifier
                                      const AdditionalFields& = AdditionalFields()
                                      );
  JobTicket::Ptr putDisk(const std::string , // URI
                         const std::string , // Filename
                         const std::string = "", // Identifier
                         const AdditionalFields& = AdditionalFields()
                         );

  JobTicket::Ptr subscribeUSK(const std::string, const std::string, bool);

  void watchGlobal( bool enabled, int verbosity );
  void refreshPersistentRequest();
};
}

#endif
