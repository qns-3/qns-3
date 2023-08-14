#include "ns3/quantum-node.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity functions
#include "ns3/quantum-error-model.h" // class QuantumErrorModel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumNode");

NS_OBJECT_ENSURE_REGISTERED (QuantumNode);

QuantumNode::QuantumNode (Ptr<QuantumPhyEntity> qphyent_, std::string owner_)
    : Node (),
      m_qphyent (qphyent_),
      m_owner (owner_),
      m_address (Address ()),
      m_next_port (9),
      m_rank (UINT_MAX),
      m_qmemory (QuantumMemory ())
{
}

QuantumNode::~QuantumNode ()
{
}

QuantumNode::QuantumNode ()
    : Node (),
      m_qphyent (nullptr),
      m_owner (""),
      m_address (Address ()),
      m_next_port (9),
      m_rank (UINT_MAX),
      m_qmemory (QuantumMemory ())
{
}

void
QuantumNode::DoDispose (void)
{
  Node::DoDispose ();
}

TypeId
QuantumNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QuantumNode").SetParent<Node> ().AddConstructor<QuantumNode> ();
  return tid;
}

void
QuantumNode::SetAddress (Address addr)
{
  NS_LOG_LOGIC ("Setting m_address of " << m_owner << " to " << addr);
  m_address = addr;
}

Address
QuantumNode::GetAddress () const
{
  return m_address;
}

uint16_t
QuantumNode::AllocPort ()
{
  return m_next_port++;
}

uint16_t
QuantumNode::GetNextPort () const
{
  return m_next_port;
}

Ptr<QuantumPhyEntity>
QuantumNode::GetQuantumPhyEntity () const
{
  return m_qphyent;
}

void
QuantumNode::SetRank (unsigned r)
{
  m_rank = r;
}

unsigned
QuantumNode::GetRank () const
{
  return m_rank;
}

void
QuantumNode::AddQubit (const std::string &name)
{
  NS_LOG_INFO (YELLOW_CODE << "Adding qubit " << name << " to " << m_owner << " with local idx "
                           << m_qmemory.GetSize () << END_CODE);
  m_qmemory.AddQubit (name.c_str ());
}

bool
QuantumNode::RemoveQubit (const std::string &name)
{
  NS_LOG_INFO (RED_CODE << "Removing qubit " << name << " from " << m_owner << END_CODE);
  return m_qmemory.RemoveQubit (name.c_str ());
}

std::string
QuantumNode::GetQubit (unsigned local) const
{
  return m_qmemory.GetQubit (local);
}

bool
QuantumNode::OwnQubit (const std::string &name) const
{
  return m_qmemory.ContainQubit (name.c_str ());
}

void
QuantumNode::SetErrorModel (Ptr<QuantumErrorModel> pmodel, const std::string &qubit)
{
  m_qphyent->SetErrorModel (pmodel, qubit);
}

void
QuantumNode::SetDephaseModel (const std::string &gate,
                              double rate // dephasing rate of the gate of the node
)
{
  m_qphyent->SetDephaseModel (m_owner, gate, rate);
}

void
QuantumNode::SetTimeModel (double rate) // dephasing rate of the time model
{
  m_qphyent->SetTimeModel (m_owner, rate);
}

const std::string &
QuantumNode::GetOwner () const
{
  return m_owner;
}

bool
QuantumNode::operator<(const QuantumNode &another) const
{
  return m_rank < another.m_rank;
}

void
QuantumNode::PrintIt () const
{
  NS_LOG_LOGIC ("m_owner  : " << m_owner << "\naddress: " << m_address
                              << "\nnapps  : " << GetNApplications ());
}

std::ostream &
operator<< (std::ostream &out, const Ptr<QuantumNode> &pnode)
{
  out << pnode->GetOwner ();
  return out;
}

} // namespace ns3