
#include "Node.h"
#include "Log.h"
#include <boost/lexical_cast.hpp>
#include <typeinfo>
#include "Exceptions.h"
#include "FCPErrorResponse.h"
#include <fstream>
#include <sstream>

using namespace FCPLib;

std::string
Node::_getUniqueId() {
    char newid[100];
    sprintf(newid, "id%d", (int) time(0));
    return std::string(newid);
}

const Message::Ptr
Node::getNodeHelloMessage() const
{
  return nodeHelloMessage;
}

void
Node::checkProtocolError(Response &resp)
{
  ServerMessage::Ptr sm = createResult<ServerMessage::Ptr, LastMessage>( resp );

  if ( sm->isError() )
    throw FCPException( sm->getMessage() );
}

Node::Node(std::string name_, std::string host, int port)
  : name(name_),
    clientReqQueue( new TQueue<JobTicket::Ptr>() ),
    globalCommandsTimeout(20)  // 20 sec
{
  if (!name.size())
    name = Node::_getUniqueId();
  log().log(DEBUG, "Node started name=" + name + "\n");

  nodeThread = new NodeThread(host, port, clientReqQueue);
  executor.execute( nodeThread );

  Message::Ptr m = Message::factory(std::string("ClientHello"));
  m->setField("Name", name);
  m->setField("ExpectedVersion", "2.0");

  JobTicket::Ptr job = JobTicket::factory("__hello", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "Node constructor: waiting for response to ClientHello");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // check if CloceConnectionDuplicateName or ProtocolError has arrived
  checkProtocolError(resp); // throws

  nodeHelloMessage = boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                                 FCPResult>(
                                                     FCPResult::factory( job->getCommandName(), resp )
                                                )->getMessage();
}

Node::~Node()
{
  executor.interrupt();
}

