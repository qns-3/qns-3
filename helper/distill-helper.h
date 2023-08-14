#ifndef DISTILL_HELPER_H
#define DISTILL_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumChannel;

class DistillHelper
{
public:
  DistillHelper (Ptr<QuantumPhyEntity> qphyent, bool checker, Ptr<QuantumChannel> conn);

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (Ptr<Node> node) const;

  ApplicationContainer Install (std::string nodeName) const;

  ApplicationContainer Install (NodeContainer c) const;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* DISTILL_HELPER_H */
