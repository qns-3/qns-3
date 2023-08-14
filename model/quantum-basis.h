#ifndef QUANTUM_BASIS_H
#define QUANTUM_BASIS_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/callback.h"

#include "ns3/pointer.h" // class PointerValue
#include "ns3/uinteger.h" // class UintegerValue
#include "ns3/string.h" // class StringValue
#include "ns3/pair.h" // class PairValue
#include "ns3/boolean.h" // class BooleanValue
#include "ns3/qubit.h" // class Qubit

#include <exatn.hpp>
#include "talsh.h"
#include "talshxx.hpp"
#include "tensor_network.hpp"

#include <string>
#include <vector>
#include <complex>
#include <map>
#include <cmath>
#include <climits>

namespace ns3 {

/* macro */

/** Threshold to check if a double is zero. */
#define EPS (1e-6)

/** A large enough number of seconds since each Simulator::Run (). */
#define ETERNITY (1024)

/** Delimeter to separate different fields in a message. */
#define DELIM (".")

/** The constant duration t, that a default gate error is modeled
 *  as a time-t duration of dephasing. */
#define GATE_DURATION (2e-4)

#define CLASSICAL_DELAY (0.1) // ms

#define TELEP_DELAY (0.5) // seconds

#define DIST_EPR_DELAY (0.005) // seconds

#define SETUP_DELAY (0.1) // seconds


/* logging color */

#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define RED_CODE "\033[91m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"


/* reserved name */

/**
 * \brief Prefix of all reserved names our simulator passes to ExaTN.
 * 
 * \note Users are highly recommended not to create tensors, gates or apps beginning with "QNS_",
 * in order to avoid name conflicts in ExaTN.
*/
const std::string QNS_PREFIX = "QNS_";
const std::string QNS_GATE_PREFIX = QNS_PREFIX + "GATE_";
const std::string QNS_QUBITS_PREFIX = QNS_PREFIX + "QUBITS";
const std::string QNS_EPR_PREFIX = QNS_PREFIX + "EPR";
const std::string QNS_ERROR_PREFIX = QNS_PREFIX + "ERROR";
const std::string QNS_TRACE_PREFIX = QNS_PREFIX + "TRACED";

const std::string QNS_EXATN_PREFIX = QNS_PREFIX + "EXATN";
const std::string QNS_ANCILLA_PREFIX = QNS_PREFIX + "ANCILLA";

const std::string QNS_TELEP_PREFIX = QNS_PREFIX + "TELEP";
const std::string QNS_DISTILL_PREFIX = QNS_PREFIX + "DISTILL";

const std::string APP_DIST_EPR = QNS_PREFIX + "dist_epr";
const std::string APP_TELEP = QNS_PREFIX + "telep";
const std::string APP_DISTILL = QNS_PREFIX + "distill";
const std::string APP_DISTILL_NESTED = QNS_PREFIX + "distill_nested";



/* util */

/**
 * \brief Allocate a new ExaTN tensor name for a ancilla qubit in some protocol.
 * 
 * \return The name of the new tensor.
*/
std::string AllocAncillaQubit ();

std::ostream &operator<< (std::ostream &out, const std::complex<double> &val);

std::vector<std::complex<double>> operator* (std::complex<double> scalar,
                                             std::vector<std::complex<double>> complexVec);

std::vector<std::complex<double>> operator+ (std::vector<std::complex<double>> complex1,
                                             std::vector<std::complex<double>> complex2);

bool operator== (std::vector<std::complex<double>> complex1,
                 std::vector<std::complex<double>> complex2);

unsigned Log2 (unsigned size);

std::vector<std::string> GetPreHalf (const std::vector<std::string> &qubits);

std::vector<std::string> GetSufHalf (const std::vector<std::string> &qubits);

/**
 * \brief Pick an outcome according to the probability distribution.
 * \param prob The probability of outcoming 0.
 * \return 0 or 1.
 * 
 * \note The return value is a complex number, but the imaginary part should be zero.
*/
std::complex<double> PickOutcome (const double &prob);



/* constant */

/** State vector for ket 0. */
const std::vector<std::complex<double>> q_ket_0 = {
    {1.0, 0.0},
    {0.0, 0.0}};

/** State vector for ket 1. */
const std::vector<std::complex<double>> q_ket_1 = {
    {0.0, 0.0},
    {1.0, 0.0}};

/** State vector for bell state. */
const std::vector<std::complex<double>> q_bell = {
    {1. / sqrt (2), 0.0},
    {0.0, 0.0},
    {0.0, 0.0},
    {1. / sqrt (2), 0.0}};

/** Pauli I gate. */
const std::vector<std::complex<double>> pauli_I = {{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}};

/** Pauli X gate. */
const std::vector<std::complex<double>> pauli_X = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};

