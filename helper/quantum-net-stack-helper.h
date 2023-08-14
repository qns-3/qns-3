#ifndef QUANTUM_NET_STACK_HELPER_H
#define QUANTUM_NET_STACK_HELPER_H

#include "ns3/node-container.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumNode;

class QuantumNetStackHelper
{
public:
  QuantumNetStackHelper (void);

  virtual ~QuantumNetStackHelper (void);

  void Install (Ptr<QuantumNode> alice, Ptr<QuantumNode> bob) const;

  void Install (NodeContainer c) const;

private:
  Ptr<QuantumPhyEntity> m_qphyent;
};

} // namespace ns3

#endif /* QUANTUM_NET_STACK_HELPER_H */
