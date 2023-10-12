#include "ns3/telep-lin-adapt-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-protocol.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TelepLinAdaptApp");

/* source */

NS_OBJECT_ENSURE_REGISTERED (TelepLinAdaptApp);

TelepLinAdaptApp::TelepLinAdaptApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                                    const std::pair<std::string, std::string> &epr_)
    : m_qphyent (qphyent_),
      m_conn (conn_), 
      m_port (GetNode ()->GetObject<QuantumNode> ()->AllocPort ()), // to receive from predecessor

      m_epr(epr_),
      m_qubits_pred ({"", ""}),
      m_qubits ({"", epr_.first}), 
      m_qubit (epr_.second),
      
      m_data (0),
      m_dataSize (0)
{
  if (m_conn)
    // to send to successor
    SetRemote (m_conn->GetDst (m_qphyent)->GetAddress (), m_conn->GetDst (m_qphyent)->GetNextPort ());
}

TelepLinAdaptApp::~TelepLinAdaptApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

TelepLinAdaptApp::TelepLinAdaptApp ()
  : m_qphyent (nullptr),
    m_conn (nullptr), 
    m_port (9),
    m_qubits_pred ({"", ""}),
    m_qubits ({"", ""}), 
    m_qubit (""),

    m_data (0), 
    m_dataSize (0)
{
}

TypeId
TelepLinAdaptApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::TelepLinAdaptApp")
          .AddConstructor<TelepLinAdaptApp> ()
          .SetParent<Application> ()
          .AddAttribute ("PeerAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&TelepLinAdaptApp::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9), MakeUintegerAccessor (&TelepLinAdaptApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&TelepLinAdaptApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("Port", "The port to receive from predecessor", UintegerValue (0),
                          MakeUintegerAccessor (&TelepLinAdaptApp::m_port),
                          MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&TelepLinAdaptApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&TelepLinAdaptApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("EPR", "The EPR Alice to generate and distribute the latter of which to Bob",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&TelepLinAdaptApp::m_epr),
                         MakePairChecker<StringValue, StringValue> ())
          .AddAttribute ("Input", "The pointer to the state Alice wants to teleport to Bob",
                         PointerValue (), MakePointerAccessor (&TelepLinAdaptApp::m_input),
                         MakePointerChecker<Qubit> ());
  return tid;
}

void
TelepLinAdaptApp::Teleport ()
{
  assert(m_conn); // should not be the last owner
  NS_LOG_LOGIC ("Teleport at time " << Simulator::Now () << " from " << m_conn->GetSrcOwner ()
                                     << " to " << m_conn->GetDstOwner () << " using EPR "
                                     << m_epr.first << " " << m_epr.second);

  m_qubits.second = m_epr.first;
  m_qubit = m_epr.second;

  // generate and distribute EPR
  Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
      m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR).first->GetObject<DistributeEPRSrcProtocol> ();
  Simulator::ScheduleNow (&DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                          dist_epr_src_app, m_epr);
  
  // local operations
  Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent,
                       m_conn->GetSrcOwner (), QNS_GATE_PREFIX + "CNOT",
                       std::vector<std::complex<double>>{},
                       std::vector<std::string>{m_qubits.second, m_qubits.first});
  Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent,
                       m_conn->GetSrcOwner (), QNS_GATE_PREFIX + "H",
                       std::vector<std::complex<double>>{},
                       std::vector<std::string>{m_qubits.first});

  if (! m_input) // not the first owner in the chain
    {
      // God pass the xor result from Alice's predecessor to Alice
      Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                          QNS_GATE_PREFIX + "CNOT", std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits.second, m_qubits_pred.second});
      // Alice's predecessor's second qubit is not used anymore
      Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{m_qubits_pred.second});
      Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                          QNS_GATE_PREFIX + "CNOT", std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits.first, m_qubits_pred.first});
      // Alice's predecessor's first qubit is not used anymore
      Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{m_qubits_pred.first});
    }
    // Alice send a classical message to invoke the next teleportation (from Bob to Bob's successor)
    Simulator::Schedule (Seconds (CLASSICAL_DELAY), &TelepLinAdaptApp::Send, this);

}