/** Pauli Y gate. */
const std::vector<std::complex<double>> pauli_Y = {{0.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}, {0.0, 0.0}};

/** Pauli X multipled with pauli Z. */
const std::vector<std::complex<double>> pauli_XZ = {
    {0.0, 0.0}, {-1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};

/** Pauli Z gate. */
const std::vector<std::complex<double>> pauli_Z = {{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}};

/** |0><0|. */
const std::vector<std::complex<double>> meas_0 = {
    {1.0, 0.0},
    {0.0, 0.0},
    {0.0, 0.0},
    {0.0, 0.0}};

/** |1><1|. */
const std::vector<std::complex<double>> meas_1 = {
    {0.0, 0.0},
    {0.0, 0.0},
    {0.0, 0.0},
    {1.0, 0.0}};

/** |0><0| tensor I. */
const std::vector<std::complex<double>> P0I = {
    {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},

    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};

/** |1><1| tensor X */
const std::vector<std::complex<double>> P1X = { 
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},

    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};

/** |1><1| tensor Z */
const std::vector<std::complex<double>> P1Z = {
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},

    {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}};

/** Hardarmard gate. */
const std::vector<std::complex<double>> hadamard = {
    {1. / sqrt (2), 0.0}, {1. / sqrt (2), 0.0}, {1. / sqrt (2), 0.0}, {-1. / sqrt (2), 0.0}};

/** Controlled-not gate. */
const std::vector<std::complex<double>> cnot = {
    {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, 
    {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, 
    {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};
    // {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, 
    // {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0},
    // {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, 
    // {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};

/** Toffoli gate. */
const std::vector<std::complex<double>> toffoli = {
    {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},

    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};

/** Quantum or gate. */
const std::vector<std::complex<double>> qor = {
    {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},

    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};

/** Swap gate. */
const std::vector<std::complex<double>> swap = {
    {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, 
    {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0},
    {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, 
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}};

/** Controlled Z gate. */
const std::vector<std::complex<double>> cz = {{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
                                              {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
                                              {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0},
                                              {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}};


/** 
 * \brief Map from a gate name to its data.
 * 
 * \note If a gate name is in this reserved map, the data user provides will be ignored.
*/
const std::map<std::string, std::vector<std::complex<double>>> gate2data = {
    {QNS_GATE_PREFIX + "I", pauli_I},    {QNS_GATE_PREFIX + "PX", pauli_X},
    {QNS_GATE_PREFIX + "PY", pauli_Y},   {QNS_GATE_PREFIX + "PZ", pauli_Z},
    {QNS_GATE_PREFIX + "H", hadamard},   {QNS_GATE_PREFIX + "CNOT", cnot},
    {QNS_GATE_PREFIX + "TOFF", toffoli}, {QNS_GATE_PREFIX + "QOR", qor},
    {QNS_GATE_PREFIX + "SWAP", swap},    {QNS_GATE_PREFIX + "CZ", cz}};

} // namespace ns3

#endif /* QUANTUM_BASIS_H */