from mining4sat_wrapper import run_mining4sat
from mistle_v2 import *
from cnfalgo_wrapper import *
import random
import os
import matplotlib
import matplotlib.pyplot as plt
from collections import defaultdict

# plt.style.use("cyberpunk")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=24)
matplotlib.rc("axes", titlesize=22)
matplotlib.rc("axes", labelsize=22)
matplotlib.rc("xtick", labelsize=22)
matplotlib.rc("ytick", labelsize=22)
matplotlib.rc("legend", fontsize=18)
matplotlib.rc("figure", titlesize=22)

seed = 0
random.seed(seed)
os.chdir("..")


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


def split_data(data, num_folds=10, seed=0):
    random.seed(seed)

    split_data = [[] for _ in range(num_folds)]

    for datapoint in data:
        split_data[int(random.random() * num_folds)].append(datapoint)

    return split_data


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


def get_pct_satisfiable_completions(
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
    total = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_p), missing_attributes, [])

        for completion in completed_pas:
            # if not check_pa_satisfiability(completion, mistle_pos_theory.clauses):
            if check_pa_satisfiability(completion, mistle_neg_theory.clauses):
                accuracy += 1
            total += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        assert c_n == i_n | m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_n), missing_attributes, [])

        for completion in completed_pas:
            # if not check_pa_satisfiability(completion, mistle_neg_theory.clauses):
            if check_pa_satisfiability(completion, mistle_pos_theory.clauses):
                accuracy += 1
            total += 1

    print(
        "Mistle Accuracy = "
        + str(accuracy)
        + "/"
        + str(total)
        + " = "
        + str(accuracy / total)
    )
    accuracy = accuracy / total
    return accuracy


def get_pct_satisfiable_completions2(
    mistle_pos_theory,
    mistle_neg_theory,
    complete_positives,
    complete_negatives,
    incomplete_positives,
    incomplete_negatives,
    missing_positives,
    missing_negatives,
):

    accuracy1 = 0
    accuracy2 = 0
    accuracy3 = 0
    accuracy4 = 0
    total = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_p), missing_attributes, [])

        for completion in completed_pas:
            p = check_pa_satisfiability(completion, mistle_pos_theory.clauses)
            n = check_pa_satisfiability(completion, mistle_pos_theory.clauses)
            if not p:
                accuracy1 += 1
                accuracy2 += 1
            if n:
                accuracy3 += 1
                accuracy4 += 1
            total += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        assert c_n == i_n | m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_n), missing_attributes, [])

        for completion in completed_pas:
            p = check_pa_satisfiability(completion, mistle_pos_theory.clauses)
            n = check_pa_satisfiability(completion, mistle_pos_theory.clauses)
            if p:
                accuracy1 += 1
                accuracy3 += 1
            if not n:
                accuracy2 += 1
                accuracy4 += 1
            total += 1

    accuracy1 = accuracy1 / total
    accuracy2 = accuracy2 / total
    accuracy3 = accuracy3 / total
    accuracy4 = accuracy4 / total

    print(
        "Mistle Accuracy = "
        + str(accuracy1)
        + " ; "
        + str(accuracy2)
        + " ; "
        + str(accuracy3)
        + " ; "
        + str(accuracy4)
        + "/"
        + str(total)
    )

    return accuracy1, accuracy2, accuracy3, accuracy4, total


def get_cnfalgo_accuracy(
    cnfalgo_theory,
    complete_test_positives,
    complete_test_negatives,
    incomplete_test_positives,
    incomplete_test_negatives,
    missing_test_positives,
    missing_test_negatives,
    pos_literal=1000,
):
    cnfalgo_nb_clauses = len(cnfalgo_theory)

    modified_cnfalgo_theory = set()
    for clause in cnfalgo_theory:
        modified_cnfalgo_theory.add(clause)

    cnfalgo_nb_literals = 0
    for clause in cnfalgo_theory:
        if pos_literal in clause or -pos_literal in clause:
            cnfalgo_nb_literals += len(clause) - 1
        else:
            cnfalgo_nb_literals += len(clause)

    accuracy = 0
    total = 0
    for c_p, i_p, m_p in zip(
        complete_test_positives, incomplete_test_positives, missing_test_positives
    ):
        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_p), missing_attributes, [])

        for completion in completed_pas:
            completion.append(pos_literal)
            if check_pa_satisfiability(completion, modified_cnfalgo_theory):
                accuracy += 1
            total += 1

    for c_n, i_n, m_n in zip(
        complete_test_negatives, incomplete_test_negatives, missing_test_negatives
    ):
        assert c_n == i_n | m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions_recursive(list(i_n), missing_attributes, [])

        for completion in completed_pas:
            completion.append(-pos_literal)
            if check_pa_satisfiability(completion, modified_cnfalgo_theory):
                accuracy += 1
            total += 1

    print(
        "CNFAlgo Accuracy = "
        + str(accuracy)
        + "/"
        + str(total)
        + " = "
        + str(accuracy / total)
    )
    accuracy = accuracy / total
    return cnfalgo_nb_clauses, cnfalgo_nb_literals, accuracy


