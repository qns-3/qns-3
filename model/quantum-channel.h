#ifndef QUANTUM_CHANNEL_H
#define QUANTUM_CHANNEL_H

#include "ns3/object.h"

namespace ns3 {

class QuantumNode;
class QuantumPhyEntity;

class QuantumChannel : public Object
{

private:

  /** Names of the source and the destination nodes. */
  std::string m_src_owner, m_dst_owner;

public:

  QuantumChannel (const std::pair<std::string, std::string> &conn);
  QuantumChannel (std::string src_owner_, std::string dst_owner_);
  QuantumChannel (const std::pair<Ptr<QuantumNode>, Ptr<QuantumNode>> &conn);
  QuantumChannel (Ptr<QuantumNode> src, Ptr<QuantumNode> dst);

  ~QuantumChannel ();

  QuantumChannel ();
  static TypeId GetTypeId ();

  Ptr<QuantumNode> GetSrc (Ptr<QuantumPhyEntity> qphyent) const;
  Ptr<QuantumNode> GetDst (Ptr<QuantumPhyEntity> qphyent) const;

  std::string GetSrcOwner () const;
  std::string GetDstOwner () const;

  /** 
   * \brief Set the depolarization model for the channel.
   * \param fidel Outcoming fidelity of the EPR pairs distributed through the channel.
   * \param qphyent Quantum physical entity to record the model.
   * 
  */
  void SetDepolarModel (double fidel, Ptr<QuantumPhyEntity> qphyent);

  bool operator<(const QuantumChannel &other) const;

};

} // namespace ns3

#endif /* QUANTUM_CHANNEL_H */