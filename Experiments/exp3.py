from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import numpy as np
from time import time
from mining4sat_wrapper import run_mining4sat
from mistle_v2 import get_dl, Mistle
import os
import matplotlib.pyplot as plt
import mplcyberpunk

plt.style.use("cyberpunk")
seed = 0
random.seed(seed)
np.random.seed(seed)
mining4sat_absolute_path = "/Users/arcchit/Docs/Mistle/Resources/Mining4SAT"


def initialize_theory(negatives):
    theory = []
    for pa in negatives:
        theory.append(frozenset([-literal for literal in pa]))

    return theory


def get_alphabet_size(theory):
    alphabets = set()
    for clause in theory:
        for literal in clause:
            alphabets.add(abs(int(literal)))

    return len(alphabets)


dl = "me"  # For Modified Entropy

th = GeneratedTheory([[1, -4], [2, 5], [6, -7, -8]])
generator = TheoryNoisyGeneratorOnDataset(th, 400, 0.01)
_, negatives = generator.generate_dataset()

os.chdir("..")
theory = initialize_theory(negatives)

initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
print("Initial DL\t\t: " + str(initial_dl))

version = 2

###############################################################################
# Plot 1: Minsup Plot: Increase minimum support threshold
###############################################################################

# minsup_list = [1, 2, 5, 10]
minsup_list = [1, 3, 5, 10, 15, 20, 25, 30, 35]
mining4sat_compression_list = []
mistle_compression_list = []
mistle_prune_compression_list = []

for support in minsup_list:

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False
    )
    # mistle_dl = get_dl(
    #     dl, mistle_theory.clauses, [], get_alphabet_size(mistle_theory.clauses)
    # )
    # mistle_compression_list.append((initial_dl - mistle_dl) / initial_dl)
    mistle_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True
    )
    # mistle_dl = get_dl(
    #     dl, mistle_theory.clauses, [], get_alphabet_size(mistle_theory.clauses)
    # )
    # mistle_prune_compression_list.append((initial_dl - mistle_dl) / initial_dl)
    mistle_prune_compression_list.append(compression)

plt.figure(1)
plt.ylabel("Compression (DL: Modified Entropy)")
plt.xlabel("Minimum support threshold")
plt.title("Mistle outperforms Mining4SAT when minsup is varied")

plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
plt.plot(
    minsup_list, mistle_compression_list, marker="o", label="Mistle (without pruning)"
)
plt.plot(
    minsup_list,
    mistle_prune_compression_list,
    marker="o",
    label="Mistle DL (with pruning)",
)
plt.ylim(bottom=0, top=1)
plt.legend(loc="upper right", bbox_to_anchor=(1, 1))
mplcyberpunk.add_glow_effects()
# mplcyberpunk.make_lines_glow(plt)
# mplcyberpunk.add_underglow(plt)
plt.savefig("Experiments/exp3_plot1_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 2: Noise PLot: Increase noise with DL = Modified Entropy
###############################################################################

support = 1
# noise_list = [0, 0.001, 0.01, 0.02]
noise_list = [0, 0.001, 0.01, 0.02, 0.05, 0.10, 0.15, 0.20, 0.25]
mining4sat_compression_list = []
mistle_compression_list = []
mistle_prune_compression_list = []

for noise in noise_list:
    generator = TheoryNoisyGeneratorOnDataset(th, 400, noise)
    _, negatives = generator.generate_dataset()
    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False
    )
    mistle_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True
    )
    mistle_prune_compression_list.append(compression)

plt.figure(2)
plt.ylabel("Compression (DL: Modified Entropy)")
plt.xlabel("Noise in the input data")
plt.title("Mistle outperforms Mining4SAT when data becomes noisy")

plt.plot(noise_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
plt.plot(
    noise_list, mistle_compression_list, marker="o", label="Mistle (without pruning)"
)
plt.plot(
    noise_list,
    mistle_prune_compression_list,
    marker="o",
    label="Mistle DL (with pruning)",
)

plt.ylim(bottom=0, top=1)
plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot2_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 3: Size Plot: Increase data size with DL = Modified Entropy
###############################################################################

support = 1
size_list = [64, 128, 256, 512]
negative_size_list = []
mining4sat_compression_list = []
mistle_compression_list = []
mistle_prune_compression_list = []

for size in size_list:
    generator = TheoryNoisyGeneratorOnDataset(th, size, 0.1)
    _, negatives = generator.generate_dataset()
    negative_size_list.append(len(negatives))

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False
    )
    mistle_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True
    )
    mistle_prune_compression_list.append(compression)

plt.figure(3)
plt.ylabel("Compression (DL: Modified Entropy)")
plt.xlabel("Size of the input data (Number of clauses)")
plt.title("Mistle outperforms Mining4SAT when size of the data is varied")

