#include "ns3/quantum-phy-entity.h"

#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer
#include "ns3/csma-module.h" // class CsmaHelper
#include "ns3/application.h" // class Application

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-operation.h" // class QuantumOperation
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-error-model.h" // class QuantumErrorModel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumPhyEntity");

QuantumPhyEntity::QuantumPhyEntity (const std::vector<std::string> &owners)
    : m_qnetsim (QuantumNetworkSimulator (owners)), // TODO: CreateObject, release

      m_conn2apps ({}),

      m_qubit2time (std::map<std::string, Time> ()),
      m_gate2model ({}),
      m_conn2model ({}),
      m_node2model ({})

{
  /* util */

  // QuantumNode
  for (const std::string &owner : owners)
    {
      Ptr<QuantumNode> pnode = CreateObject<QuantumNode> (this, owner);
      m_owner2pnode[owner] = pnode;
      m_gate2model[pnode] = {};
    }
}

QuantumPhyEntity::~QuantumPhyEntity ()
{
}

QuantumPhyEntity::QuantumPhyEntity ()
    : m_qnetsim (QuantumNetworkSimulator ()),
      m_conn2apps ({}),

      m_qubit2time ({}),
      m_gate2model ({}),
      m_conn2model ({}),
      m_node2model ({})
{
}

void
QuantumPhyEntity::DoDispose (void)
{
  Object::DoDispose ();
}

TypeId
QuantumPhyEntity::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QuantumPhyEntity").SetParent<Object> ();
  return tid;
}

/* circuit */

bool
QuantumPhyEntity::GenerateQubitsPure (
    const std::string &owner, // owner generating the qubits
    const std::vector<std::complex<double>> &data, // state vector of the qubits
    const std::vector<std::string> &qubits // names of the qubits
)
{
  Time moment = Simulator::Now ();
  if (owner != "God" && CheckOwned (owner, qubits))
    {
      return false;
    }

  bool succeed = m_qnetsim.GenerateQubitsPure (owner, data, qubits);

  Ptr<QuantumNode> pnode = m_owner2pnode[owner];

  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      std::string qubit = qubits[i];

      pnode->AddQubit (qubit);

      Ptr<QuantumErrorModel> pmodel;
      if (m_node2model.find (pnode) !=
          m_node2model.end ()) // the owner's qubit generator has been specified
        pmodel = m_node2model[pnode];
      else
        pmodel = default_time_model.GetObject<QuantumErrorModel> ();
      SetErrorModel (pmodel, qubit);

      m_qubit2time[qubit] = moment;
    }

  return succeed;
}

bool
QuantumPhyEntity::GenerateQubitsMixed (
    const std::string &owner,
    const std::vector<std::complex<double>> &data,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  if (owner != "God" && CheckOwned (owner, qubits))
    {
      return false;
    }

  bool succeed = m_qnetsim.GenerateQubitsMixed (owner, data, qubits);

  Ptr<QuantumNode> pnode = m_owner2pnode[owner];

  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      std::string qubit = qubits[i];

      pnode->AddQubit (qubit);

      Ptr<QuantumErrorModel> pmodel;
      if (m_node2model.find (pnode) !=
          m_node2model.end ()) // the owner's qubit generator has been specified
        pmodel = m_node2model[pnode];
      else
        pmodel = default_time_model.GetObject<QuantumErrorModel> ();
      SetErrorModel (pmodel, qubit);

      m_qubit2time[qubit] = moment;
    }

  return succeed;
}

bool
QuantumPhyEntity::ApplyGate (
    const std::string &owner,
    const std::string &gate,
    const std::vector<std::complex<double>> &data,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  assert (CheckOwned (owner, qubits));

  for (const std::string &qubit : qubits)
    {
      NS_LOG_LOGIC ("Updating qubit " << qubit);
      ApplyErrorModel ({qubit}, moment);
    }

  bool succeed = m_qnetsim.ApplyGate (owner, gate, data, qubits);

  if (owner != "God")
    ApplyErrorModel (owner, gate, qubits, moment);

  return succeed;
}

bool
QuantumPhyEntity::ApplyOperation (
    const QuantumOperation &quantumOperation,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  bool succeed = m_qnetsim.ApplyOperation (quantumOperation, qubits);

  // update qubit2time only if the moment param has an actual meaning
  if (moment != Seconds (-1))
    {
      for (const std::string &qubit : qubits)
        {
          m_qubit2time[qubit] = moment;
        }
    }
  return succeed;
}