FCPMultiMessageResponse::Ptr
Node::listPeers(const AdditionalFields& fields)
{
  Message::Ptr m = Message::factory( std::string("ListPeers") );
  if (fields.hasField("WithMetadata")) m->setField("WithMetadata", fields.getField("WithMetadata"));
  if (fields.hasField("WithVolatile")) m->setField("WithVolatile", fields.getField("WithVolatile"));

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for EndListPeers message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // NOTE: error should never happen here...
  checkProtocolError(resp) // throws

  return boost::dynamic_pointer_cast<FCPMultiMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPMultiMessageResponse::Ptr
Node::listPeerNotes(const std::string& identifier)
{
  Message::Ptr m = Message::factory( std::string("ListPeerNotes") );
  m->setField("NodeIdentifier", identifier);

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for EndListPeerNotes message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPMultiMessageResponse,
                                    FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPOneMessageResponse::Ptr
Node::addPeer(const std::string &value, bool isURL = false) {
  Message::Ptr m = Message::factory( std::string("AddPeer") );
  if (!isURL)
    m->setField("File", value);
  else
    m->setField("URL", value);

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for Peer message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}


FCPOneMessageResponse::Ptr
Node::addPeer(const std::map<std::string, std::string> &message)
{
  Message::Ptr m = Message::factory( std::string("AddPeer") );

  m->setFields(message);

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for Peer message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPOneMessageResponse::Ptr
Node::modifyPeer(const std::string & nodeIdentifier,
                 const AdditionalFields& fields)
{
  Message::Ptr m = Message::factory( std::string("ModifyPeer") );

  m->setField("NodeIdentifier", nodeIdentifier);
  if (fields.hasField("AllowLocalAddresses")) m->setField("AllowLocalAddresses", fields.getField("AllowLocalAddresses"));
  if (fields.hasField("IsDisabled")) m->setField("IsDisabled", fields.getField("IsDisabled"));
  if (fields.hasField("IsListenOnly")) m->setField("IsListenOnly", fields.getField("IsListenOnly"));

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for Peer message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPOneMessageResponse::Ptr
Node::modifyPeerNote(const std::string & nodeIdentifier,
                     const std::string & noteText,
                     int peerNoteType = 1)
{
  Message::Ptr m = Message::factory( std::string("ModifyPeerNote") );

  m->setField("NodeIdentifier", nodeIdentifier);
  m->setField("NoteText", noteText);
  m->setField("PeerNoteType", "1");  // TODO: change to peerNoteType once it is used

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for PeerNote message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPOneMessageResponse::Ptr
Node::removePeer(const std::string &identifier)
{
  Message::Ptr m = Message::factory( std::string("RemovePeer") );

  m->setField("NodeIdentifier", identifier);

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for PeerRemoved message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws


  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

Message::Ptr
Node::getNode(const AdditionalFields& fields)
{
  Message::Ptr m = Message::factory( std::string("GetNode") );

  if (fields.hasField("WithPrivate")) m->setField("WithPrivate", fields.getField("WithPrivate"));
  if (fields.hasField("WithVolatile")) m->setField("WithVolatile", fields.getField("WithVolatile"));

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for NodeData message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

//  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
//                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );

  return createResult<Message::Ptr, MessageConverter>( resp );
}

FCPOneMessageResponse::Ptr
Node::getConfig(const AdditionalFields& fields)
{
  Message::Ptr m = Message::factory( std::string("GetConfig") );

  if (fields.hasField("WithCurrent")) m->setField("WithCurrent", fields.getField("WithCurrent"));
  if (fields.hasField("WithDefault")) m->setField("WithDefault", fields.getField("WithDefault"));
  if (fields.hasField("WithSortOrder")) m->setField("WithSortOrder", fields.getField("WithSortOrder"));
  if (fields.hasField("WithExpertFlag")) m->setField("WithExpertFlag", fields.getField("WithExpertFlag"));
  if (fields.hasField("WithForceWriteFlag")) m->setField("WithForceWriteFlag", fields.getField("WithForceWriteFlag"));
  if (fields.hasField("WithShortDescription")) m->setField("WithShortDescription", fields.getField("WithShortDescription"));
  if (fields.hasField("WithLongDescription")) m->setField("WithLongDescription", fields.getField("WithLongDescription"));

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for ConfigData message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPOneMessageResponse::Ptr
Node::modifyConfig(Message::Ptr m)
{
  if (m->getHeader() != "ModifyConfig")
    throw std::logic_error("ModifyConfig message expected, " + m->getHeader() + " received");

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for ConfigData message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // ProtocolError or UnknownNodeIdentifier
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPTestDDAReplyResponse::Ptr
Node::testDDARequest(std::string dir, bool read, bool write)
{
  Message::Ptr m = Message::factory( std::string("TestDDARequest") );

  m->setField("Directory", dir);
  if (read)
    m->setField("WantReadDirectory", "true");
  if (write)
    m->setField("WantWriteDirectory", "true");

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for TestDDAReply");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  // check if protocol error has occured
  checkProtocolError(resp); // throws


  return boost::dynamic_pointer_cast<FCPTestDDAReplyResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

FCPTestDDAResponse
Node::testDDAResponse(std::string dir, std::string readContent)
{
  Message::Ptr m = Message::factory( std::string("TestDDAResponse") );

  m->setField("Directory", dir);
  if (readContent != "")
    m->setField("ReadContent", readContent);

  JobTicket::Ptr job = JobTicket::factory( "__global", m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for TestDDAComplete");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();

  // check if protocol error has occured
  checkProtocolError(resp); // throws


  FCPOneMessageResponse::Ptr response =
     boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                 FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );

  m = response->getMessage();
  return FCPTestDDAResponse(m->getField("Directory"),
                            m->getField("ReadDirectoryAllowed")=="true",
                            m->getField("WriteDirectoryAllowed")=="true");
}

FCPTestDDAResponse
Node::testDDA(std::string dir, bool read, bool write)
{
   FCPTestDDAReplyResponse::Ptr replyResponse;
   Message::Ptr m;

   try
   {
     replyResponse = testDDARequest(dir, read, write);

     std::ostringstream readContent;
     if (read) {
       std::ifstream is(replyResponse->getReadFilename().c_str());
       // check that file is opened
       if (is) {
         readContent << is.rdbuf();
       }
     }
     if (write) {
       std::ofstream os(replyResponse->getWriteFilename().c_str());
       if (os) {
         os << replyResponse->getContent();;
         os.close();
       }
     }
     FCPTestDDAResponse ret = testDDAResponse(dir, readContent.str());
     //TODO: delete created file
     return ret;
   }
   catch (FCPException& e)
   {
     log().log(ERROR, e.getMessage()->toString());
     return FCPTestDDAResponse(dir, false, false);
   }
   catch (std::logic_error& e)
   {
     log().log(FATAL, e.what()); // this should never happen... TODO: should I force shutdown?
     return FCPTestDDAResponse(dir, false, false);
   }
   catch (std::runtime_error& e)
   {
     log().log(ERROR, e.what());
     return FCPTestDDAResponse(dir, false, false);
   }
   catch (std::exception& e)
   {
     log().log(ERROR, e.what());
     return FCPTestDDAResponse(dir, false, false);
   }
}

FCPOneMessageResponse::Ptr
Node::generateSSK(std::string identifier)
{
  Message::Ptr m = Message::factory( std::string("GenerateSSK") );
  m->setField("Identifier", identifier);

  JobTicket::Ptr job = JobTicket::factory( identifier, m, false);
  clientReqQueue->put(job);

  log().log(DEBUG, "waiting for SSKKeypair message");
  job->wait(globalCommandsTimeout);

  Response resp = job->getResponse();
  checkProtocolError(resp); // throws

  return boost::dynamic_pointer_cast<FCPOneMessageResponse,
                                     FCPResult>( FCPResult::factory( job->getCommandName(), resp ) );
}

JobTicket::Ptr
Node::putData(const std::string URI, std::istream* s, int dataLength, const std::string id, const AdditionalFields& fields )
{
  Message::Ptr m = Message::factory( std::string("ClientPut"), true );

  m->setField("URI", URI);
  m->setField("Identifier", id == "" ? _getUniqueId() : id);
  if (fields.hasField("Metadata.ContentType")) m->setField("Metadata.ContentType", fields.getField("Metadata.ContentType"));
  if (fields.hasField("Verbosity")) m->setField("Verbosity", fields.getField("Verbosity"));
  if (fields.hasField("MaxRetries")) m->setField("MaxRetries", fields.getField("MaxRetries"));
  if (fields.hasField("PriorityClass")) m->setField("PriorityClass", fields.getField("PriorityClass"));
  if (fields.hasField("GetCHKOnly")) m->setField("GetCHKOnly", fields.getField("GetCHKOnly"));
  if (fields.hasField("Global")) m->setField("Global", fields.getField("Global"));
  if (fields.hasField("DontCompress")) m->setField("DontCompress", fields.getField("DontCompress"));
  if (fields.hasField("ClientToken")) m->setField("ClientToken", fields.getField("ClientToken"));
  if (fields.hasField("Persistence")) m->setField("Persistence", fields.getField("Persistence"));
  if (fields.hasField("TargetFilename")) m->setField("TargetFilename", fields.getField("TargetFilename"));
  if (fields.hasField("EarlyEncode")) m->setField("EarlyEncode", fields.getField("EarlyEncode"));
  m->setField("UploadFrom", "direct");

  m->setStream(s, dataLength);

  JobTicket::Ptr job = JobTicket::factory( m->getField("Identifier"), m, false);
  clientReqQueue->put(job);

  return job;
}

JobTicket::Ptr
Node::putRedirect(const std::string URI, const std::string target, const std::string id, const AdditionalFields& fields )
{
  Message::Ptr m = Message::factory( std::string("ClientPut"));

  m->setField("URI", URI);
  m->setField("Identifier", id == "" ? _getUniqueId() : id);
  if (fields.hasField("mimetype")) m->setField("Metadata.ContentType", fields.getField("mimetype"));
  if (fields.hasField("Verbosity")) m->setField("Verbosity", fields.getField("Verbosity"));
  if (fields.hasField("MaxRetries")) m->setField("MaxRetries", fields.getField("MaxRetries"));
  if (fields.hasField("PriorityClass")) m->setField("PriorityClass", fields.getField("PriorityClass"));
  if (fields.hasField("Global")) m->setField("Global", fields.getField("Global"));
  if (fields.hasField("ClientToken")) m->setField("ClientToken", fields.getField("ClientToken"));
  if (fields.hasField("Persistence")) m->setField("Persistence", fields.getField("Persistence"));
  if (fields.hasField("TargetFilename")) m->setField("TargetFilename", fields.getField("TargetFilename"));
  if (fields.hasField("EarlyEncode")) m->setField("EarlyEncode", fields.getField("EarlyEncode"));
  m->setField("TargetURI", target);

  JobTicket::Ptr job = JobTicket::factory( m->getField("Identifier"), m, false);
  log().log(DEBUG, job->toString());
  clientReqQueue->put(job);

  return job;
}
