from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
from cnfalgo_wrapper import *
import random
import os
import matplotlib
import matplotlib.pyplot as plt
from collections import defaultdict

# plt.style.use("cyberpunk")
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


def load_complete_dataset(dataset, sample=None):
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

    if sample is None:
        sampled_var_range = var_range
    else:
        sampled_var_range = random.sample(var_range, sample)

    for positive_pa in positives:
        complete_pa = set()
        for var in sampled_var_range:
            if var in positive_pa:
                complete_pa.add(var)
            else:
                complete_pa.add(-var)
        complete_positives.append(frozenset(complete_pa))

    for negative_pa in negatives:
        complete_pa = set()
        for var in sampled_var_range:
            if var in negative_pa:
                complete_pa.add(var)
            else:
                complete_pa.add(-var)
        complete_negatives.append(frozenset(complete_pa))

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


def impute_missing_values(
    incomplete_pa,
    max_theory,
    min_theory,
    missing_attributes,
    metric="count",
    nb_clauses=None,
):
    """
    A function that predicts the most suitable completion of 'incomplete_pa' wrt a theory.
    The completion is selected out of all the candidates and it fails the most number of clauses in that theory.
    If more than one completion fail the most number of clauses in that theory, then one of them is selected at random.

    :param incomplete_pa: An incomplete partial assignment that is needed to be completed
    :param theory: A theory
    :param missing_attributes:
    :return: complete_pa
    """
    completed_pas = get_all_completions_recursive(
        list(incomplete_pa), missing_attributes, []
    )

    if metric == "count":
        max_indices = []
        max_value = -1
        for i, completed_pa in enumerate(completed_pas):
            nb_failed_clauses = 0
            for clause in get_clauses(max_theory):
                if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                    nb_failed_clauses += 1

            if nb_failed_clauses > max_value:
                max_indices = [i]
                max_value = nb_failed_clauses
            elif nb_failed_clauses == max_value:
                max_indices.append(i)
        if len(max_indices) == 1:
            return completed_pas[max_indices[0]]
        else:
            min_indices = []
            min_value = 1000
            for i, completed_pa in enumerate(completed_pas):
                nb_failed_clauses = 0
                for clause in get_clauses(min_theory):
                    if solve([tuple(clause)] + [(a,) for a in completed_pa]) != "UNSAT":
                        nb_failed_clauses += 1

                if nb_failed_clauses < min_value:
                    min_indices = [i]
                    min_value = nb_failed_clauses
                elif nb_failed_clauses == min_value:
                    min_indices.append(i)
        return completed_pas[random.choice(max_indices)]

    elif metric == "length":

        # Calculate number of clauses in the theory for different lengths
        nb_clauses = defaultdict(int)

        for clause in get_clauses(max_theory):
            nb_clauses[len(clause)] += 1

        # For each completion of the given example, calculate number of failing clauses of different lengths
        max_indices = []
        max_score = 0
        for i, completed_pa in enumerate(completed_pas):
            nb_failing_clauses = defaultdict(int)
            for clause in get_clauses(max_theory):
                if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                    nb_failing_clauses[len(clause)] += 1

            # Referred from Page 3 of 'Mining predictive kCNF expressions - Anton, Luc, Siegfried
            length_metric = 0.0
            for length, nb_clause in nb_clauses.items():
                if length in nb_failing_clauses:
                    length_metric += nb_failing_clauses[length] / (length * nb_clause)

            if length_metric > max_score:
                max_indices = [i]
                max_score = length_metric
            elif length_metric == max_score:
                max_indices.append(i)

        if len(max_indices) == 1:
            return completed_pas[max_indices[0]]
        else:
            # Calculate number of clauses in the theory for different lengths
            nb_clauses = defaultdict(int)

            for clause in get_clauses(min_theory):
                nb_clauses[len(clause)] += 1

            # For each completion of the given example, calculate number of failing clauses of different lengths
            min_indices = []
            min_score = 1000
            for i, completed_pa in enumerate(completed_pas):
                nb_failing_clauses = defaultdict(int)
                for clause in get_clauses(min_theory):
                    if solve([tuple(clause)] + [(a,) for a in completed_pa]) != "UNSAT":
                        nb_failing_clauses[len(clause)] += 1

                # Referred from Page 3 of 'Mining predictive kCNF expressions - Anton, Luc, Siegfried
                length_metric = 0.0
                for length, nb_clause in nb_clauses.items():
                    if length in nb_failing_clauses:
                        length_metric += nb_failing_clauses[length] / (
                            length * nb_clause
                        )

                if length_metric < min_score:
                    min_indices = [i]
                    min_score = length_metric
                elif length_metric == min_score:
                    min_indices.append(i)

        return completed_pas[random.choice(min_indices)]


