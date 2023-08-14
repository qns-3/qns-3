// Note that Alice's app must be created before Bob's app

#ifndef ENT_SWAP_ADAPT_LOCAL_HELPER_H
#define ENT_SWAP_ADAPT_LOCAL_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumChannel;
class QuantumNode;

class EntSwapAdaptLocalHelper
{
public:
  EntSwapAdaptLocalHelper (Ptr<QuantumPhyEntity> qphyent);

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (Ptr<Node> node) const;

  ApplicationContainer Install (std::string nodeName) const;

  ApplicationContainer Install (NodeContainer c) const;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* ENT_SWAP_ADAPT_LOCAL_HELPER_H */