plt.plot(
    negative_size_list, mining4sat_compression_list, marker="o", label="Mining4SAT"
)
plt.plot(
    negative_size_list,
    mistle_compression_list,
    marker="o",
    label="Mistle (without pruning)",
)
plt.plot(
    negative_size_list,
    mistle_prune_compression_list,
    marker="o",
    label="Mistle DL (with pruning)",
)

plt.ylim(bottom=0, top=1)
plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot3_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 4: Noise Plot: Increase noise with DL = Literal Length
###############################################################################

support = 1
dl = "ll"
# noise_list = [0, 0.001, 0.01, 0.02]
noise_list = [0, 0.001, 0.01, 0.02, 0.05, 0.10, 0.15, 0.20, 0.25]
mining4sat_compression_list = []
mistle_compression_list = []
mistle_prune_compression_list = []

for noise in noise_list:
    generator = TheoryNoisyGeneratorOnDataset(th, 400, noise)
    _, negatives = generator.generate_dataset()
    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False
    )
    mistle_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True
    )
    mistle_prune_compression_list.append(compression)

plt.figure(4)
plt.ylabel("Compression (DL: Literal Length)")
plt.xlabel("Noise in the input data")
plt.title("Mistle outperforms Mining4SAT when data becomes noisy")

plt.plot(noise_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
plt.plot(
    noise_list, mistle_compression_list, marker="o", label="Mistle (without pruning)"
)
plt.plot(
    noise_list,
    mistle_prune_compression_list,
    marker="o",
    label="Mistle DL (with pruning)",
)

plt.ylim(bottom=0, top=1)
plt.legend(loc="lower left", bbox_to_anchor=(0.03, 0))
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot4_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 5: Size Plot: Increase data size with DL = Literal Length
###############################################################################

support = 1
dl = "ll"
size_list = [64, 128, 256, 512]
negative_size_list = []
mining4sat_compression_list = []
mistle_compression_list = []
mistle_prune_compression_list = []

for size in size_list:
    generator = TheoryNoisyGeneratorOnDataset(th, size, 0.1)
    _, negatives = generator.generate_dataset()
    negative_size_list.append(len(negatives))

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False
    )
    mistle_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True
    )
    mistle_prune_compression_list.append(compression)

plt.figure(5)
plt.ylabel("Compression (DL: Literal Length)")
plt.xlabel("Size of the input data (Number of clauses)")
plt.title("Mistle outperforms Mining4SAT when size of the data is varied")

plt.plot(
    negative_size_list, mining4sat_compression_list, marker="o", label="Mining4SAT"
)
plt.plot(
    negative_size_list,
    mistle_compression_list,
    marker="o",
    label="Mistle (without pruning)",
)
plt.plot(
    negative_size_list,
    mistle_prune_compression_list,
    marker="o",
    label="Mistle DL (with pruning)",
)

plt.ylim(bottom=0, top=1)
plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot5_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 6: Size Plot: Compare Runtime
###############################################################################

support = 1
dl = "ll"
size_list = [64, 128, 256, 512]
negative_size_list = []
mining4sat_runtime_list = []
mistle_m1_runtime_list = []
mistle_np_m1_runtime_list = []
mistle_runtime_list = []
mistle_np_runtime_list = []

for size in size_list:
    generator = TheoryNoisyGeneratorOnDataset(th, size, 0.1)
    _, negatives = generator.generate_dataset()
    negative_size_list.append(len(negatives))

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    # 1
    start_time = time()
    _ = run_mining4sat(theory, support=support, code_path=mining4sat_absolute_path)
    mining4sat_runtime_list.append(time() - start_time)

    # 2
    start_time = time()
    mistle = Mistle([], negatives)
    _, _ = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True, mining_steps=1
    )
    mistle_m1_runtime_list.append(time() - start_time)

    # 3
    start_time = time()
    mistle = Mistle([], negatives)
    _, _ = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=1
    )
    mistle_np_m1_runtime_list.append(time() - start_time)

    # 4
    start_time = time()
    mistle = Mistle([], negatives)
    _, _ = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=True, mining_steps=None
    )
    mistle_runtime_list.append(time() - start_time)

    # 5
    start_time = time()
    mistle = Mistle([], negatives)
    _, _ = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=None
    )
    mistle_np_runtime_list.append(time() - start_time)


plt.figure(6)
plt.ylabel("Runtime (in seconds)")
plt.xlabel("Size of the input data (Number of clauses)")
plt.title("Mining4SAT outperforms Mistle in runtime")