def get_accuracy(
    mistle_pos_theory,
    mistle_neg_theory,
    complete_positives,
    complete_negatives,
    incomplete_positives,
    incomplete_negatives,
    missing_positives,
    missing_negatives,
    metric="count",
):
    if metric == "length":
        # Calculate number of clauses in the +ve theory for different lengths
        nb_clauses_p = defaultdict(int)

        for clause in get_clauses(mistle_pos_theory):
            nb_clauses_p[len(clause)] += 1

        # Calculate number of clauses in the -ve theory for different lengths
        nb_clauses_n = defaultdict(int)

        for clause in get_clauses(mistle_neg_theory):
            nb_clauses_n[len(clause)] += 1
    else:
        nb_clauses_p = None
        nb_clauses_n = None

    accuracy = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        predicted_c_p = impute_missing_values(
            i_p,
            mistle_pos_theory,
            mistle_neg_theory,
            missing_attributes,
            metric,
            nb_clauses_p,
        )

        if set(predicted_c_p) == set(c_p):
            accuracy += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        assert c_n == i_n | m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))

        predicted_c_n = impute_missing_values(
            i_n,
            mistle_neg_theory,
            mistle_pos_theory,
            missing_attributes,
            metric,
            nb_clauses_n,
        )

        if set(predicted_c_n) == set(c_n):
            accuracy += 1

    return accuracy / (len(complete_positives) + len(complete_negatives))


def plot_uci_nb_missing(
    dataset, M_list, dl, version, minsup=None, k=None, metric="length"
):

    complete_positives, complete_negatives = load_complete_dataset(dataset)
    out_path = "Output/CNFs/" + dataset
    cnfalgo_accuracy_list = []
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
            metric,
        )

        nb_clauses, nb_literals, cnfalgo_accuracy = complete_cnfalgo(
            incomplete_positives,
            incomplete_negatives,
            incomplete_positives,
            incomplete_negatives,
            missing_positives,
            missing_negatives,
            metric=metric,
        )
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        mistle_accuracy_list.append(accuracy)
        randomized_accuracy_list.append(1 / (2 ** m))

    plt.figure()
    plt.xlabel("Number of missing literals / row")
    plt.ylabel("Completion Accuracy")

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Random Completion")

    plt.ylim(bottom=0, top=1)
    plt.legend()
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
#     dataset="breast",
#     # M_list=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
#     M_list=[1, 2],
#     minsup=0.05,
#     dl="me",
#     version=1,
#     metric="length",
# )
#
# ###############################################################################
# # Plot 2: UCI: TicTacToe
# ###############################################################################
#
# plot_uci_nb_missing(
#     dataset="tictactoe", M_list=[1, 2, 3, 4, 5], minsup=0.2, dl="me", version=1
# )

###############################################################################
# Plot 3: UCI: Pima
###############################################################################
#
# plot_uci_nb_missing(
#     dataset="pima", M_list=[1, 2, 3, 4, 5], minsup=0.9, dl="me", version=1
# )


# ###############################################################################
# # Plot 4: UCI: Ionosphere
# ###############################################################################
#
# plot_uci_nb_missing(
#     dataset="ionosphere", M_list=[1, 2, 3, 4, 5], k=2000, dl="me", version=1
# )


def split_data(data, train_pct=0.7, seed=0):
    random.seed(seed)

    train_data = []
    test_data = []

    for datapoint in data:
        if random.random() < train_pct:
            train_data.append(datapoint)
        else:
            test_data.append(datapoint)

    return train_data, test_data


def get_missing_data(complete_data, M=0):
    """
    :param complete_data: complete data
    :param M: a positive integer fixating on the exact number of values to be made missing in each row
    :return: a tuple of incomplete positives and incomplete negatives
    """

    incomplete_data = []
    missing_data = []
    for pa in complete_data:
        missing_literals = frozenset(random.sample(pa, M))
        missing_data.append(missing_literals)
        incomplete_datapoint = set()
        for literal in pa:
            if literal not in missing_literals:
                incomplete_datapoint.add(literal)
        incomplete_data.append(frozenset(incomplete_datapoint))

    return (incomplete_data, missing_data)


