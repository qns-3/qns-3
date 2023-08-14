#include "ns3/telep-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-protocol.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TelepApp");


/* source */

NS_OBJECT_ENSURE_REGISTERED (TelepSrcApp);

TelepSrcApp::TelepSrcApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                          const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_),
      m_conn (conn_),
      m_qubits (qubits_),

      m_data (0),
      m_dataSize (0)
{
  SetRemote (m_conn->GetDst (m_qphyent)->GetAddress (), m_conn->GetDst (m_qphyent)->GetNextPort ());
}

TelepSrcApp::~TelepSrcApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

TelepSrcApp::TelepSrcApp ()
    : m_qphyent (nullptr), m_conn (nullptr), m_qubits ({"", ""}), m_data (0), m_dataSize (0)
{
}

TypeId
TelepSrcApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::TelepSrcApp")
          .AddConstructor<TelepSrcApp> ()
          .SetParent<Application> ()
          .AddAttribute ("PeerAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&TelepSrcApp::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9), MakeUintegerAccessor (&TelepSrcApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&TelepSrcApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&TelepSrcApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&TelepSrcApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("Qubits", "The two qubits of Alice",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&TelepSrcApp::m_qubits),
                         MakePairChecker<StringValue, StringValue> ())
          .AddAttribute ("Qubit", "The qubit of Bob", StringValue (),
                         MakeStringAccessor (&TelepSrcApp::m_qubit), MakeStringChecker ())
          .AddAttribute ("Input", "The pointer to the state Alice wants to teleport to Bob",
                         PointerValue (), MakePointerAccessor (&TelepSrcApp::m_input),
                         MakePointerChecker<Qubit> ());
  return tid;
}

void
TelepSrcApp::Teleport ()
{
  NS_LOG_LOGIC ("Teleport at time " << Simulator::Now ());

  // generate psi
  if (m_input)
    {
      Simulator::ScheduleNow (&QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                              m_conn->GetSrcOwner (), m_input->GetStateVector (),
                              std::vector<std::string>{m_qubits.first});
    }

  // generate and distribute EPR
  Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
      m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR).first->GetObject<DistributeEPRSrcProtocol> ();
  Simulator::ScheduleNow (&DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                          dist_epr_src_app,
                          std::pair<std::string, std::string>{m_qubits.second, m_qubit});

  // local operations
  Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent,
                          m_conn->GetSrcOwner (), QNS_GATE_PREFIX + "CNOT",
                          std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits.second, m_qubits.first});
  Simulator::Schedule (Seconds (CLASSICAL_DELAY), &QuantumPhyEntity::ApplyGate, m_qphyent,
                          m_conn->GetSrcOwner (), QNS_GATE_PREFIX + "H",
                          std::vector<std::complex<double>>{},
                          std::vector<std::string>{m_qubits.first});


  Simulator::Schedule (Seconds (CLASSICAL_DELAY), &TelepSrcApp::MeasureAndSend, this);
}

void
TelepSrcApp::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
TelepSrcApp::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete[] m_data;
      m_data = new uint8_t[dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  memcpy (&m_data[filled], fill, dataSize - filled);

  m_size = dataSize;
}

void
TelepSrcApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
TelepSrcApp::MeasureAndSend ()
{
  std::pair<unsigned, std::vector<double>> outcome_Q0 =
      m_qphyent->Measure (m_conn->GetSrcOwner (), {m_qubits.first});
  NS_LOG_LOGIC ("     => " << m_conn->GetSrcOwner ()
                           << "'s former qubit is measured to outcome-0 = " << outcome_Q0.first);
  std::pair<unsigned, std::vector<double>> outcome_Q1 =
      m_qphyent->Measure (m_conn->GetSrcOwner (), {m_qubits.second});
  NS_LOG_LOGIC ("     => " << m_conn->GetSrcOwner ()
                           << "'s latter qubit is measured to outcome-1 = " << outcome_Q1.first);

  std::string outcomes_send = std::to_string (outcome_Q0.first) + std::to_string (outcome_Q1.first);
  SetFill ((uint8_t *) &outcomes_send[0], 3, 1024);

  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "TelepSrcApp::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "TelepSrcApp::Send(): m_dataSize but no m_data");

      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  NS_LOG_LOGIC ("Sending packet to " << m_peerAddress << " Port " << m_peerPort << " DataSize "
                                     << m_dataSize << " Data " << m_data);
  SendPacket (p, Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort);
  NS_LOG_INFO (CYAN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                         << m_send_socket->GetNode ()->GetId () << " sent \"" << m_data << "\" to "
                         << Ipv6Address::ConvertFrom (m_peerAddress) << END_CODE);
}

