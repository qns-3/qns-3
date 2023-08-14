#include "ns3/quantum-channel.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumChannel");

QuantumChannel::QuantumChannel (const std::pair<std::string, std::string> &conn)
    : m_src_owner (conn.first), m_dst_owner (conn.second)
{
}

QuantumChannel::QuantumChannel (std::string src_owner_, std::string dst_owner_)
    : m_src_owner (src_owner_), m_dst_owner (dst_owner_)
{
}

QuantumChannel::QuantumChannel (const std::pair<Ptr<QuantumNode>, Ptr<QuantumNode>> &conn)
    : m_src_owner (conn.first->GetOwner ()), m_dst_owner (conn.second->GetOwner ())
{
}

QuantumChannel::QuantumChannel (Ptr<QuantumNode> src, Ptr<QuantumNode> dst)
    : m_src_owner (src->GetOwner ()), m_dst_owner (dst->GetOwner ())
{
}

QuantumChannel::~QuantumChannel ()
{
}

QuantumChannel::QuantumChannel () : m_src_owner (""), m_dst_owner ("")
{
}

TypeId
QuantumChannel::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::QuantumChannel").SetParent<Object> ().AddConstructor<QuantumChannel> ();
  return tid;
}

Ptr<QuantumNode>
QuantumChannel::GetSrc (Ptr<QuantumPhyEntity> qphyent) const
{
  return qphyent->GetNode (m_src_owner);
}

Ptr<QuantumNode>
QuantumChannel::GetDst (Ptr<QuantumPhyEntity> qphyent) const
{
  return qphyent->GetNode (m_dst_owner);
}

std::string
QuantumChannel::GetSrcOwner () const
{
  assert (m_src_owner != "");
  return m_src_owner;
}

std::string
QuantumChannel::GetDstOwner () const
{
  assert (m_dst_owner != "");
  return m_dst_owner;
}

void
QuantumChannel::SetDepolarModel (double fidel, Ptr<QuantumPhyEntity> qphyent)
{
  qphyent->SetDepolarModel ({m_src_owner, m_dst_owner}, fidel);
}

bool
QuantumChannel::operator<(const QuantumChannel &other) const
{
  return m_src_owner < other.m_src_owner ||
         (m_src_owner == other.m_src_owner && m_dst_owner < other.m_dst_owner);
}

} // namespace ns3