void
TelepLinAdaptApp::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_LOGIC ("SetRemote " << ip << " " << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
TelepLinAdaptApp::SetFill (std::string fill)
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
TelepLinAdaptApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
TelepLinAdaptApp::Send ()
{
  std::string msg_send = m_qubits.first + DELIM + m_qubits.second + DELIM + m_qubit;
  SetFill (msg_send);

  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "TelepLinAdaptApp::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "TelepLinAdaptApp::Send(): m_dataSize but no m_data");

      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  NS_LOG_LOGIC ("Sending packet to " << m_peerAddress << " Port " << m_peerPort << " DataSize "
                                     << m_dataSize << " Data " << m_data
                                     << " from " << GetNode ()->GetObject<QuantumNode> ()->GetOwner ()
                                     << " at time " << Simulator::Now ());
  SendPacket (p, Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort);
  NS_LOG_INFO (CYAN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                         << m_send_socket->GetNode ()->GetId () << " sent \"" << m_data << "\" to "
                         << Ipv6Address::ConvertFrom (m_peerAddress) << END_CODE);
}


void
TelepLinAdaptApp::HandleRead (Ptr<Socket> socket)
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

      NS_LOG_INFO (GREEN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                              << socket->GetNode ()->GetId () << "'s TelepDstApp received \""
                              << data << "\" from "
                              << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << END_CODE);
      // parse
      std::string msg_recv = std::string (data, data + sizeof (data) / sizeof (data[0]));
      std::string qubits_first = "";
      std::string qubits_second = "";
      std::string qubit = "";
      unsigned pos = 0;
      while (pos < msg_recv.size () && msg_recv[pos] != DELIM[0])
        qubits_first += msg_recv[pos++];
      pos++; // skip DELIM
      while (pos < msg_recv.size () && msg_recv[pos] != DELIM[0])
        qubits_second += msg_recv[pos++];
      pos++; // skip DELIM
      while (pos < msg_recv.size () && msg_recv[pos] != '\0')
        qubit += msg_recv[pos++];

      // set Alice's qubits according to the received message from Alice's predecessor
      m_qubits_pred = {qubits_first, qubits_second};
      m_qubits.first = qubit;
      NS_LOG_LOGIC (GetNode()->GetObject<QuantumNode>()->GetOwner() << " sees "
                    << m_qubits_pred.first << " " << m_qubits_pred.second << " " << m_qubits.first);

      
      if (m_epr.first == "" && m_epr.second == "") // last owner in the chain
        {
          // perform correction
          Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControlledOperation, m_qphyent, 
                                  GetNode ()->GetObject<QuantumNode> ()->GetOwner (),  
                                  QNS_GATE_PREFIX + "X", QNS_GATE_PREFIX + "CX", cnot,
                                  std::vector<std::string>{m_qubits_pred.second},
                                  std::vector<std::string>{m_qubits.first});
          // the latter qubit of the last but one owner is not used anymore
          Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                  std::vector<std::string>{m_qubits_pred.second});
          Simulator::ScheduleNow (&QuantumPhyEntity::ApplyControlledOperation, m_qphyent,
                                  GetNode ()->GetObject<QuantumNode> ()->GetOwner (),
                                  QNS_GATE_PREFIX + "Z", QNS_GATE_PREFIX + "CZ", cz,
                                  std::vector<std::string>{m_qubits_pred.first},
                                  std::vector<std::string>{m_qubits.first});
          // the former qubit of the last but one owner is not used anymore
          Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                                  std::vector<std::string>{m_qubits_pred.first});

          // contract and check the result
          Simulator::ScheduleNow (&QuantumPhyEntity::Contract, m_qphyent, "ascend");
          Simulator::ScheduleNow (&QuantumPhyEntity::PeekDM, m_qphyent,
                                  GetNode ()->GetObject<QuantumNode> ()->GetOwner (),
                                  std::vector<std::string>{m_qubits.first}, m_output);

        }
      else
        {
          Teleport ();
        }

    }
}

void
TelepLinAdaptApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  NS_LOG_LOGIC ("SetupReceiveSocket for " << GetNode ()->GetObject<QuantumNode> ()->GetOwner ()
                                          << " with addr " << Ipv6Address::ConvertFrom (GetNode ()->GetObject<QuantumNode> ()->GetAddress ())
                                          << " port " << port);
  Inet6SocketAddress local = 
    Inet6SocketAddress (Ipv6Address::ConvertFrom (GetNode ()->GetObject<QuantumNode> ()->GetAddress ()), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
TelepLinAdaptApp::StartApplication ()
{
  NS_LOG_LOGIC ("StartApplication at time " << Simulator::Now ());
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  if (m_conn) // not the last owner in the chain
    {
      // Send socket
      m_send_socket = Socket::CreateSocket (GetNode (), tid);
    }

  // Receive sockets
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  if (m_port == 0)
    m_port = GetNode ()->GetObject<QuantumNode> ()->AllocPort ();
  SetupReceiveSocket (m_recv_socket, m_port);

  m_recv_socket->SetRecvCallback (MakeCallback (&TelepLinAdaptApp::HandleRead, this));

  if (m_input) // first owner in the chain
    {
      m_qubits.first = m_input->GetName ();
      if (! m_input->GetStateVector ().empty())
        {
          // generate psi
          Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                                  m_conn->GetSrcOwner (), m_input->GetStateVector (),
                                  std::vector<std::string>{m_qubits.first});
        }
      // starts the teleportation chain
      Teleport ();
    }
}

void
TelepLinAdaptApp::SetQubits (const std::pair<std::string, std::string> &qubits_)
{
  NS_LOG_LOGIC ("Setting qubits " << qubits_.first << " " << qubits_.second);
  m_qubits = qubits_;
}

void
TelepLinAdaptApp::SetQubit (const std::string &qubit_)
{
  m_qubit = qubit_;
}

void
TelepLinAdaptApp::SetInput (Ptr<Qubit> input_)
{
  m_input = input_;
}

std::vector<std::complex<double>>
TelepLinAdaptApp::GetOutput () const
{
  return m_output;
}

} // namespace ns3