def check_test_completions(
    mistle_pos_theory,
    mistle_neg_theory,
    complete_test_positives,
    complete_test_negatives,
):
    accuracy1 = 0
    accuracy2 = 0
    accuracy3 = 0
    accuracy4 = 0
    total = 0

    for c_p in complete_test_positives:
        p = check_pa_satisfiability(c_p, mistle_pos_theory.clauses)
        n = check_pa_satisfiability(c_p, mistle_neg_theory.clauses)
        if not p:
            accuracy1 += 1
            accuracy2 += 1
        if n:
            accuracy3 += 1
            accuracy4 += 1
        total += 1

    for c_n in complete_test_negatives:
        p = check_pa_satisfiability(c_n, mistle_pos_theory.clauses)
        n = check_pa_satisfiability(c_n, mistle_neg_theory.clauses)
        if p:
            accuracy1 += 1
            accuracy3 += 1
        if not n:
            accuracy2 += 1
            accuracy4 += 1
        total += 1

    accuracy1 = accuracy1 / total
    accuracy2 = accuracy2 / total
    accuracy3 = accuracy3 / total
    accuracy4 = accuracy4 / total

    print(
        "Mistle Accuracy = "
        + str(accuracy1)
        + " ; "
        + str(accuracy2)
        + " ; "
        + str(accuracy3)
        + " ; "
        + str(accuracy4)
        + "/"
        + str(total)
    )

    return accuracy1, accuracy2, accuracy3, accuracy4, total


def get_cnfalgo_accuracy2(
    cnfalgo_theory, complete_test_positives, complete_test_negatives, pos_literal=1000
):
    cnfalgo_nb_clauses = len(cnfalgo_theory)

    cnfalgo_nb_literals = 0
    for clause in cnfalgo_theory:
        if pos_literal in clause or -pos_literal in clause:
            cnfalgo_nb_literals += len(clause) - 1
        else:
            cnfalgo_nb_literals += len(clause)

    accuracy = 0

    for c_p in complete_test_positives:
        c_p = list(c_p)
        c_p.append(pos_literal)
        if check_pa_satisfiability(c_p, cnfalgo_theory):
            accuracy += 1

    for c_n in complete_test_negatives:
        c_n = list(c_n)
        c_n.append(-pos_literal)
        if check_pa_satisfiability(c_n, cnfalgo_theory):
            accuracy += 1

    print(
        "CNFAlgo Accuracy = "
        + str(accuracy)
        + "/"
        + str(len(complete_test_positives) + len(complete_test_negatives))
        + " = "
        + str(accuracy / (len(complete_test_positives) + len(complete_test_negatives)))
    )
    accuracy = accuracy / (len(complete_test_positives) + len(complete_test_negatives))
    return cnfalgo_nb_clauses, cnfalgo_nb_literals, accuracy


