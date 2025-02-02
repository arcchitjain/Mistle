from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import numpy as np
from time import time
from mining4sat_wrapper import run_mining4sat
from mistle_v2 import get_dl, Mistle
import os
import matplotlib
import matplotlib.pyplot as plt
import mplcyberpunk

plt.style.use("seaborn")

matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=24)
matplotlib.rc("axes", titlesize=22)
matplotlib.rc("axes", labelsize=22)
matplotlib.rc("xtick", labelsize=22)
matplotlib.rc("ytick", labelsize=22)
matplotlib.rc("legend", fontsize=22)
matplotlib.rc("figure", titlesize=22)
seed = 0
random.seed(seed)
np.random.seed(seed)
os.chdir("..")
mining4sat_absolute_path = os.path.abspath("Resources/Mining4SAT")


def initialize_theory(negatives):
    theory = []
    for pa in negatives:
        theory.append(frozenset([-literal for literal in pa]))

    return theory


def load_cnf(file):
    theory = []
    negatives = []
    with open(file, "r+") as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            if line[0] == "c" or line[0] == "p":
                continue
            theory.append(frozenset([int(literal) for literal in line.split(" ")[:-1]]))
            negatives.append(
                frozenset([-int(literal) for literal in line.split(" ")[:-1]])
            )
    return theory, negatives


def get_alphabet_size(theory):
    alphabets = set()
    for clause in theory:
        for literal in clause:
            alphabets.add(abs(int(literal)))

    return len(alphabets)


def write_cnf(theory, file):
    alphabet_size = get_alphabet_size(theory)
    with open(file, "w+") as f:
        f.write("p cnf " + str(alphabet_size) + " " + str(len(theory)) + "\n")
        for clause in theory:
            f.write(" ".join([str(literal) for literal in clause]) + " 0\n")


dl = "ll"
version = 1

###############################################################################
# Plot 1: File:     eq.atree.braun.10.unsat.cnf
###############################################################################

# minsup_list = [1, 2, 3, 4, 5]
# file = "eq.atree.braun.10.unsat.cnf"
# in_path = "Data/CNFs/" + file
# out_path = "Output/CNFs/" + file[:-4]
# mining4sat_compression_list = []
# mistle_compression_list = []
#
# for support in minsup_list:
#
#     theory, negatives = load_cnf(in_path)
#     initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
#
#     mining4sat_theory = run_mining4sat(
#         theory, support=support, code_path=mining4sat_absolute_path
#     )
#     mining4sat_dl = get_dl(
#         dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
#     )
#     if mining4sat_dl == 0:
#         mining4sat_compression_list.append(0)
#     else:
#         mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)
#
#     mistle = Mistle([], negatives)
#     mistle_theory, compression = mistle.learn(
#         dl_measure=dl,
#         minsup=support,
#         lossy=False,
#         prune=True,
#         allow_empty_positives=True,
#     )
#     write_cnf(mistle_theory.clauses, out_path + "_L" + str(support) + ".cnf")
#     mistle_compression_list.append(compression)
#
# plt.figure(1)
# if dl == "ll":
#     plt.ylabel("Compression\n(DL: Literal Length)")
# elif dl == "sl":
#     plt.ylabel("Compression\n(DL: Symbol Length)")
# elif dl == "se":
#     plt.ylabel("Compression\n(DL: Shanon Entropy)")
# elif dl == "me":
#     plt.ylabel("Compression\n(DL: Modified Entropy)")
# plt.xlabel("Minimum support threshold")
# # plt.title("File: eq.atree.braun.10.unsat.cnf")
#
# print("minsup_list = " + str(minsup_list))
# print("mining4sat_compression_list = " + str(mining4sat_compression_list))
# print("mistle_compression_list = " + str(mistle_compression_list))
#
# # minsup_list = [1, 2, 3, 4, 5]
# # mining4sat_compression_list = [0.0020437359493153486, 0.0020437359493153486, 0, 0, 0]
# # mistle_compression_list = [0.007766196607398324, 0.007766196607398324, 0.007766196607398324, 0.007766196607398324, 0.007766196607398324]
#
# plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
# plt.plot(minsup_list, mistle_compression_list, marker="o", label="Mistle")
#
# plt.legend()
# # mplcyberpunk.add_underglow()
# plt.savefig("Experiments/exp3_sat_plot1_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

