#include "ns3/distill-nested-adapt-app.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-memory.h" // class QuantumMemory
#include "ns3/distribute-epr-protocol.h" // class DistributeEPRSrcProtocol


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistillNestedAdaptApp");

NS_OBJECT_ENSURE_REGISTERED (DistillNestedAdaptApp);

DistillNestedAdaptApp::DistillNestedAdaptApp (Ptr<QuantumPhyEntity> qphyent_, bool checker_,
                                              Ptr<QuantumChannel> conn_,
                                              const std::pair<std::string, std::string> &qubits_)
    : m_qphyent (qphyent_),
      m_checker (checker_),
      m_conn (conn_),
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

DistillNestedAdaptApp::~DistillNestedAdaptApp ()
{
  if (m_data)
    delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

DistillNestedAdaptApp::DistillNestedAdaptApp ()
    : m_qphyent (nullptr),
      m_checker (false),
      m_pnode (nullptr),
      m_src_qubits (nullptr),
      m_dst_qubits (nullptr),
      m_occupied (Seconds (0)),

      m_port (9),
      m_peerPort (9),
      m_size (0),
      m_data (0),
      m_dataSize (0)
{
}

TypeId
DistillNestedAdaptApp::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::DistillNestedAdaptApp")
          .SetParent<Application> ()
          .AddConstructor<DistillNestedAdaptApp> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.", UintegerValue (9),
                         MakeUintegerAccessor (&DistillNestedAdaptApp::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute (
              "PeerAddress", "The destination Address of the outbound packets", AddressValue (),
              MakeAddressAccessor (&DistillNestedAdaptApp::m_peerAddress), MakeAddressChecker ())
          .AddAttribute ("PeerPort", "The destination port of the outbound packets",
                         UintegerValue (9),
                         MakeUintegerAccessor (&DistillNestedAdaptApp::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("DataSize", "The amount of data to send in bytes", UintegerValue (0),
                         MakeUintegerAccessor (&DistillNestedAdaptApp::m_dataSize),
                         MakeUintegerChecker<uint32_t> ())

          .AddAttribute ("QPhyEntity", "The pointer to the quantum physical entity", PointerValue (),
                         MakePointerAccessor (&DistillNestedAdaptApp::m_qphyent),
                         MakePointerChecker<QuantumPhyEntity> ())
          .AddAttribute (
              "Checker", "Whether the owner is the checker (Bob) in the distillation", BooleanValue (false),
              MakeBooleanAccessor (&DistillNestedAdaptApp::m_checker), MakeBooleanChecker ())
          .AddAttribute ("PNode", "The pointer to the quantum node owning this app", PointerValue (),
                         MakePointerAccessor (&DistillNestedAdaptApp::m_pnode),
                         MakePointerChecker<QuantumNode> ())
          .AddAttribute ("QChannel", "The pointer to the quantum connection", PointerValue (),
                         MakePointerAccessor (&DistillNestedAdaptApp::m_conn),
                         MakePointerChecker<QuantumChannel> ())
          .AddAttribute ("SrcQubits", "The pointer to the qubits of the source owner",
                         PointerValue (),
                         MakePointerAccessor (&DistillNestedAdaptApp::m_src_qubits),
                         MakePointerChecker<QuantumMemory> ())
          .AddAttribute ("DstQubits", "The pointer to the qubits of the destination owner",
                         PointerValue (),
                         MakePointerAccessor (&DistillNestedAdaptApp::m_dst_qubits),
                         MakePointerChecker<QuantumMemory> ())
          .AddAttribute (
              "FlagQubit", "The name of the flag qubit of the distillation", StringValue (""),
              MakeStringAccessor (&DistillNestedAdaptApp::m_flag_qubit), MakeStringChecker ());
  return tid;
}

void
DistillNestedAdaptApp::Distillate (
    const std::vector<std::string> &src_qubits,
    const std::vector<std::string>
        &dst_qubits)
{
  NS_LOG_LOGIC (PURPLE_CODE << "Scheduling a distillation to get EPR pair " << src_qubits[0]
                            << " " << dst_qubits[0] << END_CODE);
  if (!m_checker)
    { // Alice
      assert (src_qubits.size () == dst_qubits.size ());
      unsigned pairs = src_qubits.size ();

      if (pairs > 2) // recursively schedule to distillate prefix half and the suffix half
        {
          NS_LOG_LOGIC ("pairs = " << pairs << " > 2");

          Distillate (GetPreHalf (src_qubits), GetPreHalf (dst_qubits));

          Distillate (GetSufHalf (src_qubits), GetSufHalf (dst_qubits));
        }
      else // generate and distillate the two EPR pairs
        {
          assert (pairs == 2);
          NS_LOG_LOGIC ("pairs = " << pairs);
        }

      NS_LOG_LOGIC ("Scheduling a DistillateOnce at occupied time = "
                    << GetOccupied ().As (Time::S) << " s to get EPR pair " << src_qubits[0] << " "
                    << dst_qubits[0] << " at the cost of EPR pair " << src_qubits[pairs / 2] << " "
                    << dst_qubits[pairs / 2]);

      Simulator::Schedule (GetOccupied (), &DistillNestedAdaptApp::DistillateOnce, this, src_qubits,
                           dst_qubits); // distillate the two EPR pairs to get one
      // Occupy (Seconds (DIST_EPR_DELAY));
    }
  else
    { // Bob
    }
}

void
DistillNestedAdaptApp::DistillateOnce (
    const std::vector<std::string> &src_qubits,
    const std::vector<std::string>
        &dst_qubits)
{
  NS_LOG_INFO (PURPLE_CODE << "Distillating once to get " << src_qubits[0] << " " << dst_qubits[0]
                           << " at time " << GetOccupied ().As (Time::S) << " s"
                           << " at the cost of " << src_qubits[src_qubits.size () / 2] << " "
                           << dst_qubits[dst_qubits.size () / 2] << END_CODE);

  Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::Checkpoint, m_qphyent);
  
  if (!m_checker)
    { // Alice

      std::pair<std::string, std::string> epr_goal = {src_qubits[0], dst_qubits[0]};
      NS_LOG_LOGIC ("Alice's goal EPR pair is " << epr_goal.first << " " << epr_goal.second);
      std::pair<std::string, std::string> epr_meas = {src_qubits[src_qubits.size () / 2],
                                                      dst_qubits[dst_qubits.size () / 2]};
      NS_LOG_LOGIC ("Alice's meas EPR pair is " << epr_meas.first << " " << epr_meas.second);

      if (src_qubits.size () == 2)
        {
          Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
              m_qphyent->GetConn2Apps (m_conn, APP_DIST_EPR)
                  .first->GetObject<DistributeEPRSrcProtocol> ();
          NS_LOG_LOGIC ("Scheduling a GenerateAndDistributeEPR at occupied time = "
                        << GetOccupied ().As (Time::S) << " s to dist EPR pair " << epr_goal.first
                        << " " << epr_goal.second);
          Simulator::Schedule (GetOccupied (), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                               dist_epr_src_app, epr_goal);
          Occupy (Seconds (DIST_EPR_DELAY));

          NS_LOG_LOGIC ("Scheduling a GenerateAndDistributeEPR at occupied time = "
                        << GetOccupied ().As (Time::S) << " s to dist EPR pair " << epr_meas.first
                        << " " << epr_meas.second);
          Simulator::Schedule (GetOccupied (), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                               dist_epr_src_app, epr_meas);
          Occupy (Seconds (DIST_EPR_DELAY));
        }

      std::vector<std::complex<double>> unused;
        
      // Alice applies CNOT
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent,
                           m_pnode->GetOwner (), QNS_GATE_PREFIX + "CNOT", cnot,
                           std::vector<std::string>{epr_meas.first, epr_goal.first});

      // Bob applies CNOT
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent,
                           m_conn->GetDstOwner (), QNS_GATE_PREFIX + "CNOT", cnot,
                           std::vector<std::string>{epr_meas.second, epr_goal.second});

      // parity check, store in Bob's meas qubit (0 wanted)
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                           QNS_GATE_PREFIX + "CNOT", cnot,
                           std::vector<std::string>{epr_meas.second, epr_meas.first});
      // Alice's meas qubit is not used anymore
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::PartialTrace, m_qphyent,
                           std::vector<std::string>{epr_meas.first});
      
      // negate the parity (1 wanted)
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                           QNS_GATE_PREFIX + "PX", pauli_X,
                           std::vector<std::string>{epr_meas.second});

      // create a ancilla qubit to store the result && flag
      std::string anc = AllocAncillaQubit ();
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                           m_pnode->GetOwner (), q_ket_0, std::vector<std::string>{anc});
      // && flag, store in ancilla (1 wanted)
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                           QNS_GATE_PREFIX + "TOFF", toffoli,
                           std::vector<std::string>{anc, epr_meas.second, m_flag_qubit});
      // Bob's meas qubit is not used anymore
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::PartialTrace, m_qphyent,
                           std::vector<std::string>{epr_meas.second}); 
      
      // swap, store the result in flag (1 wanted)
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::ApplyGate, m_qphyent, "God",
                           QNS_GATE_PREFIX + "SWAP", swap,
                           std::vector<std::string>{anc, m_flag_qubit});
      // ancilla qubit is not used anymore
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::PartialTrace, m_qphyent,
                           std::vector<std::string>{anc});

      NS_LOG_LOGIC ("Scheduling a CNOT + PX + TOFF + SWAP + Trace at occupied time = "
                    << GetOccupied ().As (Time::S) << " s to get EPR pair " << epr_goal.first << " "
                    << epr_goal.second << " at the cost of EPR pair " << epr_meas.first << " "
                    << epr_meas.second << " and ancilla qubit " << anc);
      
      if (src_qubits.size () == m_src_qubits->GetSize ()) { // the last distillation
        Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::Contract, m_qphyent, "distill");
        // peek the goal epr
        Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::PeekDM, m_qphyent, 
                           "God", std::vector<std::string>{epr_goal.first, epr_goal.second}, unused);
        // measure the flag qubit
        Simulator::Schedule (GetOccupied (), &DistillNestedAdaptApp::GetWin, this);
        // peek the goal epr again (could be better or worse)
        Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::PeekDM, m_qphyent, 
                           "God", std::vector<std::string>{epr_goal.first, epr_goal.second}, unused);
        double fidel;
        Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::CalculateFidelity, m_qphyent,
                            epr_goal, fidel);
      }
    }
  else
    { // Bob
    }
}

