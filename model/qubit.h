#ifndef QUBIT_H
#define QUBIT_H

#include "ns3/object.h"

#include "ns3/quantum-basis.h"

#include <complex>

namespace ns3 {

/**
 * \brief Qubit in state vector representation.
 * 
 * \note This class is only used for attribute passing when setting up quantum applications.
*/
class Qubit : public Object
{

private:

  /** State vector of the qubit. */
  std::vector<std::complex<double>> m_state_vector;
  /** Name of the qubit. */
  std::string m_qubit;

public:

  Qubit (
    const std::vector<std::complex<double>> &state_vector_,
    const std::string &qubit_ = "");
  ~Qubit ();

  Qubit ();
  static TypeId GetTypeId ();

  std::vector<std::complex<double>> GetStateVector () const;
  std::string GetName () const;
};

} // namespace ns3

#endif /* QUBIT_H */