###############################################################################
# Plot 2: File: eq.atree.braun.11.unsat.cnf
###############################################################################

# minsup_list = [1, 2, 3, 4, 5]
# file = "eq.atree.braun.11.unsat.cnf"
# in_path = "Data/CNFs/" + file
# out_path = "Output/CNFs/" + file[:-4]
#
# mining4sat_compression_list = []
# mistle_compression_list = []
#
# for support in minsup_list:
#
#     theory, negatives = load_cnf(in_path)
#     initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
#
#     mining4sat_theory = run_mining4sat(
#         theory, support=support, code_path=mining4sat_absolute_path
#     )
#     mining4sat_dl = get_dl(
#         dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
#     )
#     if mining4sat_dl == 0:
#         mining4sat_compression_list.append(0)
#     else:
#         mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)
#
#     mistle = Mistle([], negatives)
#     mistle_theory, compression = mistle.learn(
#         dl_measure=dl,
#         minsup=support,
#         lossy=False,
#         prune=True,
#         allow_empty_positives=True,
#     )
#     write_cnf(mistle_theory.clauses, out_path + "_L" + str(support) + ".cnf")
#     mistle_compression_list.append(compression)
#
# plt.figure(2)
# if dl == "ll":
#     plt.ylabel("Compression\n(DL: Literal Length)")
# elif dl == "sl":
#     plt.ylabel("Compression\n(DL: Symbol Length)")
# elif dl == "se":
#     plt.ylabel("Compression\n(DL: Shanon Entropy)")
# elif dl == "me":
#     plt.ylabel("Compression\n(DL: Modified Entropy)")
# plt.xlabel("Minimum support threshold")
# # plt.title("File: eq.atree.braun.11.unsat.cnf")
#
# print("minsup_list = " + str(minsup_list))
# print("mining4sat_compression_list = " + str(mining4sat_compression_list))
# print("mistle_compression_list = " + str(mistle_compression_list))
#
# # minsup_list = [1, 2, 3, 4, 5]
# # mining4sat_compression_list = [0.0003252032520325203, 0.0003252032520325203, 0, 0, 0]
# # mistle_compression_list = [0.006829268292682927, 0.006829268292682927, 0.006829268292682927, 0.006829268292682927, 0.006829268292682927]
#
# plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
# plt.plot(minsup_list, mistle_compression_list, marker="o", label="Mistle")
#
# plt.legend()
# plt.savefig("Experiments/exp3_sat_plot2_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()


###############################################################################
# Plot 3: File: goldb-heqc-frg1mul.cnf
###############################################################################

minsup_list = [1, 2, 3, 4, 5]
file = "goldb-heqc-frg1mul.cnf"
in_path = "Data/CNFs/" + file
out_path = "Output/CNFs/" + file[:-4]

mining4sat_compression_list = []
mistle_compression_list = []

for support in minsup_list:

    theory, negatives = load_cnf(in_path)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))

    mining4sat_theory = run_mining4sat(
        theory, support=support, code_path=mining4sat_absolute_path
    )
    mining4sat_dl = get_dl(
        dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
    )
    if mining4sat_dl == 0:
        mining4sat_compression_list.append(0)
    else:
        mining4sat_compression_list.append((initial_dl - mining4sat_dl) / initial_dl)

    mistle = Mistle([], negatives)
    mistle_theory, compression = mistle.learn(
        dl_measure=dl,
        minsup=support,
        lossy=False,
        prune=True,
        allow_empty_positives=True,
    )
    write_cnf(mistle_theory.clauses, out_path + "_L" + str(support) + ".cnf")
    mistle_compression_list.append(compression)

plt.figure(2)
if dl == "ll":
    plt.ylabel("Compression\n(DL: Literal Length)")
elif dl == "me":
    plt.ylabel("Compression\n(DL: Description Length)")
plt.xlabel("Minimum support threshold")

print("minsup_list = " + str(minsup_list))
print("mining4sat_compression_list = " + str(mining4sat_compression_list))
print("mistle_compression_list = " + str(mistle_compression_list))

plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
plt.plot(minsup_list, mistle_compression_list, marker="o", label="Mistle")

plt.legend()
plt.savefig("Experiments/exp3_sat_plot3_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
