#include "ns3/distill-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-protocol.h" // class DistributeEPRSrcProtocol

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistillApp");

NS_OBJECT_ENSURE_REGISTERED (DistillApp);

DistillApp::DistillApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_, Ptr<QuantumChannel> conn_,
                        const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_),
      m_checker (checker_),
      m_conn (conn_),
      m_qubits (qubits_),
      m_win (false),
      m_data (0),
      m_dataSize (0)
{
  if (!m_checker)
    { // Alice
      m_pnode = m_conn->GetSrc (m_qphyent);
      SetRemote (m_conn->GetDst (m_qphyent)->GetAddress (),
                 m_conn->GetDst (m_qphyent)->GetNextPort ());
      m_port = m_pnode->AllocPort ();
    }
  else
    { // Bob
      m_pnode = m_conn->GetDst (m_qphyent);
      SetRemote (m_conn->GetSrc (m_qphyent)->GetAddress (),
                 m_conn->GetSrc (m_qphyent)->GetNextPort () - 1);
      m_port = m_pnode->AllocPort ();
    }
}

DistillApp::~DistillApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

DistillApp::DistillApp ()
    : m_qphyent (nullptr),
      m_checker (false),
      m_pnode (nullptr),
      m_qubits ({"", ""}),
      m_win (false),

      m_port (9),
      m_peerPort (9),
      m_size (0),
      m_data (0),
      m_dataSize (0)
{
}

TypeId
DistillApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::DistillApp")
          .SetParent<Application> ()
          .AddConstructor<DistillApp> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.", UintegerValue (9),
                         MakeUintegerAccessor (&DistillApp::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PeerAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&DistillApp::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9), MakeUintegerAccessor (&DistillApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&DistillApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&DistillApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("Checker", "Whether the owner is the checker (Bob) in the distillation",
                         BooleanValue (false), MakeBooleanAccessor (&DistillApp::m_checker),
                         MakeBooleanChecker ())
          .AddAttribute ("PNode", "The pointer to the quantum node owning this app", PointerValue (),
                         MakePointerAccessor (&DistillApp::m_pnode),
                         MakePointerChecker<QuantumNode> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&DistillApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("EPRGoal", "The goal EPR pair of the distillation",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&DistillApp::m_epr_goal),
                         MakePairChecker<StringValue, StringValue> ())
          .AddAttribute ("EPRMeas", "The epr pair to be measured",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&DistillApp::m_epr_meas),
                         MakePairChecker<StringValue, StringValue> ())
          .AddAttribute ("Qubits", "The two qubits of this owner",
                         PairValue<StringValue, StringValue> (),
                         MakePairAccessor<StringValue, StringValue> (&DistillApp::m_qubits),
                         MakePairChecker<StringValue, StringValue> ());
  return tid;
}

void
DistillApp::Distillate ()
{
  if (!m_checker)
    { // Alice
      Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
          m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR)
              .first->GetObject<DistributeEPRSrcProtocol> ();
      Simulator::Schedule (Seconds (2.1), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                           dist_epr_src_app, m_epr_goal);
      std::vector<std::complex<double>> unused;
      // Simulator::Schedule (Seconds (2.2), &QuantumPhyEntity::PeekDM, m_qphyent, "God",
      //                      std::vector<std::string>{m_epr_goal.first, m_epr_goal.second}, unused);
      Simulator::Schedule (Seconds (2.2), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                           dist_epr_src_app, m_epr_meas);
      // Simulator::Schedule (Seconds (2.3), &QuantumPhyEntity::PeekDM, m_qphyent, "God",
      //                      std::vector<std::string>{m_epr_meas.first, m_epr_meas.second}, unused);
      Simulator::Schedule (Seconds (2.3), &DistillApp::Send, this);
    }
  else
    { // Bob
    }
}

void
DistillApp::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}


