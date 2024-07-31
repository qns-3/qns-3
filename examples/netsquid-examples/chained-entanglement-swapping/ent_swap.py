from repeater_chain import *
from netsquid.qubits.qformalism import QFormalism, set_qstate_formalism

set_qstate_formalism(QFormalism.DM)

import time
import tracemalloc

for num_nodes in range(10, 160, 10):
	for _ in range(10):
		start_time = time.time()
		tracemalloc.start()
		run_simulation(num_nodes=num_nodes, node_distance=50, num_iters=10000)["fidelity"]
		current, peak = tracemalloc.get_traced_memory()
		tracemalloc.stop()
		end_time = time.time()

