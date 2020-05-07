from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
import os
import matplotlib.pyplot as plt
import mplcyberpunk

plt.style.use("cyberpunk")
seed = 0
random.seed(seed)
np.random.seed(seed)
mining4sat_absolute_path = "/Users/arcchit/Docs/Mistle/Resources/Mining4SAT"
os.chdir("..")


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


def plot_uci_with_minsup(dataset, minsup_list, dl, version=1):
    positives, negatives = globals()["load_" + dataset]()
    pos_theory = initialize_theory(positives)
    neg_theory = initialize_theory(negatives)
    initial_dl = (
        get_dl(
            dl, pos_theory + neg_theory, [], get_alphabet_size(pos_theory + neg_theory)
        )
        + 1
    )

    out_path = "Output/CNFs/" + dataset
    mining4sat_compression_list = []
    mistle_lossless_compression_list = []
    mistle_compression_list = []

    for support in minsup_list:

        # 1
        mining4sat_pos_theory = run_mining4sat(
            pos_theory, support=support, code_path=mining4sat_absolute_path
        )
        mining4sat_neg_theory = run_mining4sat(
            neg_theory, support=support, code_path=mining4sat_absolute_path
        )
        mining4sat_dl = (
            get_dl(
                dl,
                mining4sat_pos_theory + mining4sat_neg_theory,
                [],
                get_alphabet_size(mining4sat_pos_theory + mining4sat_pos_theory),
            )
            + 1
        )

        if mining4sat_dl == 0:
            mining4sat_compression_list.append(0)
        else:
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )

        # 2
        mistle_pos = Mistle([], positives)
        mistle_pos_theory, _ = mistle_pos.learn(
            dl_measure=dl, minsup=support, lossy=False
        )
        mistle_neg = Mistle([], negatives)
        mistle_neg_theory, _ = mistle_neg.learn(
            dl_measure=dl, minsup=support, lossy=False
        )
        write_cnf(
            get_clauses(mistle_pos_theory),
            out_path + "_L" + str(support) + "_lossless_pos.cnf",
        )
        write_cnf(
            get_clauses(mistle_neg_theory),
            out_path + "_L" + str(support) + "_lossless_neg.cnf",
        )
        mistle_dl = (
            get_dl(
                dl,
                get_clauses(mistle_pos_theory) + get_clauses(mistle_neg_theory),
                [],
                get_alphabet_size(
                    get_clauses(mistle_pos_theory) + get_clauses(mistle_neg_theory)
                ),
            )
            + 1
        )
        if mistle_dl == 0:
            mistle_lossless_compression_list.append(0)
        else:
            mistle_lossless_compression_list.append(
                (initial_dl - mistle_dl) / initial_dl
            )

        # 3
        mistle_pos = Mistle(negatives, positives)
        mistle_pos_theory, _ = mistle_pos.learn(
            dl_measure=dl, minsup=support, lossy=True
        )
        mistle_neg = Mistle(positives, negatives)
        mistle_neg_theory, _ = mistle_neg.learn(
            dl_measure=dl, minsup=support, lossy=True
        )
        write_cnf(
            get_clauses(mistle_pos_theory), out_path + "_L" + str(support) + "_pos.cnf"
        )
        write_cnf(
            get_clauses(mistle_neg_theory), out_path + "_L" + str(support) + "_neg.cnf"
        )
        mistle_pos_dl = get_dl(
            dl,
            get_clauses(mistle_pos_theory),
            [],
            get_alphabet_size(get_clauses(mistle_pos_theory)),
        )
        mistle_neg_dl = get_dl(
            dl,
            get_clauses(mistle_neg_theory),
            [],
            get_alphabet_size(get_clauses(mistle_neg_theory)),
        )
        if mistle_pos_dl > mistle_neg_dl:
            mistle_compression_list.append((initial_dl - mistle_neg_dl) / initial_dl)
        else:
            mistle_compression_list.append((initial_dl - mistle_pos_dl) / initial_dl)

    plt.figure()
    if dl == "ll":
        plt.ylabel("Compression (DL: Literal Length)")
    elif dl == "sl":
        plt.ylabel("Compression (DL: Symbol Length)")
    elif dl == "se":
        plt.ylabel("Compression (DL: Shanon Entropy)")
    elif dl == "me":
        plt.ylabel("Compression (DL: Modified Entropy)")

    plt.xlabel("Minimum support threshold")
    plt.title("UCI: " + dataset)

    plt.plot(minsup_list, mining4sat_compression_list, marker="o", label="Mining4SAT")
    plt.plot(
        minsup_list,
        mistle_lossless_compression_list,
        marker="o",
        label="Mistle (lossless)",
    )
    plt.plot(minsup_list, mistle_compression_list, marker="o", label="Mistle (lossy)")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    mplcyberpunk.add_glow_effects()
    plt.savefig(
        "Experiments/exp3_uci_" + dataset + "_v" + str(version) + ".png",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


###############################################################################
# Plot 1: UCI: Breast
###############################################################################

minsup_list = [1, 10, 30, 50]
dataset = "breast"
dl = "me"
plot_uci_with_minsup(dataset, minsup_list, dl)

###############################################################################
# Plot 2: UCI: TicTacToe
###############################################################################

minsup_list = [1, 10, 30, 50]
dataset = "tictactoe"
dl = "me"
plot_uci_with_minsup(dataset, minsup_list, dl)

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
