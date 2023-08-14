#include "ns3/quantum-memory.h" // class QuantumMemory

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumMemory");

QuantumMemory::QuantumMemory (std::vector<std::string> qubits_) : m_qubits (qubits_)
{
}

QuantumMemory::~QuantumMemory ()
{
  m_qubits.clear ();
}

QuantumMemory::QuantumMemory () : m_qubits ({})
{
}

TypeId
QuantumMemory::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::QuantumMemory").SetParent<Object> ().AddConstructor<QuantumMemory> ();
  return tid;
}

void
QuantumMemory::AddQubit (std::string qubit)
{
  m_qubits.push_back (qubit);
}

bool
QuantumMemory::RemoveQubit (std::string qubit)
{
  for (unsigned i = 0; i < m_qubits.size (); ++i)
    {
      if (m_qubits[i] == qubit)
        {
          m_qubits.erase (m_qubits.begin () + i);
          return true;
        }
    }
  return false;
}

unsigned
QuantumMemory::GetSize () const
{
  return m_qubits.size ();
}

std::string
QuantumMemory::GetQubit (unsigned local) const
{
  return m_qubits[local];
}

bool
QuantumMemory::ContainQubit (std::string qubit) const
{
  for (unsigned i = 0; i < m_qubits.size (); ++i)
    {
      if (m_qubits[i] == qubit)
        {
          return true;
        }
    }
  return false;
}

} // namespace ns3