void
DistillNestedAdaptApp::HandleRead (Ptr<Socket> socket)
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

      NS_LOG_INFO (GREEN_CODE << "At time " << GetOccupied ().As (Time::S) << " Node # "
                              << socket->GetNode ()->GetId () << " received \"" << data
                              << "\" from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                              << END_CODE);
    }
}


void
DistillNestedAdaptApp::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
DistillNestedAdaptApp::SetFill (std::string fill)
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
DistillNestedAdaptApp::SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port)
{
  m_send_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

void
DistillNestedAdaptApp::Send ()
{
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "DistillNestedAdaptApp::DistillateOnce(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "DistillNestedAdaptApp::DistillateOnce(): m_dataSize but no m_data");

      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  SendPacket (p, Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort);
  NS_LOG_INFO (CYAN_CODE << "At time " << GetOccupied ().As (Time::S) << " Node # "
                         << m_send_socket->GetNode ()->GetId () << " sent \"" << m_data << "\" to "
                         << Ipv6Address::ConvertFrom (m_peerAddress) << END_CODE);
}


void
DistillNestedAdaptApp::SetSrcQubits (Ptr<QuantumMemory> src_qubits)
{
  NS_LOG_LOGIC ("Occupied time = " << GetOccupied ().As (Time::S) << " s");
  NS_LOG_LOGIC ("Setting src_qubits to ");
  for (unsigned i = 0; i < src_qubits->GetSize (); ++i)
    NS_LOG_LOGIC (src_qubits->GetQubit (i) << " ");

  m_src_qubits = src_qubits;
}

void
DistillNestedAdaptApp::SetDstQubits (Ptr<QuantumMemory> dst_qubits)
{
  NS_LOG_LOGIC ("Setting dst_qubits to ");
  for (unsigned i = 0; i < dst_qubits->GetSize (); ++i)
    NS_LOG_LOGIC (dst_qubits->GetQubit (i) << " ");
  m_dst_qubits = dst_qubits;
}

bool
DistillNestedAdaptApp::GetWin () const
{
  std::pair<unsigned, std::vector<double>> outcome =
      m_qphyent->Measure (m_pnode->GetOwner (), std::vector<std::string>{m_flag_qubit});
  bool wins = (outcome.first == 1);
  NS_LOG_INFO (GREEN_CODE << "At time " << Simulator::Now ().As (Time::S) << " Alice finds out "
                          << (wins ? "wins" : "loses") << END_CODE);
  NS_LOG_INFO (CYAN_CODE << "Probability of succeeding is " << outcome.second[1] << END_CODE);

  return wins;
}

Time
DistillNestedAdaptApp::GetOccupied () const
{
  return m_occupied;
}

void
DistillNestedAdaptApp::Occupy (Time time)
{
  m_occupied += time;
}

void
DistillNestedAdaptApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
DistillNestedAdaptApp::StartApplication ()
{
  NS_LOG_LOGIC ("Starting DistillNestedAdaptApp of "
                << m_pnode << " with address " << m_pnode->GetAddress () << " and port " << m_port);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_recv_socket, m_port);
  m_recv_socket->SetRecvCallback (MakeCallback (&DistillNestedAdaptApp::HandleRead, this));

  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  if (!m_checker)
    { // Alice
      Simulator::Schedule (GetOccupied (), &QuantumPhyEntity::GenerateQubitsPure, m_qphyent,
                           m_pnode->GetOwner (), q_ket_1, std::vector<std::string>{m_flag_qubit});
      std::vector<std::string> src_qubits = {};
      std::vector<std::string> dst_qubits = {};
      assert (m_src_qubits->GetSize () == m_dst_qubits->GetSize ());
      for (unsigned i = 0; i < m_src_qubits->GetSize (); ++i)
        {
          src_qubits.push_back (m_src_qubits->GetQubit (i));
          dst_qubits.push_back (m_dst_qubits->GetQubit (i));
        }

      Simulator::Schedule (GetOccupied (), &DistillNestedAdaptApp::Distillate, this, src_qubits,
                           dst_qubits);
    }
  else
    { // Bob
    }
}

} // namespace ns3