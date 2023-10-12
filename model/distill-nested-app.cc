#include "ns3/distill-nested-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-memory.h" // class QuantumMemory
#include "ns3/distribute-epr-protocol.h" // class DistributeEPRSrcProtocol


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistillNestedApp");

NS_OBJECT_ENSURE_REGISTERED (DistillNestedApp);

DistillNestedApp::DistillNestedApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_,
                                    Ptr<QuantumChannel> conn_,
                                    const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_),
      m_checker (checker_),
      m_conn (conn_),
      m_win (false),
      m_data (0),
      m_dataSize (0),
      m_src_qubits (nullptr),
      m_dst_qubits (nullptr),
      m_occupied (Seconds (0))
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

DistillNestedApp::~DistillNestedApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

DistillNestedApp::DistillNestedApp ()
    : m_qphyent (nullptr),
      m_checker (false),
      m_pnode (nullptr),
      m_src_qubits (nullptr),
      m_dst_qubits (nullptr),
      m_win (false),
      m_occupied (Seconds (0)),

      m_port (9),
      m_peerPort (9),
      m_size (0),
      m_data (0),
      m_dataSize (0)
{
}

TypeId
DistillNestedApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::DistillNestedApp")
          .SetParent<Application> ()
          .AddConstructor<DistillNestedApp> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.", UintegerValue (9),
                         MakeUintegerAccessor (&DistillNestedApp::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PeerAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&DistillNestedApp::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9), MakeUintegerAccessor (&DistillNestedApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&DistillNestedApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&DistillNestedApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute ("Checker", "Whether the owner is the checker (Bob) in the distillation",
                         BooleanValue (false), MakeBooleanAccessor (&DistillNestedApp::m_checker),
                         MakeBooleanChecker ())
          .AddAttribute ("PNode", "The pointer to the quantum node owning this app", PointerValue (),
                         MakePointerAccessor (&DistillNestedApp::m_pnode),
                         MakePointerChecker<QuantumNode> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&DistillNestedApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("SrcQubits", "The pointer to the qubits of the source owner",
                         PointerValue (), MakePointerAccessor (&DistillNestedApp::m_src_qubits),
                         MakePointerChecker<QuantumMemory> ())
          .AddAttribute ("DstQubits", "The pointer to the qubits of the destination owner",
                         PointerValue (), MakePointerAccessor (&DistillNestedApp::m_dst_qubits),
                         MakePointerChecker<QuantumMemory> ());
  return tid;
}

void
DistillNestedApp::Distillate (
    const std::vector<std::string> &src_qubits,
    const std::vector<std::string>
        &dst_qubits)
{
  NS_LOG_LOGIC ("Distillating");
  if (!m_checker)
    { // Alice
      assert (src_qubits.size () == dst_qubits.size ());
      unsigned pairs = src_qubits.size ();

      if (pairs > 2) // recursively schedule to distillate prefix half and the suffix half
        {
          NS_LOG_LOGIC ("pairs = " << pairs << " > 2");

          assert (m_qphyent);
          Ptr<DistillNestedApp> dstApp = m_qphyent->GetConn2Apps (m_conn, APP_DISTILL_NESTED)
                                             .second->GetObject<DistillNestedApp> ();
          assert (dstApp);

          Distillate (GetPreHalf (src_qubits), GetPreHalf (dst_qubits));

          Distillate (GetSufHalf (src_qubits), GetSufHalf (dst_qubits));
        }
      else // generate and distillate the two EPR pairs
        {
          assert (pairs == 2);
          NS_LOG_LOGIC ("pairs = " << pairs);

          std::pair<std::string, std::string> epr_goal = {src_qubits[0], dst_qubits[0]};
          std::pair<std::string, std::string> epr_meas = {src_qubits[1], dst_qubits[1]};

          Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
              m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR)
                  .first->GetObject<DistributeEPRSrcProtocol> ();
          Simulator::Schedule (GetOccupied (), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                               dist_epr_src_app, epr_goal);
          Occupy (Seconds (0.1));
          Simulator::Schedule (GetOccupied (), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                               dist_epr_src_app, epr_meas);
          Occupy (Seconds (0.1));
        }

      NS_LOG_LOGIC ("Scheduling a DistillateOnce at occupied time " << GetOccupied ().As (Time::S));
      
      Simulator::Schedule (GetOccupied (), &DistillNestedApp::DistillateOnce, this, src_qubits,
                           dst_qubits); // distillate the two EPR pairs to get one
      Occupy (Seconds (0.1));
      
    }
  else
    { // Bob
    }
}

