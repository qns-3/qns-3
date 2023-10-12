#include "ns3/ent-swap-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("EntSwapApp");


/* source */

NS_OBJECT_ENSURE_REGISTERED (EntSwapSrcApp);

EntSwapSrcApp::EntSwapSrcApp (Ptr<QuantumPhyEntity> qphyent_, Ptr<QuantumChannel> conn_,
                          const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_),
      m_conn (conn_),
      m_qubits (qubits_),

      m_data (0),
      m_dataSize (0)
{
  SetRemote (m_conn->GetDst (m_qphyent)->GetAddress (), m_conn->GetDst (m_qphyent)->GetNextPort ());
}

EntSwapSrcApp::~EntSwapSrcApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

EntSwapSrcApp::EntSwapSrcApp ()
    : m_qphyent (nullptr), m_conn (nullptr), m_qubits ({"", ""}), m_data (0), m_dataSize (0)
{
}

TypeId
EntSwapSrcApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::EntSwapSrcApp")
          .AddConstructor<EntSwapSrcApp> ()
          .SetParent<Application> ()
          .AddAttribute ("PeerAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&EntSwapSrcApp::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9), MakeUintegerAccessor (&EntSwapSrcApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&EntSwapSrcApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&EntSwapSrcApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&EntSwapSrcApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("Qubits", "The two qubits of this owner",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&EntSwapSrcApp::m_qubits),
                         MakePairChecker<StringValue, StringValue> ())
          ;
  return tid;
}

void
EntSwapSrcApp::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
EntSwapSrcApp::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
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
EntSwapSrcApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
EntSwapSrcApp::MeasureAndSend ()
{
  m_qphyent->ApplyGate ("God", QNS_GATE_PREFIX + "CNOT", // noiseless
                        std::vector<std::complex<double>>{},
                        std::vector<std::string>{m_qubits.second, m_qubits.first});
  m_qphyent->ApplyGate ("God", QNS_GATE_PREFIX + "H", // noiseless
                        std::vector<std::complex<double>>{},
                        std::vector<std::string>{m_qubits.first});

  std::pair<unsigned, std::vector<double>> outcome_Q0 =
      m_qphyent->Measure (m_conn->GetSrcOwner (), {m_qubits.first});
  NS_LOG_LOGIC ("     => " << m_conn->GetSrcOwner ()
                           << "'s former qubit is measured to outcome-0 = " << outcome_Q0.first);
  std::pair<unsigned, std::vector<double>> outcome_Q1 =
      m_qphyent->Measure (m_conn->GetSrcOwner (), {m_qubits.second});
  NS_LOG_LOGIC ("     => " << m_conn->GetSrcOwner ()
                           << "'s latter qubit is measured to outcome-1 = " << outcome_Q1.first);

  
  Simulator::ScheduleNow (&QuantumPhyEntity::PartialTrace, m_qphyent,
                          std::vector<std::string>{m_qubits.first, m_qubits.second});

  std::string outcomes_send = std::to_string (outcome_Q0.first) + std::to_string (outcome_Q1.first);
  SetFill ((uint8_t *) &outcomes_send[0], 3, 1024);

  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "EntSwapSrcApp::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "EntSwapSrcApp::Send(): m_dataSize but no m_data");

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
EntSwapSrcApp::SetQubits (const std::pair<std::string, std::string> &qubits_)
{
  NS_LOG_LOGIC ("Setting qubits " << qubits_.first << " " << qubits_.second);
  m_qubits = qubits_;
}

void
EntSwapSrcApp::StartApplication ()
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  Simulator::ScheduleNow (&EntSwapSrcApp::MeasureAndSend, this);
}

/* destination */

NS_OBJECT_ENSURE_REGISTERED (EntSwapDstApp);

EntSwapDstApp::EntSwapDstApp (Ptr<QuantumPhyEntity> qphyent_,
                          Ptr<QuantumNode> pnode_,
                          const std::string &qubit_)
    : m_qphyent (qphyent_),
      m_pnode (pnode_),
      m_qubit (qubit_),
      m_count (0),
      m_flag_x (false),
      m_flag_z (false),

      m_port (pnode_->AllocPort ())
{
}

EntSwapDstApp::~EntSwapDstApp ()
{
}

EntSwapDstApp::EntSwapDstApp ()
    : m_qphyent (nullptr),
      m_pnode (nullptr),
      m_qubit (""),
      m_count (0),
      m_flag_x (false),
      m_flag_z (false),

      m_port (9)
{
}

TypeId
EntSwapDstApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::EntSwapDstApp")
          .AddConstructor<EntSwapDstApp> ()
          .SetParent<Application> ()
          .AddAttribute ("Port", "The destination port of the outbound packets", UintegerValue (9),
                         MakeUintegerAccessor (&EntSwapDstApp::m_port),
                         MakeUintegerChecker<uint16_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&EntSwapDstApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("Pnode", "The pointer to the quantum node owning this app", PointerValue (),
                         MakePointerAccessor (&EntSwapDstApp::m_pnode),
                         MakePointerChecker<QuantumNode> ())
          .AddAttribute ("Qubit", "The qubit of the last owner", StringValue (),
                         MakeStringAccessor (&EntSwapDstApp::m_qubit), MakeStringChecker ())
          .AddAttribute ("Count", "The number of packets to receive from middle nodes", UintegerValue (0),
                         MakeUintegerAccessor (&EntSwapDstApp::m_count),
                         MakeUintegerChecker<unsigned> ())
          ;
  return tid;
}

void
EntSwapDstApp::HandleRead (Ptr<Socket> socket)
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
                              << socket->GetNode ()->GetId () << "'s EntSwapDstApp received \""
                              << data << "\" from "
                              << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << END_CODE);

      m_flag_x ^= (data[1] - '0');
      m_flag_z ^= (data[0] - '0');
      m_count--;
      
      if (m_count)
        continue;

      std::string x_correction_gate =
          m_flag_x ? QNS_GATE_PREFIX + "PX" : QNS_GATE_PREFIX + "I";
      m_qphyent->ApplyGate (m_pnode->GetOwner (), x_correction_gate, {}, {m_qubit});

      std::string z_correction_gate =
          m_flag_z ? QNS_GATE_PREFIX + "PZ" : QNS_GATE_PREFIX + "I";
      m_qphyent->ApplyGate (m_pnode->GetOwner (), z_correction_gate, {}, {m_qubit});

    }
}

void
EntSwapDstApp::SetQubit (const std::string &qubit_)
{
  m_qubit = qubit_;
}

void
EntSwapDstApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local =
      Inet6SocketAddress (Ipv6Address::ConvertFrom (m_pnode->GetAddress ()), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
EntSwapDstApp::StartApplication ()
{
  // Receive sockets
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);

  m_recv_socket->SetRecvCallback (MakeCallback (&EntSwapDstApp::HandleRead, this));
}

} // namespace ns3