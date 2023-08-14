#include "ns3/distribute-epr-protocol.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistributeEPRProtocol");

/* source */

NS_OBJECT_ENSURE_REGISTERED (DistributeEPRSrcProtocol);

DistributeEPRSrcProtocol::DistributeEPRSrcProtocol (Ptr<QuantumPhyEntity> qphyent_,
                                                    Ptr<QuantumChannel> conn_,
                                                    const std::pair<std::string, std::string> &epr_)
    : m_data (0), m_dataSize (0), m_size (0), m_qphyent (qphyent_), m_conn (conn_), m_epr (epr_)
{
  SetRemote (m_conn->GetDst (m_qphyent)->GetAddress (), m_conn->GetDst (m_qphyent)->GetNextPort ());
}

DistributeEPRSrcProtocol::~DistributeEPRSrcProtocol ()
{
  NS_LOG_LOGIC ("Destroying DistributeEPRSrcProtocol");
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = 0;
  NS_LOG_LOGIC ("Destroyed DistributeEPRSrcProtocol");
}

DistributeEPRSrcProtocol::DistributeEPRSrcProtocol ()
    : m_data (0), m_dataSize (0), m_size (0), m_qphyent (nullptr), m_epr ({})
{
}

TypeId
DistributeEPRSrcProtocol::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::DistributeEPRSrcProtocol")
          .AddConstructor<DistributeEPRSrcProtocol> ()
          .SetParent<Application> ()
          .AddAttribute (
              "PeerAddress", "The destination Address of the outbound packets", AddressValue (),
              MakeAddressAccessor (&DistributeEPRSrcProtocol::m_peerAddress), MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9),
                         MakeUintegerAccessor (&DistributeEPRSrcProtocol::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&DistributeEPRSrcProtocol::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&DistributeEPRSrcProtocol::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&DistributeEPRSrcProtocol::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute (
              "EPR", "The EPR pair to be distributed",
              PairValue<StringValue, StringValue> (),
              MakePairAccessor<StringValue, StringValue> (&DistributeEPRSrcProtocol::m_epr),
              MakePairChecker<StringValue, StringValue> ());
  return tid;
}

void
DistributeEPRSrcProtocol::GenerateAndDistributeEPR (const std::pair<std::string, std::string> &epr)
{
  if (epr != std::pair<std::string, std::string>{})
    SetEPR (epr);

  m_qphyent->GenerateEPR (m_conn, m_epr);
  
  Send (epr);
}

void
DistributeEPRSrcProtocol::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
DistributeEPRSrcProtocol::SetFill (std::string fill)
{
  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete[] m_data;
      m_data = new uint8_t[dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  m_size = dataSize;
}

void
DistributeEPRSrcProtocol::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
DistributeEPRSrcProtocol::Send (
    const std::pair<std::string, std::string> &epr) // generate and distribute EPR
{
  if (epr != std::pair<std::string, std::string>{})
    SetEPR (epr);

  std::string qubit = m_epr.second;
  NS_LOG_INFO (CYAN_CODE << "Distributing qubit named " << qubit << " of "
                         << m_conn->GetSrc (m_qphyent)->GetOwner () << " to "
                         << m_conn->GetDst (m_qphyent)->GetOwner () << END_CODE);

  // resign from source
  m_conn->GetSrc (m_qphyent)->RemoveQubit (qubit);

  std::string msg_send = m_epr.first + DELIM + qubit;

  SetFill (msg_send);

  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "DistributeEPRSrcProtocol::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "DistributeEPRSrcProtocol::Send(): m_dataSize but no m_data");

      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  NS_LOG_LOGIC ("@ DistributeEPRSrcProtocol::Send Going to send packet to address "
                << Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  SendPacket (p, Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort);

  NS_LOG_INFO (CYAN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                         << m_send_socket->GetNode ()->GetId () << " sent \"" << msg_send
                         << "\" to " << Ipv6Address::ConvertFrom (m_peerAddress) << END_CODE);
}


bool
DistributeEPRSrcProtocol::SetEPR (const std::pair<std::string, std::string> &epr_)
{
  NS_LOG_LOGIC ("Setting EPR " << epr_.first << " " << epr_.second
                               << " for DistributeEPRSrcProtocol of "
                               << m_conn->GetSrc (m_qphyent)->GetOwner ());
  m_epr = epr_;
  return true;
}

void
DistributeEPRSrcProtocol::StartApplication ()
{
  //Send Socket
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  NS_LOG_LOGIC ("DistributeEPRSrcProtocol Setting up peer address " << m_peerAddress << " port "
                                                                    << m_peerPort);
}

/* destination */

NS_OBJECT_ENSURE_REGISTERED (DistributeEPRDstProtocol);

DistributeEPRDstProtocol::DistributeEPRDstProtocol (Ptr<QuantumPhyEntity> qphyent_,
                                                    Ptr<QuantumChannel> conn_)
    : m_qphyent (qphyent_), m_conn (conn_), m_output ({})
{
  m_port = m_conn->GetDst (m_qphyent)->AllocPort ();
}

DistributeEPRDstProtocol::~DistributeEPRDstProtocol ()
{
}

DistributeEPRDstProtocol::DistributeEPRDstProtocol ()
    : m_port (9), m_qphyent (nullptr), m_conn (nullptr), m_output ({})
{
}

TypeId
DistributeEPRDstProtocol::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::DistributeEPRDstProtocol")
          .AddConstructor<DistributeEPRDstProtocol> ()
          .SetParent<Application> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.", UintegerValue (9),
                         MakeUintegerAccessor (&DistributeEPRDstProtocol::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&DistributeEPRDstProtocol::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&DistributeEPRDstProtocol::m_conn),
                         MakePointerChecker<QuantumChannel> ());
  return tid;
}

void
DistributeEPRDstProtocol::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
DistributeEPRDstProtocol::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (!m_qphyent)
        continue;

      unsigned size = packet->GetSize ();
      uint8_t data[size];
      packet->CopyData (data, size);

      // parse
      std::string msg_recv = std::string (data, data + sizeof (data) / sizeof (data[0]));

      NS_LOG_INFO (GREEN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                              << socket->GetNode ()->GetId ()
                              << "'s DistributeEPRDstProtocol received \"" << msg_recv << "\" from "
                              << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << END_CODE);

      std::pair<std::string, std::string> m_epr = {"", ""};
      unsigned pos = 0;
      while (pos < msg_recv.size () && msg_recv[pos] != DELIM[0])
        m_epr.first += msg_recv[pos++];
      pos++; // skip DELIM
      while (pos < msg_recv.size () && msg_recv[pos] != '\0')
        m_epr.second += msg_recv[pos++];

      std::string qubit = m_epr.second;

      // assign to destination

      m_conn->GetDst (m_qphyent)->AddQubit (qubit);

      m_qphyent->ApplyErrorModel (
          {m_conn->GetSrc (m_qphyent)->GetOwner (), m_conn->GetDst (m_qphyent)->GetOwner ()},
          m_epr);
    }
}

std::vector<std::complex<double>>
DistributeEPRDstProtocol::GetOutput () const
{
  return m_output;
}

void
DistributeEPRDstProtocol::StartApplication ()
{
  // Receive Socket
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);
  m_recv_socket->SetRecvCallback (MakeCallback (&DistributeEPRDstProtocol::HandleRead, this));
}

} // namespace ns3