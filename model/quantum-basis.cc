#include "ns3/quantum-basis.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumBasis");

/* util */

unsigned ancilla_count;

std::string
AllocAncillaQubit ()
{
  return QNS_ANCILLA_PREFIX + std::to_string (ancilla_count++);
}

std::ostream &
operator<< (std::ostream &out, const std::complex<double> &val)
{
  if (norm (val) < EPS)
    {
      out << std::fixed << std::setw(10) << "0    ";
    }
  else if (abs (val.imag ()) < EPS)
    {
      if (val.real () > 0)
        out << std::fixed << std::setw(9) << val.real () << " ";
      else
        out << std::fixed << std::setw(8) << val.real () << " ";
    }
  else if (abs (val.real ()) < EPS)
    {
      if (val.imag () > 0)
        out << std::fixed << std::setw(10) << val.imag () << "·i";
      else
        out << std::fixed << std::setw(9) << val.imag () << "·i";
    }
  else
    {
      out << std::fixed << std::setprecision(2) << val.real () << "+" << val.imag () << "i";
    }
  return out;
}

std::vector<std::complex<double>>
operator* (std::complex<double> scalar, std::vector<std::complex<double>> vec)
{
  for (int i = 0; i < vec.size (); i++)
    {
      vec[i] *= scalar;
    }
  return vec;
}

std::vector<std::complex<double>>
operator+ (std::vector<std::complex<double>> vec1, std::vector<std::complex<double>> vec2)
{
  assert (vec1.size () == vec2.size ());
  for (int i = 0; i < vec1.size (); i++)
    {
      vec1[i] += vec2[i];
    }
  return vec1;
}

bool
operator== (std::vector<std::complex<double>> vec1, std::vector<std::complex<double>> vec2)
{
  assert (vec1.size () == vec2.size ());
  for (int i = 0; i < vec1.size (); i++)
    {
      if (abs (norm (vec1[i] - vec2[i])) > EPS)
        return false;
    }
  return true;
}

unsigned
Log2 (unsigned size)
{
  unsigned log_2_size = -1;
  for (unsigned i = size; i; i >>= 1)
    {
      ++log_2_size;
    }
  return log_2_size;
}

std::vector<std::string>
GetPreHalf (const std::vector<std::string> &qubits)
{
  std::vector<std::string> pre_half = {};
  for (unsigned i = 0; i < qubits.size () / 2; ++i)
    pre_half.push_back (qubits[i]);
  return pre_half;
}

std::vector<std::string>
GetSufHalf (const std::vector<std::string> &qubits)
{
  std::vector<std::string> suf_half = {};
  for (unsigned i = qubits.size () / 2; i < qubits.size (); ++i)
    suf_half.push_back (qubits[i]);
  return suf_half;
}

std::complex<double>
PickOutcome (const double &prob)
{
  NS_LOG_INFO (LIGHT_YELLOW_CODE << "Picking outcome with probability distribution: [" << prob << ", " << 1 - prob << "]" << END_CODE);

  double div = (double) (rand ()) / ((double) (RAND_MAX));
  if (div < prob)
    return {0.0, 0.0};
  return {1.0, 0.0};
}

} // namespace ns3