#include "ns3/qubit.h"

namespace ns3 {

Qubit::Qubit (
  const std::vector<std::complex<double>> &state_vector_,
  const std::string &qubit_
)
  : m_state_vector (state_vector_),
    m_qubit (qubit_)
{
}

Qubit::~Qubit ()
{
  m_state_vector.clear ();
}

Qubit::Qubit () : m_state_vector ({})
{
}

TypeId
Qubit::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Qubit").SetParent<Object> ().AddConstructor<Qubit> ();
  return tid;
}

std::vector<std::complex<double>>
Qubit::GetStateVector () const
{
  return m_state_vector;
}

std::string
Qubit::GetName () const
{
  return m_qubit;
}
} // namespace ns3