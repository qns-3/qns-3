// Note that Alice's app must be created before Bob's app

#ifndef ENT_SWAP_ADAPT_APP_H
#define ENT_SWAP_ADAPT_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"

#include <complex>

namespace ns3 {

class QuantumPhyEntity;
class QuantumMemory;

/**
 * \brief Entanglement swapping application with (less gratifying an) adaptation.
 * 
 * This application is used to schedule a chained entanglement swapping
 * with long distance operations.
 * A longer EPR pair is gained at the cost of several shorter (more local) EPR pairs.
 * 
 * 1. Every node generates and distributes a EPR pair to its neighbor with the help of DistributeEPRApps.
 * 2. God pass control information from every node to the last.
 * 3. God apply controlled correction gates.
*/
class EntSwapAdaptApp : public Application
{
public:
  EntSwapAdaptApp (Ptr<QuantumPhyEntity> qphyent_,
                   Ptr<QuantumMemory> qubits_former_,
                   Ptr<QuantumMemory> qubits_latter_);
  virtual ~EntSwapAdaptApp ();

  EntSwapAdaptApp ();
  static TypeId GetTypeId ();

  /**
   * \brief Schedule all events in the chained entanglement swapping.
  */
  void EntanglementSwapping ();

private:
  virtual void StartApplication ();

  Ptr<QuantumPhyEntity> m_qphyent; /**< The quantum physical entity. */
  Ptr<QuantumMemory> m_qubits_former; /**< The former qubit of this owner. */
  Ptr<QuantumMemory> m_qubits_latter; /**< The latter qubit of this owner. */
};

} // namespace ns3

#endif /* ENT_SWAP_ADAPT_APP_H */