plt.plot(negative_size_list, mining4sat_runtime_list, marker="o", label="Mining4SAT")
plt.plot(
    negative_size_list,
    mistle_m1_runtime_list,
    marker="o",
    label="Mistle (pruning and mining only once)",
)
plt.plot(
    negative_size_list,
    mistle_np_m1_runtime_list,
    marker="o",
    label="Mistle (no pruning and mining only once)",
)
plt.plot(
    negative_size_list,
    mistle_np_runtime_list,
    marker="o",
    label="Mistle (no pruning and no mining cap)",
)
plt.plot(
    negative_size_list,
    mistle_runtime_list,
    marker="o",
    label="Mistle (pruning and no mining cap)",
)

plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot6_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 7: Size plot: Use less operators and no pruning and mine once
###############################################################################

support = 1
size_list = [64, 128, 256, 512]
negative_size_list = []
mining4sat_compression_list = []
mistle_w_compression_list = []
mistle_w_s_compression_list = []
mistle_all_compression_list = []

for size in size_list:
    generator = TheoryNoisyGeneratorOnDataset(th, size, 0.1)
    _, negatives = generator.generate_dataset()
    negative_size_list.append(len(negatives))

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl,
        minsup=support,
        lossy=False,
        prune=False,
        mining_steps=1,
        permitted_operators={
            "D": False,
            "W": True,
            "V": False,
            "S": False,
            "R": False,
            "T": False,
        },
    )
    mistle_w_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl,
        minsup=support,
        lossy=False,
        prune=False,
        mining_steps=1,
        permitted_operators={
            "D": False,
            "W": True,
            "V": False,
            "S": True,
            "R": False,
            "T": False,
        },
    )
    mistle_w_s_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=1
    )
    mistle_all_compression_list.append(compression)


plt.figure(7)
plt.ylabel("Compression (DL: Modified Entropy)")
plt.xlabel("Size of the input data (Number of clauses)")
plt.title("Mistle outperforms Mining4SAT even with fewer operators and without pruning")

plt.plot(
    negative_size_list, mining4sat_compression_list, marker="o", label="Mining4SAT"
)
plt.plot(
    negative_size_list,
    mistle_w_compression_list,
    marker="o",
    label="Mistle - only W operator (without pruning)",
)
plt.plot(
    negative_size_list,
    mistle_w_s_compression_list,
    marker="o",
    label="Mistle - only W and S operators (without pruning)",
)
plt.plot(
    negative_size_list,
    mistle_all_compression_list,
    marker="o",
    label="Mistle - R, S, W operators (without pruning)",
)

plt.ylim(bottom=0, top=1)
plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot7_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()


###############################################################################
# Plot 8: Noise plot: Use less operators and no pruning and mine once
###############################################################################

support = 1
dl = "ll"
# noise_list = [0, 0.001, 0.01, 0.02]
noise_list = [0, 0.001, 0.01, 0.02, 0.05, 0.10, 0.15, 0.20, 0.3, 0.4, 0.5]
mining4sat_compression_list = []
mistle_w_compression_list = []
mistle_w_s_compression_list = []
mistle_all_compression_list = []

for noise in noise_list:
    generator = TheoryNoisyGeneratorOnDataset(th, 400, noise)
    _, negatives = generator.generate_dataset()
    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl,
        minsup=support,
        lossy=False,
        prune=False,
        mining_steps=1,
        permitted_operators={
            "D": False,
            "W": True,
            "V": False,
            "S": False,
            "R": False,
            "T": False,
        },
    )
    mistle_w_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl,
        minsup=support,
        lossy=False,
        prune=False,
        mining_steps=1,
        permitted_operators={
            "D": False,
            "W": True,
            "V": False,
            "S": True,
            "R": False,
            "T": False,
        },
    )
    mistle_w_s_compression_list.append(compression)

    mistle = Mistle([], negatives)
    _, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=1
    )
    mistle_all_compression_list.append(compression)


plt.figure(8)
plt.ylabel("Compression (DL: Modified Entropy)")
plt.xlabel("Noise in the input data")
plt.title("Mistle outperforms Mining4SAT on noisy data even with fewer operators")

plt.plot(noise_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
plt.plot(
    noise_list,
    mistle_w_compression_list,
    marker="o",
    label="Mistle - W operator (without pruning and mining once)",
)
plt.plot(
    noise_list,
    mistle_w_s_compression_list,
    marker="o",
    label="Mistle - S, W operators (without pruning and mining once)",
)
plt.plot(
    noise_list,
    mistle_all_compression_list,
    marker="o",
    label="Mistle - R, S, W operators (without pruning and mining once)",
)

plt.ylim(bottom=0, top=1)
plt.legend()
mplcyberpunk.add_glow_effects()
plt.savefig("Experiments/exp3_plot8_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()