bool
QuantumPhyEntity::ApplyControlledOperation (
    const std::string &orig_owner,
    const std::string &orig_gate,
    const std::string &gate,
    const std::vector<std::complex<double>> &data, const std::vector<std::string> &control_qubits,
    const std::vector<std::string> &target_qubits)
{
  bool succeed = m_qnetsim.ApplyControlledOperation (orig_owner, orig_gate, gate, data,
                                                    control_qubits, target_qubits);

  ApplyErrorModel (orig_owner, orig_gate, target_qubits);

  return succeed;
}

std::pair<unsigned, std::vector<double>>
QuantumPhyEntity::Measure (const std::string &owner, const std::vector<std::string> &qubits)
{
  Time moment = Simulator::Now ();
  assert (CheckOwned (owner, qubits));
  for (const std::string &q : m_qnetsim.m_qubits_all)
    {
      if (!CheckValid ({q}))
        {
          continue;
        }
      ApplyErrorModel ({q}, moment);
    }

  return m_qnetsim.Measure (owner, qubits);
}

std::vector<std::complex<double>>
QuantumPhyEntity::PeekDM (
    const std::string &owner,
    const std::vector<std::string> &qubits,
    std::vector<std::complex<double>> &dm
)
{
  if (owner != "God")
    {
      assert (CheckOwned (owner, qubits));
    }

  return m_qnetsim.PeekDM (owner, qubits, dm);
}

bool
QuantumPhyEntity::PartialTrace (
  const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  for (const std::string &qubit : qubits)
    {
      ApplyErrorModel ({qubit}, moment);
    }

  return m_qnetsim.PartialTrace (qubits);
}

std::vector<std::complex<double>>
QuantumPhyEntity::Contract (const std::string &optimizer)
{
  return m_qnetsim.Contract (optimizer);
}

Ptr<QuantumNode>
QuantumPhyEntity::GetNode (const std::string &owner)
{
  return m_owner2pnode[owner];
}

void
QuantumPhyEntity::SetErrorModel (Ptr<QuantumErrorModel> pmodel, const std::string &qubit)
{
  NS_LOG_LOGIC ("Setting error model " << pmodel->GetTypeDesc () << " to qubit " << qubit);
  m_qubit2model[qubit] = pmodel;
}

void
QuantumPhyEntity::SetDephaseModel (const std::string &owner,
                                   const std::string &gate,
                                   double rate
)
{
  Ptr<QuantumErrorModel> pmodel = CreateObject<DephaseModel> (rate);
  NS_LOG_LOGIC ("Setting dephase model " << pmodel->GetTypeDesc () << " to gate " << gate
                                         << " of node " << owner);
  Ptr<QuantumNode> pnode = m_owner2pnode[owner];

  m_gate2model[pnode][gate] = pmodel;
}

void
QuantumPhyEntity::ApplyErrorModel ( // using DephaseModel
    const std::string &owner, const std::string &gate, const std::vector<std::string> &qubits,
    const Time &moment)
{
  Ptr<QuantumNode> pnode = m_owner2pnode[owner];

  for (const std::string &qubit : qubits)
    { // default
      if (m_gate2model[pnode].find (gate) == m_gate2model[pnode].end ())
        {
          NS_LOG_LOGIC ("Using default DephaseModel");
          default_dephase_model.ApplyErrorModel (this, {qubit},
                                                 Seconds (GATE_DURATION + moment.GetSeconds ()));
        }
      else
        { // specified
          m_gate2model[pnode][gate]->ApplyErrorModel (this, {qubit},
                                                      Seconds (GATE_DURATION + moment.GetSeconds ()));
        }
    }
}
void
QuantumPhyEntity::SetDepolarModel (std::pair<std::string, std::string> conn, double fidel)
{
  Ptr<QuantumErrorModel> pmodel = CreateObject<DepolarModel> (fidel);
  NS_LOG_LOGIC ("Setting depolar model " << pmodel->GetTypeDesc () << " to connection " << conn.first
                                         << " <--> " << conn.second);
  m_conn2model[conn] = pmodel;
}
void
QuantumPhyEntity::ApplyErrorModel ( // using DepolarModel
    std::pair<std::string, std::string> conn, const std::pair<std::string, std::string> &epr)
{
  NS_LOG_LOGIC ("Applying depolar error to EPR pair consisting of "
                << epr.first << " and " << epr.second << " using DepolarModel between "
                << conn.first << " and " << conn.second
                << " at time " << Simulator::Now ());

  for (auto [conn, pmodel] : m_conn2model)
    {
      if (conn.first == "" && conn.second == "")
        {
          NS_LOG_LOGIC ("Skipping default DepolarModel");
          continue;
        }
      // NS_LOG_LOGIC ("Connection " << conn.first << " <--> " << conn.second << " has "
      //                             << pmodel->GetTypeDesc ());
    }
  if (m_conn2model.find (conn) == m_conn2model.end ())
    { // default
      NS_LOG_LOGIC ("Using default DepolarModel");
      default_depolar_model.ApplyErrorModel (this, {epr.second}, Seconds (-1));
    }
  else
    { // specified
      NS_LOG_LOGIC ("Using specified DepolarModel");
      m_conn2model[conn]->ApplyErrorModel (this, {epr.second}, Seconds (-1));
    }
}

