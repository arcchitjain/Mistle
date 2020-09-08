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
matplotlib.rc("legend", fontsize=18)
matplotlib.rc("figure", titlesize=22)

seed = 0
random.seed(seed)
os.chdir("..")


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


def split_data2(data, train_pct=0.7, seed=0):
    random.seed(seed)

    train_data = []
    test_data = []

    for datapoint in data:
        if random.random() < train_pct:
            train_data.append(datapoint)
        else:
            test_data.append(datapoint)

    return train_data, test_data


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
            n = check_pa_satisfiability(completion, mistle_neg_theory.clauses)
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
            n = check_pa_satisfiability(completion, mistle_neg_theory.clauses)
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
    # What %age of the completions of the test examples are satisfiable
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

    accuracy = 0

    for c_p in complete_test_positives:
        if check_pa_satisfiability(c_p, mistle_neg_theory.clauses):
            accuracy += 1

    for c_n in complete_test_negatives:
        if check_pa_satisfiability(c_n, mistle_pos_theory.clauses):
            accuracy += 1

    print(
        "Mistle Accuracy = "
        + str(accuracy)
        + "/"
        + str(len(complete_test_positives) + len(complete_test_negatives))
        + " = "
        + str(accuracy / (len(complete_test_positives) + len(complete_test_negatives)))
    )

    return accuracy / (len(complete_test_positives) + len(complete_test_negatives))


def get_cnfalgo_accuracy2(
    cnfalgo_theory, complete_test_positives, complete_test_negatives, pos_literal=1000
):
    # How many of the completed test examples are satisfiable
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


