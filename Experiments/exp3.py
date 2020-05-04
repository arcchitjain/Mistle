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


support = 1
dl = "ce"

th = GeneratedTheory([[1, -4], [2, 5], [6, -7, -8]])
generator = TheoryNoisyGeneratorOnDataset(th, 400, 0.01)
_, negatives = generator.generate_dataset()

os.chdir("..")
theory = initialize_theory(negatives)

initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
print("Initial DL\t\t: " + str(initial_dl))

# fig, (ax1, ax2) = plt.subplots(2, 1)

###############################################################################
# Plot 1: Increase minimum support threshold
###############################################################################

# minsup_list = [1, 2, 5, 10]
minsup_list = [1, 2, 5, 10, 15, 20, 25]
mining4sat_dl_list = []
mistle_dl_list = []
initial_dl_list = [initial_dl] * len(minsup_list)

for support in minsup_list:

    mining4sat_theory = run_mining4sat(
        theory,
        support=support,
        code_path="/Users/arcchit/Docs/Mistle/Resources/Mining4SAT",
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_dl_list.append(mining4sat_dl)

    mistle = Mistle([], negatives)
    mistle_theory, compression = mistle.learn(
        dl_measure=dl, minsup=support, lossy=False
    )
    mistle_dl = get_dl(
        dl, mistle_theory.clauses, [], get_alphabet_size(mistle_theory.clauses)
    )
    mistle_dl_list.append(mistle_dl)

plt.figure(1)
plt.ylabel("Description Length (in bits)")
plt.xlabel("Minimum support threshold")
plt.title("Mistle outperforms Mining4SAT when minsup is varied")

plt.plot(minsup_list, initial_dl_list, marker="o", label="Uncompressed DL")
plt.plot(minsup_list, mining4sat_dl_list, marker="o", label="Mining4SAT DL")
plt.plot(minsup_list, mistle_dl_list, marker="o", label="Mistle DL")

plt.legend(loc="upper right", bbox_to_anchor=(0.95, 0.9))
mplcyberpunk.add_glow_effects()
# mplcyberpunk.make_lines_glow(plt)
# mplcyberpunk.add_underglow(plt)
plt.show()

###############################################################################
# Plot 2: Increase noise
###############################################################################

support = 1
# noise_list = [0, 0.001, 0.01, 0.02]
noise_list = [0, 0.001, 0.01, 0.02, 0.05, 0.10, 0.15, 0.20, 0.25]
mining4sat_dl_list = []
mistle_dl_list = []
initial_dl_list = []

for noise in noise_list:
    generator = TheoryNoisyGeneratorOnDataset(th, 400, noise)
    _, negatives = generator.generate_dataset()
    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
    initial_dl_list.append(initial_dl)

    mining4sat_theory = run_mining4sat(
        theory,
        support=support,
        code_path="/Users/arcchit/Docs/Mistle/Resources/Mining4SAT",
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_dl_list.append(mining4sat_dl)

    mistle = Mistle([], negatives)
    mistle_theory, _ = mistle.learn(dl_measure=dl, minsup=support, lossy=False)
    mistle_dl = get_dl(
        dl, mistle_theory.clauses, [], get_alphabet_size(mistle_theory.clauses)
    )
    mistle_dl_list.append(mistle_dl)

plt.figure(2)
plt.ylabel("Description Length (in bits)")
plt.xlabel("Noise in the input data")
plt.title("Mistle outperforms Mining4SAT when data becomes noisy")

plt.plot(noise_list, initial_dl_list, marker="o", label="Uncompressed DL")
plt.plot(noise_list, mining4sat_dl_list, marker="o", label="Mining4SAT DL")
plt.plot(noise_list, mistle_dl_list, marker="o", label="Mistle DL")

plt.legend()
mplcyberpunk.add_glow_effects()
plt.show()

###############################################################################
# Plot 3: Increase data size
###############################################################################

support = 1
size_list = [64, 128, 256, 512]
negative_size_list = []
mining4sat_dl_list = []
mistle_dl_list = []
initial_dl_list = []

for size in size_list:
    generator = TheoryNoisyGeneratorOnDataset(th, size, 0.1)
    _, negatives = generator.generate_dataset()
    negative_size_list.append(len(negatives))

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
    initial_dl_list.append(initial_dl)

    mining4sat_theory = run_mining4sat(
        theory,
        support=support,
        code_path="/Users/arcchit/Docs/Mistle/Resources/Mining4SAT",
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    mining4sat_dl_list.append(mining4sat_dl)

    mistle = Mistle([], negatives)
    mistle_theory, _ = mistle.learn(dl_measure=dl, minsup=support, lossy=False)
    mistle_dl = get_dl(
        dl, mistle_theory.clauses, [], get_alphabet_size(mistle_theory.clauses)
    )
    mistle_dl_list.append(mistle_dl)

plt.figure(3)
plt.ylabel("Description Length (in bits)")
plt.xlabel("Size of the input data")
plt.title("Mistle outperforms Mining4SAT when size of the data is varied")

plt.plot(negative_size_list, initial_dl_list, marker="o", label="Uncompressed DL")
plt.plot(negative_size_list, mining4sat_dl_list, marker="o", label="Mining4SAT DL")
plt.plot(negative_size_list, mistle_dl_list, marker="o", label="Mistle DL")

plt.legend()
mplcyberpunk.add_glow_effects()
plt.show()
