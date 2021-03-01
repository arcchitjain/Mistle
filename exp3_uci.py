from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
import os
import matplotlib
import matplotlib.pyplot as plt
import mplcyberpunk
import random
import sys
from time import time

# plt.style.use("cyberpunk")
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
# os.chdir("..")
mining4sat_absolute_path = os.path.abspath("Resources/Mining4SAT")


dataset_class_vars = {
    "iris_17.dat": (17, 18),
    "iris_18.dat": (17, 18),
    "iris_19.dat": (17, 18),
    "zoo.dat": (36, 37),
    "glass.dat": (42, 43),
    "wine.dat": (66, 67),
    "ecoli.dat": (27, 28),
    "hepatitis.dat": (55, 56),
    "heart.dat": (48, 49),
    "dermatology.dat": (44, 45),
    "auto.dat": (132, 133),
    "breast.dat": (19, 20),
    "horseColic.dat": (84, 85),
    "pima.dat": (37, 38),
    "congres.dat": (33, 34),
    "ticTacToe.dat": (28, 29),
    "ionosphere.dat": (156, 157),
    "flare.dat": (31, 32),
    "cylBands.dat": (123, 124),
    "led.dat": (15, 16),
    "soyabean.dat": (100, 101),
    "pageBlocks.dat": (42, 43),
    "nursery.dat": (28, 29),
    "chessKRvK.dat": (41, 42),
}


# topk = 5k-20k
minsup_dict = {
    "iris_17.dat": 0.006666666666666667,
    "iris_18.dat": 0.006666666666666667,
    "iris_19.dat": 0.006666666666666667,
    "zoo.dat": 0.009900990099009901,
    "glass.dat": 0.004672897196261682,
    "wine.dat": 0.015625,
    "ecoli.dat": 0.002976190476190476,
    "hepatitis.dat": 0.25,
    "heart.dat": 0.03125,
    "dermatology.dat": 0.0078125,
    "auto.dat": 0.11640381366943867,
    "breast.dat": 0.001430615164520744,
    "horseColic.dat": 0.06149123818307586,
    "pima.dat": 0.0013020833333333333,
    "congres.dat": 0.1995470492747552,
    "ticTacToe.dat": 0.0078125,
    "ionosphere.dat": 0.27960369179497835,
    "flare.dat": 0.0007199424046076314,
    "cylBands.dat": 0.45031445120414193,
    "led.dat": 0.0003125,
    "soyabean.dat": 0.49355773072313225,
}


def initialize_theory(negatives):
    theory = []
    for pa in negatives:
        theory.append(frozenset([-literal for literal in pa]))

    return theory


def load_cnf(file):
    clauses = []
    negatives = []
    with open(file, "r+") as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            if line[0] == "c" or line[0] == "p":
                continue
            clauses.append(
                frozenset([int(literal) for literal in line.split(" ")[:-1]])
            )
            negatives.append(
                frozenset([-int(literal) for literal in line.split(" ")[:-1]])
            )
    return clauses, negatives


def get_alphabet_size(clauses):
    alphabets = set()
    for clause in clauses:
        for literal in clause:
            alphabets.add(abs(int(literal)))

    return len(alphabets)


def write_cnf(clauses, file):
    alphabet_size = get_alphabet_size(clauses)
    with open(file, "w+") as f:
        f.write("p cnf " + str(alphabet_size) + " " + str(len(clauses)) + "\n")
        for clause in clauses:
            f.write(" ".join([str(literal) for literal in clause]) + " 0\n")


def get_clauses(theory):
    if hasattr(theory, "clauses"):
        return theory.clauses
    else:
        return []