def plot_uci_nb_missing_split(
    dataset,
    M_list,
    dl,
    version,
    minsup=None,
    k=None,
    metric="length",
    train_pct=0.7,
    sample_vars=None,
    sample_rows=None,
):

    # train_pct = (
    #     0.7
    # )  # Train PCT = 0.7 denotes that 70% of the data will be used for training and the rest for testing.
    # sample_vars = 10
    # sample_rows = None
    complete_positives, complete_negatives = load_complete_dataset(
        dataset, sample=sample_vars
    )

    if sample_rows is not None:
        complete_positives = random.sample(complete_positives, sample_rows)
        complete_negatives = random.sample(complete_negatives, sample_rows)

    complete_train_positives, complete_test_positives = split_data(
        complete_positives, train_pct
    )
    complete_train_negatives, complete_test_negatives = split_data(
        complete_negatives, train_pct
    )

    nb_literls = 0
    for p_in in complete_train_positives + complete_train_negatives:
        nb_literls += len(p_in)
    train_data_literals_list = [nb_literls] * len(M_list)
    cnfalgo_accuracy_list = []
    cnfalgo_clauses_list = []
    cnfalgo_literals_list = []
    mistle_clauses_list = []
    mistle_literals_list = []
    mistle_accuracy_list = []
    randomized_accuracy_list = []

    for m in M_list:
        (incomplete_test_positives, missing_test_positives) = get_missing_data(
            complete_test_positives, m
        )
        (incomplete_test_negatives, missing_test_negatives) = get_missing_data(
            complete_test_negatives, m
        )

        mistle_neg_theory, _ = Mistle(
            complete_train_positives, complete_train_negatives
        ).learn(dl_measure=dl, minsup=minsup, k=k)
        mistle_pos_theory, _ = Mistle(
            complete_train_negatives, complete_train_positives
        ).learn(dl_measure=dl, minsup=minsup, k=k)

        nb_clauses = 0
        clauses = []
        if mistle_pos_theory is not None:
            nb_clauses += len(mistle_pos_theory.clauses)
            clauses += mistle_pos_theory.clauses
        if mistle_neg_theory is not None:
            nb_clauses += len(mistle_neg_theory.clauses)
            clauses += mistle_neg_theory.clauses

        mistle_clauses_list.append(nb_clauses)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        mistle_literals_list.append(mistle_nb_literals)
        mistle_accuracy_list.append(
            get_accuracy(
                mistle_pos_theory,
                mistle_neg_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
                metric,
            )
        )

        cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = complete_cnfalgo(
            complete_train_positives,
            complete_train_negatives,
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
            metric=metric,
        )
        cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        randomized_accuracy_list.append(1 / (2 ** m))

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("Completion Accuracy")

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Random Completion")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_uci_missing_split_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("# of clauses in learned theory")

    plt.plot(M_list, mistle_clauses_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_clauses_list, marker="o", label="CNF-cc")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_nb_clauses_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("# of literals")

    plt.plot(M_list, mistle_literals_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_literals_list, marker="o", label="CNF-cc")
    plt.plot(M_list, train_data_literals_list, marker="o", label="Train Data")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_nb_literals_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )

    print("Mistle Accuracy List\t:" + str(mistle_accuracy_list))
    print("Mistle Clauses List\t\t:" + str(mistle_clauses_list))
    print("Mistle Literals List\t:" + str(mistle_literals_list))

    print("CNF-cc Accuracy List\t:" + str(cnfalgo_accuracy_list))
    print("CNF-cc Clauses List\t\t:" + str(cnfalgo_clauses_list))
    print("CNF-cc Literals List\t:" + str(cnfalgo_literals_list))


# plot_uci_nb_missing_split(
#     dataset="breast",
#     # M_list=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
#     M_list=[1],
#     minsup=0.05,
#     dl="me",
#     version=1,
# )

# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5, 7],
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=7,
#     metric="count",
#     train_pct=0.7,
#     sample_vars=12,
#     sample_rows=None,
# )

plot_uci_nb_missing_split(
    dataset="tictactoe",
    M_list=[1, 3, 5, 7],
    minsup=1,
    # k=20000,
    dl="me",
    version=8,
    metric="length",
    train_pct=0.7,
    sample_vars=12,
    sample_rows=None,
)