void
TelepSrcApp::SetQubits (const std::pair<std::string, std::string> &qubits_)
{
  NS_LOG_LOGIC ("Setting qubits " << qubits_.first << " " << qubits_.second);
  m_qubits = qubits_;
}

void
TelepSrcApp::SetQubit (const std::string &qubit_)
{
  m_qubit = qubit_;
}

void
TelepSrcApp::SetInput (Ptr<Qubit> input_)
{
  m_input = input_;
}

void
TelepSrcApp::StartApplication ()
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  Teleport ();
}

/* destination */

NS_OBJECT_ENSURE_REGISTERED (TelepDstApp);

TelepDstApp::TelepDstApp (Ptr<QuantumPhyEntity> qphyent_,
                          const std::pair<std::string, std::string> &conn_,
                          const std::string &qubit_)
    : m_qphyent (qphyent_),
      m_pnode (qphyent_->GetNode (conn_.second)),
      m_qubit (qubit_),
      m_port (qphyent_->GetNode (conn_.second)->AllocPort ())
{
}

TelepDstApp::~TelepDstApp ()
{
}

TelepDstApp::TelepDstApp ()
    : m_qphyent (nullptr),
      m_pnode (nullptr),
      m_qubit (""),

      m_port (9)
{
}

TypeId
TelepDstApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::TelepDstApp")
          .AddConstructor<TelepDstApp> ()
          .SetParent<Application> ()
          .AddAttribute ("Port", "The destination port of the outbound packets", UintegerValue (9),
                         MakeUintegerAccessor (&TelepDstApp::m_port),
                         MakeUintegerChecker<uint16_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&TelepDstApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("Pnode", "The pointer to the quantum node owning this app", PointerValue (),
                         MakePointerAccessor (&TelepDstApp::m_pnode),
                         MakePointerChecker<QuantumNode> ())
          .AddAttribute ("Qubit", "The qubits of Bob", StringValue (),
                         MakeStringAccessor (&TelepDstApp::m_qubit), MakeStringChecker ());
  return tid;
}

void
TelepDstApp::HandleRead (Ptr<Socket> socket)
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

      std::string x_correction_gate =
          (data[1] - '0') ? QNS_GATE_PREFIX + "PX" : QNS_GATE_PREFIX + "I";
      m_qphyent->ApplyGate (m_pnode->GetOwner (), x_correction_gate, {}, {m_qubit});

      std::string z_correction_gate =
          (data[0] - '0') ? QNS_GATE_PREFIX + "PZ" : QNS_GATE_PREFIX + "I";
      m_qphyent->ApplyGate (m_pnode->GetOwner (), z_correction_gate, {}, {m_qubit});

      // if (m_pnode->GetOwner () == "Owner7")
      // m_qphyent->PeekDM (m_pnode->GetOwner (), {m_qubit}, m_output);
    }
}

void
TelepDstApp::SetQubit (const std::string &qubit_)
{
  m_qubit = qubit_;
}

std::vector<std::complex<double>>
TelepDstApp::GetOutput () const
{
  return m_output;
}

void
TelepDstApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local =
      Inet6SocketAddress (Ipv6Address::ConvertFrom (m_pnode->GetAddress ()), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
TelepDstApp::StartApplication ()
{
  // Receive sockets
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);

  m_recv_socket->SetRecvCallback (MakeCallback (&TelepDstApp::HandleRead, this));
}

} // namespace ns3