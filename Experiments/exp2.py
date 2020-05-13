from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
import random
import os
import matplotlib
import matplotlib.pyplot as plt
import mplcyberpunk

plt.style.use("cyberpunk")
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


def load_complete_dataset(dataset):
    """
    Apply closed world assumption on the  data to complete it
    :param dataset: Name of the dataset in string
    :return: a tuple of positives and negatives (which are complete, i.e., they contain every variables in each row)
    """
    positives, negatives = globals()["load_" + dataset]()
    complete_positives = []
    complete_negatives = []
    if dataset == "breast":
        var_range = list(range(1, 19))
    elif dataset == "tictactoe":
        var_range = list(range(1, 28))
    elif dataset == "pima":
        var_range = list(range(1, 37))
    elif dataset == "ionosphere":
        var_range = list(range(1, 156))
    else:
        var_range = None

    for positive_pa in positives:
        complete_pa = set(positive_pa)
        for var in var_range:
            if var not in positive_pa:
                complete_pa.add(-var)
        complete_positives.append(complete_pa)

    for negative_pa in negatives:
        complete_pa = set(negative_pa)
        for var in var_range:
            if var not in negative_pa:
                complete_pa.add(-var)
        complete_negatives.append(complete_pa)

    return complete_positives, complete_negatives


def get_clauses(theory):
    """
    :param theory: it can be an instance if a Theory class or could just be None
    :return: a list of clauses present in the theory
    """
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

    complete_positives, complete_negatives = load_complete_dataset(dataset)

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
    complete_positives, complete_negatives = load_complete_dataset(dataset)

    incomplete_positives = []
    missing_positives = []
    for pa in complete_positives:
        missing_literals = frozenset(random.sample(pa, M))
        missing_positives.append(missing_literals)
        incomplete_positive = set()
        for literal in pa:
            if literal not in missing_literals:
                incomplete_positive.add(literal)
        incomplete_positives.append(frozenset(incomplete_positive))

    incomplete_negatives = []
    missing_negatives = []
    for pa in complete_negatives:
        missing_literals = frozenset(random.sample(pa, M))
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


def get_all_completions_recursive(incomplete_pa, missing_attributes, result):
    """
    Example: if incomplete_pa = [1, 2, 3], missing_attributes = [4, 5],
        then the result after the recursion should be:
        [[1, 2, 3, 4, 5], [1, 2, 3, 4, -5], [1, 2, 3, -4, 5], [1, 2, 3, -4, -5]]
    :param incomplete_pa: an incomplete partial assignment taht is needed to be completed
    :param missing_attributes: a list of positive literals whose values are missing in the incomplete_pa
    :param result: a variable that stores the current / temporary value of the final result
    :return: a list of all completions of the incomplete_pa wrt all posisble combinations of missing_attributes
    """
    if not missing_attributes:
        return result

    pos_result = copy(result)
    neg_result = copy(result)
    attribute = missing_attributes.pop()

    new_result = []

    if result:
        for p_r in pos_result:
            l = copy(p_r)
            l.append(attribute)
            new_result.append(l)

        for n_r in neg_result:
            l = copy(n_r)
            l.append(-attribute)
            new_result.append(l)
    else:
        new_result.append(incomplete_pa + [attribute])
        new_result.append(incomplete_pa + [-attribute])

    return get_all_completions_recursive([], missing_attributes, new_result)


def impute_missing_values(incomplete_pa, theory, missing_attributes):
    """
    A function that predicts the most suitable completion of 'incomplete_pa' wrt a theory.
    The completion that is selected out of all the candidates fails the most number of clauses in that theory.
    :param incomplete_pa: An incomplete partial assignment that is needed to be completed
    :param theory: A theory
    :param missing_attributes:
    :return:
    """
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
            i_p, mistle_pos_theory, missing_attributes
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
            i_n, mistle_neg_theory, missing_attributes
        )
        if set(predicted_c_n) == set(c_n):
            accuracy += 1

    return accuracy / (len(complete_positives) + len(complete_negatives))


def plot_uci_nb_missing(dataset, M_list, dl, version, minsup=None, k=None):
    complete_positives, complete_negatives = load_complete_dataset(dataset)
    out_path = "Output/CNFs/" + dataset
    mistle_accuracy_list = []
    randomized_accuracy_list = []
    for m in M_list:
        (
            incomplete_positives,
            incomplete_negatives,
            missing_positives,
            missing_negatives,
        ) = get_uci_nb_missing(dataset, m)

        mistle_pos_theory, _ = Mistle(incomplete_negatives, incomplete_positives).learn(
            dl_measure=dl, minsup=minsup, k=k
        )
        mistle_neg_theory, _ = Mistle(incomplete_positives, incomplete_negatives).learn(
            dl_measure=dl, minsup=minsup, k=k
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
        randomized_accuracy_list.append(1 / (2 ** m))

    plt.figure()
    plt.xlabel("Number of missing literals / row")
    plt.ylabel("Completion Accuracy")

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Random Completion")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    mplcyberpunk.add_underglow()
    plt.savefig(
        "Experiments/exp2_uci_nb_missing_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


# ###############################################################################
# # Plot 1: UCI: Breast
# ###############################################################################
#
# plot_uci_nb_missing(
#     dataset="breast", M_list=[1, 2, 3, 4, 5, 6], minsup=0.05, dl="me", version=1
# )

###############################################################################
# Plot 2: UCI: TicTacToe
###############################################################################

plot_uci_nb_missing(
    dataset="tictactoe", M_list=[1, 2, 3, 4, 5], minsup=0.2, dl="me", version=1
)

# ###############################################################################
# # Plot 3: UCI: Pima
# ###############################################################################
#
# plot_uci_nb_missing(dataset="pima", M_list=[1, 2, 3, 4, 5], k=2000, dl="me", version=1)
#
#
# ###############################################################################
# # Plot 4: UCI: Ionosphere
# ###############################################################################
#
# plot_uci_nb_missing(
#     dataset="ionosphere", M_list=[1, 2, 3, 4, 5], k=2000, dl="me", version=1
# )