void
DistillNestedApp::DistillateOnce (
    const std::vector<std::string> &src_qubits,
    const std::vector<std::string>
        &dst_qubits)
{
  if (!m_checker)
    { // Alice

      std::pair<std::string, std::string> epr_goal = {src_qubits[0], dst_qubits[0]};
      NS_LOG_LOGIC ("Alice's goal EPR pair is " << epr_goal.first << " " << epr_goal.second);
      std::pair<std::string, std::string> epr_meas = {src_qubits[src_qubits.size () / 2],
                                                      dst_qubits[dst_qubits.size () / 2]};
      NS_LOG_LOGIC ("Alice's meas EPR pair is " << epr_meas.first << " " << epr_meas.second);

      m_qphyent->ApplyGate (m_pnode->GetOwner (), QNS_GATE_PREFIX + "CNOT", {},
                            {epr_meas.first, epr_goal.first});
      std::pair<unsigned, std::vector<double>> outcome_A =
          m_qphyent->Measure (m_pnode->GetOwner (), {epr_meas.first});
      NS_LOG_LOGIC ("     => " << m_pnode << "'s qubit is measured to " << outcome_A.first);

      std::string msg_send =
          std::to_string (outcome_A.first) + DELIM + epr_goal.second + DELIM + epr_meas.second;
      SetFill (msg_send);
    }
  else
    { // Bob
    }

  Send ();
}

void
DistillNestedApp::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
DistillNestedApp::SetFill (std::string fill)
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
DistillNestedApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
DistillNestedApp::Send ()
{
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "DistillNestedApp::DistillateOnce(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "DistillNestedApp::DistillateOnce(): m_dataSize but no m_data");

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
DistillNestedApp::HandleRead (Ptr<Socket> socket)
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
              NS_LOG_LOGIC (m_pnode << " finds out that DISTILL wins!\n");
              m_win = true;
            }
          else if (strncmp (msg_from_B.c_str (), "lose", 4) == 0)
            {
              NS_LOG_LOGIC (m_pnode << " finds out that DISTILL loses!\n");
              m_win = false;
            }
        }
      else
        { // Bob
          // parse
          unsigned outcome_A_recv = data[0] - '0';
          std::string dst_qubit_goal = "";
          std::string dst_qubit_meas = "";
          unsigned pos = 2;
          while (data[pos] != DELIM[0])
            {
              dst_qubit_goal += data[pos];
              ++pos;
            }
          ++pos;
          while (data[pos] != '\0')
            {
              dst_qubit_meas += data[pos];
              ++pos;
            }
          NS_LOG_LOGIC ("Bob's goal qubit is " << dst_qubit_goal << " meas qubit is "
                                               << dst_qubit_meas);

          m_qphyent->ApplyGate (m_pnode->GetOwner (), QNS_GATE_PREFIX + "CNOT", {},
                                {dst_qubit_meas, dst_qubit_goal});
          std::pair<unsigned, std::vector<double>> outcome_B =
              m_qphyent->Measure (m_pnode->GetOwner (), {dst_qubit_meas});
          NS_LOG_LOGIC ("     => " << m_pnode << "'s qubit is measured to " << outcome_B.first);

          std::string msg_send = "";
          if (outcome_A_recv == outcome_B.first)
            { // same
              NS_LOG_LOGIC (m_pnode << " finds out that DISTILL wins!\n");
              msg_send = "win";
              m_win = true;
            }
          else
            { // different
              NS_LOG_LOGIC (m_pnode << " finds out that DISTILL loses!\n");
              msg_send = "lose";
              m_win = false;
            }
          SetFill (msg_send);
          Send ();
        }
    }
}

void
DistillNestedApp::SetSrcQubits (Ptr<QuantumMemory> src_qubits)
{
  NS_LOG_LOGIC ("Occupied time = " << GetOccupied ().As (Time::S) << " s");
  NS_LOG_LOGIC ("Setting src_qubits to ");
  for (unsigned i = 0; i < src_qubits->GetSize (); ++i)
    NS_LOG_LOGIC (src_qubits->GetQubit (i) << " ");

  m_src_qubits = src_qubits;
}

void
DistillNestedApp::SetDstQubits (Ptr<QuantumMemory> dst_qubits)
{
  NS_LOG_LOGIC ("Setting dst_qubits to ");
  for (unsigned i = 0; i < dst_qubits->GetSize (); ++i)
    NS_LOG_LOGIC (dst_qubits->GetQubit (i) << " ");
  m_dst_qubits = dst_qubits;
}

bool
DistillNestedApp::GetWin () const
{
  return m_win;
}

Time
DistillNestedApp::GetOccupied () const
{
  return m_occupied;
}

void
DistillNestedApp::Occupy (Time time)
{
  m_occupied += time;
}

void
DistillNestedApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
DistillNestedApp::StartApplication ()
{
  NS_LOG_LOGIC ("Starting DistillNestedApp of "
                << m_pnode << " with address " << m_pnode->GetAddress () << " and port " << m_port);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);
  m_recv_socket->SetRecvCallback (MakeCallback (&DistillNestedApp::HandleRead, this));

  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  if (!m_checker)
    { // Alice
      std::vector<std::string> src_qubits = {};
      std::vector<std::string> dst_qubits = {};
      assert (m_src_qubits->GetSize () == m_dst_qubits->GetSize ());
      for (unsigned i = 0; i < m_src_qubits->GetSize (); ++i)
        {
          src_qubits.push_back (m_src_qubits->GetQubit (i));
          dst_qubits.push_back (m_dst_qubits->GetQubit (i));
        }
      Distillate (src_qubits, dst_qubits);
    }
  else
    { // Bob
    }
}

} // namespace ns3