def plot_uci_nb_missing_split(
    dataset,
    M_list,
    dl,
    version,
    minsup=None,
    k=None,
    sample_vars=None,
    sample_rows=None,
):

    complete_positives, complete_negatives = load_complete_dataset(
        dataset, sample=sample_vars
    )

    if sample_rows is not None:
        complete_positives = random.sample(complete_positives, sample_rows)
        complete_negatives = random.sample(complete_negatives, sample_rows)

    num_folds = 10

    split_positives = split_data(complete_positives, num_folds, seed=seed)
    split_negatives = split_data(complete_negatives, num_folds, seed=seed)

    cnfalgo_accuracy_list = []
    cnfalgo_clauses_list = []
    cnfalgo_literals_list = []

    mistle_clauses_list = []
    mistle_literals_list = []
    mistle_accuracy_list1 = []
    mistle_accuracy_list2 = []
    mistle_accuracy_list3 = []
    mistle_accuracy_list4 = []
    mistle_total_tests = []
    randomized_accuracy_list = []

    for m in M_list:

        fold_cnfalgo_accuracy_list = []
        fold_cnfalgo_clauses_list = []
        fold_cnfalgo_literals_list = []
        fold_mistle_clauses_list = []
        fold_mistle_literals_list = []
        fold_mistle_accuracy_list1 = []
        fold_mistle_accuracy_list2 = []
        fold_mistle_accuracy_list3 = []
        fold_mistle_accuracy_list4 = []
        fold_mistle_total_tests = []
        fold_randomized_accuracy_list = []

        for fold in range(num_folds):

            complete_train_positives = []
            complete_train_negatives = []
            complete_test_positives = []
            complete_test_negatives = []

            for i in range(num_folds):
                if i == fold:
                    complete_test_positives = split_positives[i]
                    complete_test_negatives = split_negatives[i]
                else:
                    complete_train_positives += split_positives[i]
                    complete_train_negatives += split_negatives[i]

            nb_literls = 0
            for p_in in complete_train_positives + complete_train_negatives:
                nb_literls += len(p_in)
            train_data_literals_list = [nb_literls] * len(M_list)

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

            fold_mistle_clauses_list.append(nb_clauses)

            mistle_nb_literals = 0
            for clause in clauses:
                mistle_nb_literals += len(clause)
            fold_mistle_literals_list.append(mistle_nb_literals)

            a1, a2, a3, a4, total = get_pct_satisfiable_completions2(
                mistle_pos_theory,
                mistle_neg_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )
            fold_mistle_accuracy_list1.append(a1)
            fold_mistle_accuracy_list2.append(a2)
            fold_mistle_accuracy_list3.append(a3)
            fold_mistle_accuracy_list4.append(a4)
            fold_mistle_total_tests.append(total)

            cnfalgo_theory = get_cnfalgo_theory(
                complete_train_positives, complete_train_negatives
            )

            cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = get_cnfalgo_accuracy(
                cnfalgo_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )
            fold_cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
            fold_cnfalgo_literals_list.append(cnfalgo_nb_literals)
            fold_cnfalgo_accuracy_list.append(cnfalgo_accuracy)

            fold_randomized_accuracy_list.append(1 / (2 ** m))

        cnfalgo_accuracy_list.append(sum(fold_cnfalgo_accuracy_list) / num_folds)
        cnfalgo_clauses_list.append(sum(fold_cnfalgo_clauses_list) / num_folds)
        cnfalgo_literals_list.append(sum(fold_cnfalgo_literals_list) / num_folds)

        mistle_accuracy_list1.append(sum(fold_mistle_accuracy_list1) / num_folds)
        mistle_accuracy_list2.append(sum(fold_mistle_accuracy_list2) / num_folds)
        mistle_accuracy_list3.append(sum(fold_mistle_accuracy_list3) / num_folds)
        mistle_accuracy_list4.append(sum(fold_mistle_accuracy_list4) / num_folds)

        mistle_clauses_list.append(sum(fold_mistle_clauses_list) / num_folds)
        mistle_literals_list.append(sum(fold_mistle_literals_list) / num_folds)

        mistle_total_tests.append(sum(fold_mistle_total_tests) / num_folds)
        randomized_accuracy_list.append(sum(fold_randomized_accuracy_list) / num_folds)

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )

    print("mistle_accuracy_list1 = " + str(mistle_accuracy_list1))
    print("mistle_accuracy_list2 = " + str(mistle_accuracy_list2))
    print("mistle_accuracy_list3 = " + str(mistle_accuracy_list3))
    print("mistle_accuracy_list4 = " + str(mistle_accuracy_list4))
    print("mistle_clauses_list = " + str(mistle_clauses_list))
    print("mistle_literals_list = " + str(mistle_literals_list))

    print("cnfalgo_accuracy_list = " + str(cnfalgo_accuracy_list))
    print("cnfalgo_clauses_list = " + str(cnfalgo_clauses_list))
    print("cnfalgo_literals_list = " + str(cnfalgo_literals_list))

    print("train_data_literals_list = " + str(train_data_literals_list))

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("% of completions\ncorrectly satisfiable", multialignment="center")

    # plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle1")
    plt.plot(M_list, mistle_accuracy_list1, marker="o", label="M1")
    plt.plot(M_list, mistle_accuracy_list2, marker="o", label="M2")
    plt.plot(M_list, mistle_accuracy_list3, marker="o", label="M3")
    plt.plot(M_list, mistle_accuracy_list4, marker="o", label="M4")
    plt.plot(M_list, cnfalgo_accuracy_list, marker="o", label="CNF")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Rand")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    # plt.legend(bbox_to_anchor=(1.05, 1), loc="upper left")
    plt.savefig(
        "Experiments/exp2.2_uci_missing_split_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        # bbox_inches="tight",
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
        "Experiments/exp2.2_nb_clauses_" + dataset + "_v" + str(version) + ".pdf",
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
        "Experiments/exp2.2_nb_literals_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )


def plot_uci_nb_missing_split2(
    dataset, M_list, dl, version, minsup=None, k=None, train_pct=0.8, metric=1
):
    # This function does not do 10 fold CV.

    complete_positives, complete_negatives = load_data(dataset)

    complete_train_positives, complete_test_positives = split_data2(
        complete_positives, train_pct
    )
    complete_train_negatives, complete_test_negatives = split_data2(
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
    mistle_accuracy_list1 = []
    mistle_accuracy_list2 = []
    mistle_accuracy_list3 = []
    mistle_accuracy_list4 = []
    mistle_total_tests = []
    randomized_accuracy_list = []

    for m in M_list:
        (incomplete_test_positives, missing_test_positives) = get_missing_data(
            complete_test_positives, m
        )
        (incomplete_test_negatives, missing_test_negatives) = get_missing_data(
            complete_test_negatives, m
        )

        mistle_pos_theory, _ = Mistle(
            complete_train_negatives, complete_train_positives
        ).learn(dl_measure=dl, minsup=minsup, k=k)
        mistle_neg_theory, _ = Mistle(
            complete_train_positives, complete_train_negatives
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

        if metric == 1:
            # mistle_accuracy_list.append(
            #     get_pct_satisfiable_completions(
            #         mistle_pos_theory,
            #         mistle_neg_theory,
            #         complete_test_positives,
            #         complete_test_negatives,
            #         incomplete_test_positives,
            #         incomplete_test_negatives,
            #         missing_test_positives,
            #         missing_test_negatives,
            #     )
            # )
            a1, a2, a3, a4, total = get_pct_satisfiable_completions2(
                mistle_pos_theory,
                mistle_neg_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )
            mistle_accuracy_list1.append(a1)
            mistle_accuracy_list2.append(a2)
            mistle_accuracy_list3.append(a3)
            mistle_accuracy_list4.append(a4)
            mistle_total_tests.append(total)
        elif metric == 2:
            mistle_accuracy_list.append(
                check_test_completions(
                    mistle_pos_theory,
                    mistle_neg_theory,
                    complete_test_positives,
                    complete_test_negatives,
                )
            )

        cnfalgo_theory = get_cnfalgo_theory(
            complete_train_positives, complete_train_negatives
        )

        if metric == 1:
            cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = get_cnfalgo_accuracy(
                cnfalgo_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )
        elif metric == 2:
            cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = get_cnfalgo_accuracy2(
                cnfalgo_theory, complete_test_positives, complete_test_negatives
            )

        cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        randomized_accuracy_list.append(1 / (2 ** m))

    plt.figure()
    plt.xlabel("# of missing literals / row")
    plt.ylabel("% of completions\ncorrectly satisfiable", multialignment="center")

    plt.plot(M_list, mistle_accuracy_list, marker="o", label="Mistle")
    # plt.plot(M_list, mistle_accuracy_list1, marker="o", label="M1")
    # plt.plot(M_list, mistle_accuracy_list2, marker="o", label="M2")
    # plt.plot(M_list, mistle_accuracy_list3, marker="o", label="M3")
    # plt.plot(M_list, mistle_accuracy_list4, marker="o", label="M4")
    plt.plot(M_list, cnfalgo_accuracy_list, marker="o", label="CNF")
    plt.plot(M_list, randomized_accuracy_list, marker="o", label="Rand")

    plt.ylim(bottom=0, top=1)
    plt.legend()
    # plt.legend(bbox_to_anchor=(1.05, 1), loc="upper left")
    plt.savefig(
        "Experiments/exp2.2_uci_missing_split_"
        + dataset
        + "_v"
        + str(version)
        + ".pdf",
        # bbox_inches="tight",
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
        "Experiments/exp2.2_nb_clauses_" + dataset + "_v" + str(version) + ".pdf",
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
        "Experiments/exp2.2_nb_literals_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )

    print("Mistle Accuracy List\t:" + str(mistle_accuracy_list))
    # print("Mistle Accuracy List1\t:" + str(mistle_accuracy_list1))
    # print("Mistle Accuracy List2\t:" + str(mistle_accuracy_list2))
    # print("Mistle Accuracy List3\t:" + str(mistle_accuracy_list3))
    # print("Mistle Accuracy List4\t:" + str(mistle_accuracy_list4))
    # print("Mistle Total Tests\t:" + str(mistle_total_tests))
    print("Mistle Clauses List\t\t:" + str(mistle_clauses_list))
    print("Mistle Literals List\t:" + str(mistle_literals_list))

    print("CNF-cc Accuracy List\t:" + str(cnfalgo_accuracy_list))
    print("CNF-cc Clauses List\t\t:" + str(cnfalgo_clauses_list))
    print("CNF-cc Literals List\t:" + str(cnfalgo_literals_list))


# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5, 7],
#     k=20000,
#     dl="me",
#     version=1,
#     train_pct=0.7,
#     sample_vars=15,
#     sample_rows=None,
#     metric=1,
# )

# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5],
#     k=20000,
#     dl="me",
#     version=2,
#     train_pct=0.7,
#     sample_vars=10,
#     sample_rows=None,
#     metric=1,
# )

# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5],
#     k=20000,
#     dl="me",
#     version=3,
#     train_pct=0.7,
#     sample_vars=10,
#     sample_rows=None,
#     metric=2,
# )

# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5, 7],
#     k=20000,
#     dl="me",
#     version=5,
#     train_pct=0.7,
#     sample_vars=10,
#     sample_rows=None,
#     metric=1,
# )

# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5, 7],
#     k=20000,
#     dl="me",
#     version=6,
#     train_pct=0.7,
#     sample_vars=10,
#     sample_rows=None,
#     metric=1,
# )


# Sampled Variables		:[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27]
# mistle_accuracy_list1 = [0.34392969519272765, 0.34749624184581107, 0.31326801328644416, 0.3354382353420065, 0.3335121565863027]
# mistle_accuracy_list2 = [0.3218787093136355, 0.2998773643498877, 0.3530345490514628, 0.3055932436906864, 0.35433303693664125]
# mistle_accuracy_list3 = [0.3399682577065785, 0.3841794963001732, 0.29655790277751143, 0.32011409207771674, 0.33013892346099444]
# mistle_accuracy_list4 = [0.3179172718274863, 0.33656061880424976, 0.3363244385425301, 0.29026910042639675, 0.35095980381133296]
# mistle_clauses_list = [24.0, 50.0, 19.0, 31.5, 26.5]
# mistle_literals_list = [434.0, 1149.5, 359.5, 660.0, 514.5]
# cnfalgo_accuracy_list = [0.00010006214190074515, 0.00013328703151351882, 4.5466557235409665e-05, 0.000165764207916866, 0.00011709102751318702]
# cnfalgo_clauses_list = [284550.0, 286534.0, 280614.0, 292950.0, 297910.0]
# cnfalgo_literals_list = [1345519.0, 1354823.0, 1326615.0, 1385711.0, 1410615.0]
# train_data_literals_list = [array([23279.4, 23279.4, 23279.4, 23279.4, 23279.4])]
# randomized_accuracy_list = [array([0.33418195, 0.20357312, 0.10831783, 0.05446911, 0.02636418])]
# plot_uci_nb_missing_split(
#     dataset="tictactoe",
#     M_list=[1, 3, 5, 7],
#     minsup=1,
#     # k=10000,
#     dl="me",
#     version=9,
#     sample_vars=12,
#     sample_rows=None,
# )

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
}

plot_uci_nb_missing_split2(dataset="ticTacToe.dat", version=10)