def plot_uci(data, minsup_list=None, dl="me", version=1):
    # positives, negatives = globals()["load_" + dataset]()
    positives, negatives = load_data(data, dataset_class_vars[data])
    pos_theory = initialize_theory(positives)
    neg_theory = initialize_theory(negatives)
    initial_dl = (
        get_dl(
            dl, pos_theory + neg_theory, [], get_alphabet_size(pos_theory + neg_theory)
        )
        + 1
    )
    initial_pos_dl = get_dl(dl, pos_theory, [], get_alphabet_size(pos_theory)) + 1
    initial_neg_dl = get_dl(dl, neg_theory, [], get_alphabet_size(neg_theory)) + 1

    out_path = "Output/CNFs/" + data.split(".")[0]
    mining4sat_compression_list = []
    mistle_lossless_compression_list = []
    mistle_compression_list = []

    if minsup_list is None:
        minsup_list = [round(minsup_dict[data] * (len(positives) + len(negatives)))]

    for support in minsup_list:

        # 1
        mining4sat_time = time()
        mining4sat_pos_theory = run_mining4sat(
            pos_theory, support=support, code_path=mining4sat_absolute_path
        )
        mining4sat_neg_theory = run_mining4sat(
            neg_theory, support=support, code_path=mining4sat_absolute_path
        )

        if mining4sat_pos_theory + mining4sat_neg_theory == []:
            # No theory is learned
            mining4sat_dl = initial_dl
            mining4sat_compression_list.append(0)
        elif mining4sat_pos_theory == []:
            mining4sat_dl = (
                get_dl(
                    dl,
                    mining4sat_neg_theory,
                    [],
                    get_alphabet_size(mining4sat_neg_theory),
                )
                + 1
            )
            mining4sat_compression_list.append(
                (initial_neg_dl - mining4sat_dl) / initial_neg_dl
            )
        elif mining4sat_neg_theory == []:
            mining4sat_dl = (
                get_dl(
                    dl,
                    mining4sat_pos_theory,
                    [],
                    get_alphabet_size(mining4sat_pos_theory),
                )
                + 1
            )
            mining4sat_compression_list.append(
                (initial_pos_dl - mining4sat_dl) / initial_pos_dl
            )
        else:
            mining4sat_dl = (
                get_dl(
                    dl,
                    mining4sat_pos_theory + mining4sat_neg_theory,
                    [],
                    get_alphabet_size(mining4sat_pos_theory + mining4sat_neg_theory),
                )
                + 1
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )

        # if mining4sat_dl == 0:
        #     mining4sat_compression_list.append(0)
        # else:
        #     mining4sat_compression_list.append(
        #         (initial_dl - mining4sat_dl) / initial_dl
        #     )
        mining4sat_time = time() - mining4sat_time

        # 2
        mistle_lossless_time = time()
        mistle_pos = Mistle([], positives)
        # mistle_lossless_pos_theory, _ = mistle_pos.learn(
        #     dl_measure=dl,
        #     minsup=support,
        #     lossy=False,
        #     k=5000,
        #     allow_empty_positives=True,
        # )
        # mistle_neg = Mistle([], negatives)
        # mistle_lossless_neg_theory, _ = mistle_neg.learn(
        #     dl_measure=dl,
        #     minsup=support,
        #     lossy=False,
        #     k=5000,
        #     allow_empty_positives=True,
        # )
        # write_cnf(
        #     get_clauses(mistle_lossless_pos_theory),
        #     out_path + "_L" + str(support) + "_lossless_pos.cnf",
        # )
        # write_cnf(
        #     get_clauses(mistle_lossless_neg_theory),
        #     out_path + "_L" + str(support) + "_lossless_neg.cnf",
        # )
        # mistle_dl = (
        #     get_dl(
        #         dl,
        #         get_clauses(mistle_lossless_pos_theory)
        #         + get_clauses(mistle_lossless_neg_theory),
        #         [],
        #         get_alphabet_size(
        #             get_clauses(mistle_lossless_pos_theory)
        #             + get_clauses(mistle_lossless_neg_theory)
        #         ),
        #     )
        #     + 1
        # )
        # mistle_lossless_pos_dl = get_dl(
        #     dl,
        #     get_clauses(mistle_lossless_pos_theory),
        #     [],
        #     get_alphabet_size(get_clauses(mistle_lossless_pos_theory)),
        # )
        # mistle_lossless_neg_dl = get_dl(
        #     dl,
        #     get_clauses(mistle_lossless_neg_theory),
        #     [],
        #     get_alphabet_size(get_clauses(mistle_lossless_neg_theory)),
        # )

        # if mistle_dl == 0:
        #     mistle_lossless_compression_list.append(0)
        # else:
        #     mistle_lossless_compression_list.append(
        #         (initial_dl - mistle_dl) / initial_dl
        #     )
        # mistle_lossless_time = time() - mistle_lossless_time

        # 3
        mistle_lossy_time = time()
        mistle_plus = Mistle(positives, negatives)
        mistle_plus_theory, comp_plus = mistle_plus.learn(
            dl_measure=dl, minsup=support, lossy=True, k=5000
        )
        mistle_minus = Mistle(negatives, positives)
        mistle_minus_theory, comp_minus = mistle_minus.learn(
            dl_measure=dl, minsup=support, lossy=True, k=5000
        )
        # write_cnf(
        #     get_clauses(mistle_plus_theory), out_path + "_L" + str(support) + "_plus.cnf"
        # )
        # write_cnf(
        #     get_clauses(mistle_minus_theory), out_path + "_L" + str(support) + "_minus.cnf"
        # )
        # mistle_pos_dl = get_dl(
        #     dl,
        #     get_clauses(mistle_plus_theory),
        #     [],
        #     get_alphabet_size(get_clauses(mistle_plus_theory)),
        # )
        # mistle_neg_dl = get_dl(
        #     dl,
        #     get_clauses(mistle_minus_theory),
        #     [],
        #     get_alphabet_size(get_clauses(mistle_minus_theory)),
        # )
        # if mistle_pos_dl > mistle_neg_dl:
        #     mistle_compression_list.append((initial_dl - mistle_neg_dl) / initial_dl)
        # else:
        #     mistle_compression_list.append((initial_dl - mistle_pos_dl) / initial_dl)
        if comp_plus > comp_minus:
            # mistle_comp_theory = mistle_plus_theory
            mistle_compression_list.append(comp_plus)
            print("Comp Theory is T+.")
        else:
            # mistle_comp_theory = mistle_minus_theory
            mistle_compression_list.append(comp_minus)
            print("Comp Theory is T-.")
        mistle_lossy_time = time() - mistle_lossy_time

        if mining4sat_pos_theory == []:
            print("mining4sat_pos_theory is not learned.")
        if mining4sat_neg_theory == []:
            print("mining4sat_neg_theory is not learned.")
        if mistle_plus_theory is None:
            print("mistle_plus_theory is not learned.")
        if mistle_minus_theory is None:
            print("mistle_minus_theory is not learned.")

        print("\nMinsup\t\t\t\t\t: " + str(support))
        print("Initial DL\t\t\t\t: " + str(initial_dl))
        print("Mining4SAT Time\t\t\t: " + str(mining4sat_time))
        print("Mining4SAT DL\t\t\t: " + str(mining4sat_dl))
        # print("Mistle Lossless Time\t: " + str(mistle_lossless_time))
        # print("Mistle Lossless Pos DL\t: " + str(mistle_lossless_pos_dl))
        # print("Mistle Lossless Neg DL\t: " + str(mistle_lossless_neg_dl))
        # print("Mistle Lossless DL\t\t: " + str(mistle_dl))
        print("Mistle Lossy Time\t\t: " + str(mistle_lossy_time))
        print("Mistle Lossy Plus DL\t: " + str((1 - comp_plus) * (initial_dl - 1)))
        print("Mistle Lossy Minus DL\t: " + str((1 - comp_minus) * (initial_dl - 1)))

    # plt.figure()
    # if dl == "ll":
    #     plt.ylabel("Compression\n(DL: Literal Length)")
    # elif dl == "sl":
    #     plt.ylabel("Compression\n(DL: Symbol Length)")
    # elif dl == "se":
    #     plt.ylabel("Compression\n(DL: Shanon Entropy)")
    # elif dl == "me":
    #     plt.ylabel("Compression\n(DL: Modified Entropy)")
    #
    # plt.xlabel("Minimum support threshold")
    # plt.title("UCI: " + data)
    #
    # plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
    # plt.plot(
    #     minsup_list,
    #     mistle_lossless_compression_list,
    #     marker="o",
    #     label="Mistle (lossless)",
    # )
    # plt.plot(minsup_list, mistle_compression_list, marker="o", label="Mistle (lossy)")
    #
    # plt.ylim(bottom=0, top=1.0)
    # plt.legend()
    # # mplcyberpunk.add_underglow()
    # # plt.savefig(
    # #     "Experiments/exp3_uci_" + dataset + "_v" + str(version) + ".pdf",
    # #     bbox_inches="tight",
    # # )
    # plt.show()
    # plt.close()

    print("\nData\t\t\t\t\t\t\t: " + str(data))
    print("Minsup List\t\t\t\t\t\t: " + str(minsup_list))
    print("Compression: Mining4SAT\t\t\t: " + str(mining4sat_compression_list))
    print("Compression: Mistle - Lossless\t: " + str(mistle_lossless_compression_list))
    print("Compression: Mistle - Lossy\t\t: " + str(mistle_compression_list))


plot_uci(sys.argv[1], dl="ll")

###############################################################################
# Plot 1: UCI: Breast
###############################################################################

# minsup_list = [1, 10, 30, 50]
# minsup_list = [1]
# data = "wine.dat"
# dl = "me"
# plot_uci(data)

###############################################################################
# Plot 2: UCI: TicTacToe
###############################################################################

# minsup_list = [1, 10, 30, 50]
# dataset = "tictactoe"
# dl = "me"
# plot_uci_with_minsup(dataset, minsup_list, dl)

# ###############################################################################
# # Plot 3: UCI: Pima
# ###############################################################################
#
# minsup_list = [1, 10, 30, 50]
# dataset = "pima"
# dl = "me"
# plot_uci_with_minsup(dataset, minsup_list, dl)
#
# ###############################################################################
# # Plot 4: UCI: Ionosphere
# ###############################################################################
#
# minsup_list = [50, 100]
# dataset = "ionosphere"
# dl = "me"
# plot_uci_with_minsup(dataset, minsup_list, dl)