def plot_uci_nb_missing_split(dataset, dl, version, minsup=None, k=None, num_folds=10):

    positives, negatives = globals()["load_" + dataset]()

    split_positives = split_data(positives, num_folds, seed=seed)
    split_negatives = split_data(negatives, num_folds, seed=seed)

    a1_list = []
    a2_list = []
    a3_list = []
    a4_list = []
    nb_clauses_list = []
    mistle_nb_literals_list = []
    cnfalgo_nb_clauses_list = []
    cnfalgo_nb_literals_list = []
    cnfalgo_fold_accuracy = []

    for fold in range(num_folds):
        train_positives = []
        train_negatives = []
        test_positives = []
        test_negatives = []
        for i in range(num_folds):
            if i == fold:
                test_positives = split_positives[i]
                test_negatives = split_negatives[i]
            else:
                train_positives += split_positives[i]
                train_negatives += split_negatives[i]

        mistle_pos_theory, _ = Mistle(train_negatives, train_positives).learn(
            dl_measure=dl, minsup=minsup, k=k
        )
        mistle_neg_theory, _ = Mistle(train_positives, train_negatives).learn(
            dl_measure=dl, minsup=minsup, k=k
        )

        nb_clauses = 0
        clauses = []
        if mistle_pos_theory is not None:
            nb_clauses += len(mistle_pos_theory.clauses)
            clauses += mistle_pos_theory.clauses
        if mistle_neg_theory is not None:
            nb_clauses += len(mistle_neg_theory.clauses)
            clauses += mistle_neg_theory.clauses
        nb_clauses_list.append(nb_clauses)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        mistle_nb_literals_list.append(mistle_nb_literals)

        a1, a2, a3, a4, total = check_test_completions(
            mistle_pos_theory, mistle_neg_theory, test_positives, test_negatives
        )
        a1_list.append(a1)
        a2_list.append(a2)
        a3_list.append(a3)
        a4_list.append(a4)

        cnfalgo_theory = get_cnfalgo_theory(train_positives, train_negatives)

        cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = get_cnfalgo_accuracy2(
            cnfalgo_theory, test_positives, test_negatives
        )
        cnfalgo_nb_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_nb_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_fold_accuracy.append(cnfalgo_accuracy)

    mistle_clauses = sum(nb_clauses_list) / num_folds
    mistle_literals = sum(mistle_nb_literals_list) / num_folds

    mistle_accuracy1 = sum(a1_list) / num_folds
    mistle_accuracy2 = sum(a2_list) / num_folds
    mistle_accuracy3 = sum(a3_list) / num_folds
    mistle_accuracy4 = sum(a4_list) / num_folds

    cnfalgo_clauses = sum(cnfalgo_nb_clauses_list) / num_folds
    cnfalgo_literals = sum(cnfalgo_nb_literals_list) / num_folds
    cnfalgo_accuracy = sum(cnfalgo_fold_accuracy) / num_folds

    plt.figure()
    plt.title(dataset)
    plt.ylabel("% of test examples\ncorrectly satisfiable", multialignment="center")

    plt.bar(
        ["M1", "M2", "M3", "M4", "CNF-cc"],
        [
            mistle_accuracy1,
            mistle_accuracy2,
            mistle_accuracy3,
            mistle_accuracy4,
            cnfalgo_accuracy,
        ],
    )

    plt.ylim(bottom=0, top=1)
    plt.savefig(
        "Experiments/exp2.4_uci_missing_split_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        # bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.title(dataset)
    plt.ylabel("# of clauses in learned theory")

    # plt.plot([0], mistle_clauses_list, marker="o", label="Mistle")
    # plt.plot([0], cnfalgo_clauses_list, marker="o", label="CNF-cc")
    plt.bar(["Mistle", "CNF-cc"], [mistle_clauses, cnfalgo_clauses])

    # plt.legend()
    plt.savefig(
        "Experiments/exp2.4_nb_clauses_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.title(dataset)
    plt.ylabel("# of literals")

    # plt.plot([0], mistle_literals_list, marker="o", label="Mistle")
    # plt.plot([0], cnfalgo_literals_list, marker="o", label="CNF-cc")
    plt.bar(["Mistle", "CNF-cc"], [mistle_literals, cnfalgo_literals])

    # plt.legend()
    plt.savefig(
        "Experiments/exp2.4_nb_literals_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    print("Mistle Accuracy1\t:" + str(mistle_accuracy1))
    print("Mistle Accuracy2\t:" + str(mistle_accuracy2))
    print("Mistle Accuracy3\t:" + str(mistle_accuracy3))
    print("Mistle Accuracy4\t:" + str(mistle_accuracy4))
    print("Mistle Clauses\t\t:" + str(mistle_clauses))
    print("Mistle Literals\t\t:" + str(mistle_literals))

    print("CNF-cc Accuracy\t\t:" + str(cnfalgo_accuracy))
    print("CNF-cc Clauses\t\t:" + str(cnfalgo_clauses))
    print("CNF-cc Literals\t\t:" + str(cnfalgo_literals))


plot_uci_nb_missing_split(
    dataset="tictactoe",
    minsup=1,
    # k=10000,
    dl="me",
    version=8,
)

# plot_uci_nb_missing_split(
#     dataset="ionosphere",
#     # minsup=1,
#     k=20000,
#     dl="me",
#     version=8,
# )

# plot_uci_nb_missing_split(
#     dataset="pima",
#     minsup=1,
#     # k=10000,
#     dl="me",
#     version=8,
# )