void
QuantumPhyEntity::SetTimeModel (const std::string &owner, double rate)
{
  Ptr<QuantumErrorModel> pmodel = CreateObject<TimeModel> (rate);
  NS_LOG_LOGIC ("Setting time model " << pmodel->GetTypeDesc () << " to node " << owner);
  Ptr<QuantumNode> pnode = m_owner2pnode[owner];

  m_node2model[pnode] = pmodel;
}
void
QuantumPhyEntity::ApplyErrorModel ( // using TimeModel
    const std::vector<std::string> &qubits, const Time &moment)
{
  for (const std::string &qubit : qubits)
    {
      assert (m_qubit2model.find (qubit) != m_qubit2model.end ());
      m_qubit2model[qubit]->ApplyErrorModel (this, qubits, moment);
    }
}




/* network */

void
QuantumPhyEntity::GenerateEPR (Ptr<QuantumChannel> qconn,
                               const std::pair<std::string, std::string> &epr)
{
  NS_LOG_LOGIC ("Generating EPR pair consisting of " << epr.first << " and " << epr.second);
  GenerateQubitsPure (qconn->GetSrcOwner (), q_bell, {epr.first, epr.second});
}

double 
QuantumPhyEntity::CalculateFidelity (const std::pair<std::string, std::string> &epr, double &fidel)
{
  return m_qnetsim.CalculateFidelity (epr, fidel);
}


/* util */

void
QuantumPhyEntity::Evaluate ()
{
  m_qnetsim.Evaluate (nullptr);
}

bool
QuantumPhyEntity::CheckValid (const std::vector<std::string> &qubits) const
{
  return m_qnetsim.CheckValid (qubits);
}

bool
QuantumPhyEntity::CheckOwned (const std::string &owner,
                              const std::vector<std::string> &qubits) const
{
  if (owner == "God") // God
    {
      return true;
    }

  Ptr<QuantumNode> pnode = m_owner2pnode.at (owner);

  for (const std::string &qubit : qubits) // check if owned
    {
      if (!pnode->OwnQubit (qubit))
        {
          NS_LOG_LOGIC ("At time " << Simulator::Now () << " " << owner << " skips on qubit named "
                        << qubit << " owned by others, if not about to generate it.");
          return false;
        }
    }

  return true;
}

void
QuantumPhyEntity::AddConn2Apps (Ptr<QuantumChannel> qconn, const std::string &protocol,
                                const std::pair<Ptr<Application>, Ptr<Application>> &apps)
{
  m_conn2apps[*qconn][protocol] = apps;
}

std::pair<Ptr<Application>, Ptr<Application>>
QuantumPhyEntity::GetConn2Apps (Ptr<QuantumChannel> qconn, const std::string &protocol)
{
  NS_LOG_LOGIC ("Getting " << protocol << " apps for " << qconn->GetSrcOwner () << " <--> "
                           << qconn->GetDstOwner ());
  return m_conn2apps[*qconn][protocol];
}

void
QuantumPhyEntity::SetOwnerAddress (const std::string &owner, const Address &address)
{
  m_owner2pnode[owner]->SetAddress (address);
}

void
QuantumPhyEntity::SetOwnerRank (const std::string &owner, const unsigned &rank)
{
  m_owner2pnode[owner]->SetRank (rank);
}


void
QuantumPhyEntity::Checkpoint ()
{
  m_qnetsim.Checkpoint ();
}


/* debug */

void
QuantumPhyEntity::PrintConn2Apps () const
{
  std::cout << "-- m_conn2apps --\n";
  for (const auto &[conn, apps] : m_conn2apps)
    {
      std::cout << conn.GetSrcOwner () << " <--> " << conn.GetDstOwner () << "\n";
      for (const auto &[name, app_pair] : apps)
        {
          std::cout << name << ": " << app_pair.first->GetTypeId ().GetName () << ", "
                    << app_pair.second->GetTypeId ().GetName () << "\n";
        }
    }
}

} // namespace ns3