def get_artificial_missing(complete_positives, complete_negatives, M=0):
    """
    :param dataset: Name of the uci dataset in string: ["adult", "breast", "chess", "ionoshphere", "mushroom", "pima", "tictactoe"]
    :param M: a positive integer fixating on the exact number of values to be made missing in each row
    :return: a tuple of incomplete positives and incomplete negatives
    """

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


def plot_artificial(M_list, dl, version, minsup=None, k=None, metric="length"):

    dataset = "artificial"
    th = GeneratedTheory([[1, -4], [2, 5], [6, -7, -8], [3, 9, -10]])
    var_range = range(1, 11)
    generator = TheoryNoisyGeneratorOnDataset(th, 400, 0.1)
    positives, negatives = generator.generate_dataset()
    complete_positives = []
    complete_negatives = []

    for positive_pa in positives:
        complete_pa = set(positive_pa)
        for var in var_range:
            if var not in positive_pa:
                complete_pa.add(-var)
        complete_positives.append(frozenset(complete_pa))

    for negative_pa in negatives:
        complete_pa = set(negative_pa)
        for var in var_range:
            if var not in negative_pa:
                complete_pa.add(-var)
        complete_negatives.append(frozenset(complete_pa))

    out_path = "Output/CNFs/" + dataset

    cnfalgo_accuracy_list = []
    cnfalgo_clauses_list = []
    cnfalgo_literals_list = []
    mistle_clauses_list = []
    mistle_literals_list = []
    mistle_accuracy_list = []
    randomized_accuracy_list = []

    for m in M_list:
        (
            incomplete_positives,
            incomplete_negatives,
            missing_positives,
            missing_negatives,
        ) = get_artificial_missing(complete_positives, complete_negatives, m)

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

        nb_clauses = 0
        clauses = []
        if mistle_pos_theory is not None:
            nb_clauses += len(mistle_pos_theory.clauses)
            clauses += mistle_pos_theory.clauses
        if mistle_neg_theory is not None:
            nb_clauses += len(mistle_neg_theory.clauses)
            clauses += mistle_neg_theory.clauses

        mistle_clauses_list.append(nb_clauses)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        mistle_literals_list.append(mistle_nb_literals)

        mistle_accuracy_list.append(
            get_accuracy(
                mistle_pos_theory,
                mistle_neg_theory,
                complete_positives,
                complete_negatives,
                incomplete_positives,
                incomplete_negatives,
                missing_positives,
                missing_negatives,
                metric,
            )
        )

        cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = complete_cnfalgo(
            complete_positives,
            complete_negatives,
            incomplete_positives,
            incomplete_negatives,
            missing_positives,
            missing_negatives,
            metric=metric,
        )
        cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        randomized_accuracy_list.append(1 / (2 ** m))

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("Completion Accuracy")

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Random Completion")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    plt.savefig(
        "Experiments/exp2_uci_missing_"
        + metric
        + "_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("# of clauses in learned theory")

    plt.plot(M_list, mistle_clauses_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_clauses_list, marker="o", label="CNF-cc")

    plt.legend()
    plt.savefig(
        "Experiments/exp2_nb_clauses_"
        + metric
        + "_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("# of literals")

    plt.plot(M_list, mistle_literals_list, marker="o", label="Mistle")
    plt.plot(M_list, cnfalgo_literals_list, marker="o", label="CNF-cc")

    plt.legend()
    plt.savefig(
        "Experiments/exp2_nb_literals_"
        + metric
        + "_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    print("Mistle Accuracy List\t:" + str(mistle_accuracy_list))
    print("Mistle Clauses List\t\t:" + str(mistle_clauses_list))
    print("Mistle Literals List\t:" + str(mistle_literals_list))

    print("CNF-cc Accuracy List\t:" + str(cnfalgo_accuracy_list))
    print("CNF-cc Clauses List\t\t:" + str(cnfalgo_clauses_list))
    print("CNF-cc Literals List\t:" + str(cnfalgo_literals_list))


from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory

import random

seed = 0
random.seed(seed)
np.random.seed(seed)

# plot_artificial(
#     M_list=[1, 2, 3, 4, 5, 6], minsup=0.05, dl="me", version=2, metric="count"
# )
# plot_artificial(
#     M_list=[1, 2, 3, 4, 5, 6], minsup=0.05, dl="me", version=1, metric="length"
# )