void
DistillApp::SetFill (std::string fill)
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
DistillApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
DistillApp::Send ()
{
  if (!m_checker)
    { // Alice
      m_qphyent->ApplyGate (m_pnode->GetOwner (), QNS_GATE_PREFIX + "CNOT", {},
                            {m_epr_meas.first, m_epr_goal.first});
      std::pair<unsigned, std::vector<double>> outcome_A =
          m_qphyent->Measure (m_pnode->GetOwner (), {m_epr_meas.first});
      NS_LOG_LOGIC ("     => " << m_pnode << "'s qubit is measured to " << outcome_A.first);

      std::string outcome_send = std::to_string (outcome_A.first);
      SetFill (outcome_send);
    }
  else
    { // Bob
    }

  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "DistillApp::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "DistillApp::Send(): m_dataSize but no m_data");

      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  SendPacket (p, Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort);
  NS_LOG_INFO (CYAN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Node # "
                         << m_send_socket->GetNode ()->GetId () << " sent \"" << m_data << "\" to "
                         << Ipv6Address::ConvertFrom (m_peerAddress) << END_CODE);
}

void
DistillApp::HandleRead (Ptr<Socket> socket)
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
                              << socket->GetNode ()->GetId () << " received \"" << data
                              << "\" from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                              << END_CODE);

      if (!m_checker)
        { // Alice
          std::string msg_from_B = std::string (data, data + sizeof (data) / sizeof (data[0]));
          if (strncmp (msg_from_B.c_str (), "win", 3) == 0)
            {
              NS_LOG_INFO (m_pnode << " finds out that DISTILL wins!\n");
              m_win = true;
            }
          else if (strncmp (msg_from_B.c_str (), "lose", 4) == 0)
            {
              NS_LOG_INFO (m_pnode << " finds out that DISTILL loses!\n");
              m_win = false;
            }
          std::vector<std::complex<double>> output;
          // m_qphyent->PeekDM ("God", {m_epr_goal.first, m_epr_goal.second}, output);
        }
      else
        { // Bob
          unsigned outcome_A_recv = data[0] - '0';
          m_qphyent->ApplyGate (m_pnode->GetOwner (), QNS_GATE_PREFIX + "CNOT", {},
                                {m_qubits.second, m_qubits.first});
          std::pair<unsigned, std::vector<double>> outcome_B =
              m_qphyent->Measure (m_pnode->GetOwner (), {m_qubits.second});
          NS_LOG_LOGIC ("     => " << m_pnode << "'s qubit is measured to " << outcome_B.first);

          std::string msg_send = "";
          if (outcome_A_recv == outcome_B.first)
            { // same
              NS_LOG_INFO (m_pnode << " finds out that DISTILL wins!\n");
              msg_send = "win";
              m_win = true;
            }
          else
            { // different
              NS_LOG_INFO (m_pnode << " finds out that DISTILL loses!\n");
              msg_send = "lose";
              m_win = false;
            }
          SetFill (msg_send);
          Send ();
        }
    }
}

void
DistillApp::SetEPRGoal (const std::pair<std::string, std::string> &epr_goal_)
{
  m_epr_goal = epr_goal_;
}

void
DistillApp::SetEPRMeas (const std::pair<std::string, std::string> &epr_meas_)
{
  m_epr_meas = epr_meas_;
}

void
DistillApp::SetQubits (const std::pair<std::string, std::string> &qubits_)
{
  m_qubits = qubits_;
}

bool
DistillApp::GetWin () const
{
  return m_win;
}

void
DistillApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
DistillApp::StartApplication ()
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);
  m_recv_socket->SetRecvCallback (MakeCallback (&DistillApp::HandleRead, this));

  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  if (!m_checker)
    {
      NS_LOG_LOGIC ("DistillApp of Alice has peer address " << m_peerAddress << " and port "
                                                      << m_peerPort);
      Distillate ();
    }
  else
    {
      NS_LOG_LOGIC ("DistillApp of Bob has address " << m_pnode->GetAddress () << " and port "
                                                     << m_port);
    }
}

} // namespace ns3