from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
import random
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


def get_uci_mcar(dataset, missing_parameter=0.0):
    """
    :param dataset: Name of the uci dataset in string: ["adult", "breast", "chess", "ionoshphere", "mushroom", "pima", "tictactoe"]
    :param missing_parameter: Percentage of values that should be made missing completely at random (MCAR)
    :return: a tuple of incomplete positives and incomplete negatives
    """

    complete_positives, complete_negatives = globals()["load_" + dataset]()

    incomplete_positives = []
    for pa in complete_positives:
        noisy_pa = set()
        for literal in pa:
            if random.random() >= missing_parameter:
                noisy_pa.add(literal)
        incomplete_positives.append(frozenset(noisy_pa))

    incomplete_negatives = []
    for pa in complete_negatives:
        noisy_pa = set()
        for literal in pa:
            if random.random() >= missing_parameter:
                noisy_pa.add(literal)
        incomplete_negatives.append(frozenset(noisy_pa))

    return incomplete_positives, incomplete_negatives


def get_uci_nb_missing(dataset, M=0):
    """
    :param dataset: Name of the uci dataset in string: ["adult", "breast", "chess", "ionoshphere", "mushroom", "pima", "tictactoe"]
    :param M: a positive integer fixating on the exact number of values to be made missing in each row
    :return: a tuple of incomplete positives and incomplete negatives
    """
    complete_positives, complete_negatives = globals()["load_" + dataset]()

    incomplete_positives = []
    missing_positives = []
    for pa in complete_positives:
        missing_literals = random.sample(pa, M)
        missing_positives.append(missing_literals)
        incomplete_positive = set()
        for literal in pa:
            if literal not in missing_literals:
                incomplete_positive.add(literal)
        incomplete_positives.append(frozenset(incomplete_positive))

    incomplete_negatives = []
    missing_negatives = []
    for pa in complete_negatives:
        missing_literals = random.sample(pa, M)
        missing_negatives.append(missing_literals)
        incomplete_negative = set()
        for literal in pa:
            if literal not in missing_literals:
                incomplete_negative.add(literal)
        incomplete_negatives.append(frozenset(incomplete_negative))

    return (
        incomplete_positives,
        incomplete_negatives,
        missing_positives,
        missing_negatives,
    )


def plot_uci(dataset, minsup_list, dl, version=1):
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


def get_all_completions_recursive(incomplete_pa, missing_attributes, result):
    if not missing_attributes:
        return result

    pos_result = copy(result)
    neg_result = copy(result)
    new_incomplete_pa = copy(incomplete_pa)
    attribute = missing_attributes.pop()

    new_result = []

    if result:
        for p_r in pos_result:
            p_r.append(attribute)
            new_result.append(p_r)

        for n_r in neg_result:
            n_r.append(attribute)
            new_result.append(n_r)
    else:
        new_result.append(incomplete_pa + [attribute])
        new_result.append(incomplete_pa + [-attribute])

    new_incomplete_pa.append(attribute)

    return get_all_completions_recursive(
        new_incomplete_pa, missing_attributes, new_result
    )


def impute_missing_values(theory, incomplete_pa, missing_attributes):
    completed_pas = get_all_completions_recursive(
        list(incomplete_pa), missing_attributes, []
    )

    max_indices = []
    max_value = -1
    for i, completed_pa in enumerate(completed_pas):
        nb_failed_clauses = 0
        for clause in get_clauses(theory):
            if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                nb_failed_clauses += 1

        if nb_failed_clauses > max_value:
            max_indices = [i]
            max_value = nb_failed_clauses
        elif nb_failed_clauses == max_value:
            max_indices.append(i)

    return completed_pas[random.choice(max_indices)]


def get_accuracy(
    mistle_pos_theory,
    mistle_neg_theory,
    complete_positives,
    complete_negatives,
    incomplete_positives,
    incomplete_negatives,
    missing_positives,
    missing_negatives,
):
    accuracy = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        # c_p = i_p + m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))
        predicted_c_p = impute_missing_values(
            mistle_pos_theory, i_p, missing_attributes
        )
        if set(predicted_c_p) == set(c_p):
            accuracy += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        # c_n = i_n + m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))
        predicted_c_n = impute_missing_values(
            mistle_neg_theory, i_n, missing_attributes
        )
        if set(predicted_c_n) == set(c_n):
            accuracy += 1

    return accuracy / (len(complete_positives) + len(complete_negatives))


def plot_uci_nb_missing(dataset, M_list, minsup, dl, version):
    complete_positives, complete_negatives = globals()["load_" + dataset]()
    out_path = "Output/CNFs/" + dataset
    mistle_accuracy_list = []

    for m in M_list:
        incomplete_positives, incomplete_negatives, missing_positives, missing_negatives = get_uci_nb_missing(
            dataset, m
        )

        mistle_pos_theory, _ = Mistle(incomplete_negatives, incomplete_positives).learn(
            dl_measure=dl, minsup=minsup, lossy=True
        )
        mistle_neg_theory, _ = Mistle(incomplete_positives, incomplete_negatives).learn(
            dl_measure=dl, minsup=minsup, lossy=True
        )
        write_cnf(
            get_clauses(mistle_pos_theory),
            out_path + "_L" + str(minsup) + "_M" + str(m) + "_pos.cnf",
        )
        write_cnf(
            get_clauses(mistle_neg_theory),
            out_path + "_L" + str(minsup) + "_M" + str(m) + "_neg.cnf",
        )

        accuracy = get_accuracy(
            mistle_pos_theory,
            mistle_neg_theory,
            complete_positives,
            complete_negatives,
            incomplete_positives,
            incomplete_negatives,
            missing_positives,
            missing_negatives,
        )

        mistle_accuracy_list.append(accuracy)

    plt.figure()
    plt.xlabel("Number of missing literals / row")
    plt.ylabel("Completion Accuracy")
    plt.title(
        "Accuracy on "
        + dataset
        + " when an exact number of literals are made missing in each row"
    )

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")

    # plt.ylim(bottom=0, top=1)
    plt.legend()
    mplcyberpunk.add_glow_effects()
    plt.savefig(
        "Experiments/exp2_uci_nb_missing_" + dataset + "_v" + str(version) + ".png",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


###############################################################################
# Plot 1: UCI: Breast
###############################################################################

plot_uci_nb_missing(dataset="breast", M_list=[1, 3, 5], minsup=1, dl="me", version=1)

# ###############################################################################
# # Plot 2: UCI: TicTacToe
# ###############################################################################
#
# minsup_list = [1, 10, 30, 50]
# dataset = "tictactoe"
# dl = "me"
# plot_uci(dataset, minsup_list, dl)
#
# ###############################################################################
# # Plot 3: UCI: Pima
# ###############################################################################
#
# minsup_list = [1, 10, 30, 50]
# dataset = "pima"
# dl = "me"
# plot_uci(dataset, minsup_list, dl)
#
# ###############################################################################
# # Plot 4: UCI: Ionosphere
# ###############################################################################
#
# minsup_list = [50, 100]
# dataset = "ionosphere"
# dl = "me"
# plot_uci(dataset, minsup_list, dl)
