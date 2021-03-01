from mistle_v2 import *
from cnfalgo_wrapper_aug import complete_cnfalgo_aug
import random
import numpy as np
from time import time
import os
import sys
import matplotlib
import matplotlib.pyplot as plt
from collections import defaultdict
from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory

seed = 1234
random.seed(seed)
np.random.seed(seed)

plt.style.use("seaborn")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=28)
matplotlib.rc("axes", titlesize=28)
matplotlib.rc("axes", labelsize=28)
matplotlib.rc("xtick", labelsize=28)
matplotlib.rc("ytick", labelsize=28)
matplotlib.rc("legend", fontsize=22)
matplotlib.rc("figure", titlesize=28)

###############################################################################
# Helping/Auxilliary Functions
###############################################################################

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


def get_uci_nb_missing(complete_positives, complete_negatives, M=0):
    """
    :param dataset: Name of the uci dataset in string: ["adult", "breast", "chess", "ionoshphere", "mushroom", "pima", "tictactoe"]
    :param M: a positive integer fixating on the exact number of values to be made missing in each row
    :return: a tuple of incomplete positives and incomplete negatives
    """
    # complete_positives, complete_negatives = load_complete_dataset(dataset)
    # complete_positives, complete_negatives = load_data(dataset, complete=True)

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


def get_all_completions(incomplete_pa, missing_attributes, result):
    if not missing_attributes:
        return [incomplete_pa]
    else:
        return get_all_completions_recursive(incomplete_pa, missing_attributes, result)


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
    incomplete_pa, max_theory, min_theory, missing_attributes, metric="count"
):
    """
    A function that predicts the most suitable completion of 'incomplete_pa' wrt a theory.
    The completion is selected out of all the candidates and it fails the most number of clauses in that theory.
    If more than one completion fail the most number of clauses in that theory, then one of them is selected at random.

    :param incomplete_pa: An incomplete partial assignment that is needed to be completed
    :param missing_attributes:
    :return: complete_pa
    """
    completed_pas = get_all_completions(list(incomplete_pa), missing_attributes, [])

    if metric == "count":
        max_indices = []
        max_score = -1
        nb_failed_clauses_list = []
        for i, completed_pa in enumerate(completed_pas):
            nb_failed_clauses = 0
            for clause in get_clauses(max_theory):
                if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                    nb_failed_clauses += 1
            nb_failed_clauses_list.append(nb_failed_clauses)

            if nb_failed_clauses > max_score:
                max_indices = [i]
                max_score = nb_failed_clauses
            elif nb_failed_clauses == max_score:
                max_indices.append(i)

        result = set([frozenset(completed_pas[i]) for i in max_indices])

        if len(result) > 1:
            print(
                "Max Score = "
                + str(max_score)
                + "; Max Indices = "
                + str(max_indices)
                + "; # failed_clauses_list = "
                + str(nb_failed_clauses_list)
            )
            catch = 1
        return result

        # if len(max_indices) == 1:
        #     return completed_pas[max_indices[0]]
        # else:
        #     min_indices = []
        #     min_value = 1000
        #     for i, completed_pa in enumerate(completed_pas):
        #         nb_failed_clauses = 0
        #         for clause in get_clauses(min_theory):
        #             if solve([tuple(clause)] + [(a,) for a in completed_pa]) != "UNSAT":
        #                 nb_failed_clauses += 1
        #
        #         if nb_failed_clauses < min_value:
        #             min_indices = [i]
        #             min_value = nb_failed_clauses
        #         elif nb_failed_clauses == min_value:
        #             min_indices.append(i)
        #     return completed_pas[random.choice(min_indices)]

    elif metric == "length":

        # Calculate number of clauses in the theory for different lengths
        nb_clauses = defaultdict(int)

        for clause in get_clauses(max_theory):
            nb_clauses[len(clause)] += 1

        # For each completion of the given example, calculate number of failing clauses of different lengths
        max_indices = []
        max_score = -1
        nb_failed_clauses_list = []
        for i, completed_pa in enumerate(completed_pas):
            nb_failing_clauses = defaultdict(int)
            for clause in get_clauses(max_theory):
                if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                    nb_failing_clauses[len(clause)] += 1
            nb_failed_clauses_list.append(str(nb_failing_clauses))

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

        result = set([frozenset(completed_pas[i]) for i in max_indices])

        if len(result) > 1:
            print(
                "Max Score = "
                + str(max_score)
                + "; Max Indices = "
                + str(max_indices)
                + "; # failed_clauses_list = "
                + str(nb_failed_clauses_list)
            )
            catch = 1
        return result

        # if len(max_indices) == 1:
        #     return completed_pas[max_indices[0]]
        # else:
        #     # Calculate number of clauses in the theory for different lengths
        #     nb_clauses = defaultdict(int)
        #
        #     for clause in get_clauses(min_theory):
        #         nb_clauses[len(clause)] += 1
        #
        #     # For each completion of the given example, calculate number of failing clauses of different lengths
        #     min_indices = []
        #     min_score = 1000
        #     for i, completed_pa in enumerate(completed_pas):
        #         nb_failing_clauses = defaultdict(int)
        #         for clause in get_clauses(min_theory):
        #             if solve([tuple(clause)] + [(a,) for a in completed_pa]) != "UNSAT":
        #                 nb_failing_clauses[len(clause)] += 1
        #
        #         # Referred from Page 3 of 'Mining predictive kCNF expressions - Anton, Luc, Siegfried
        #         length_metric = 0.0
        #         for length, nb_clause in nb_clauses.items():
        #             if length in nb_failing_clauses:
        #                 length_metric += nb_failing_clauses[length] / (
        #                     length * nb_clause
        #                 )
        #
        #         if length_metric < min_score:
        #             min_indices = [i]
        #             min_score = length_metric
        #         elif length_metric == min_score:
        #             min_indices.append(i)
        #
        # return completed_pas[random.choice(min_indices)]


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
    # if metric == "length":
    #     # Calculate number of clauses in the +ve theory for different lengths
    #     nb_clauses_p = defaultdict(int)
    #
    #     for clause in get_clauses(mistle_pos_theory):
    #         nb_clauses_p[len(clause)] += 1
    #
    #     # Calculate number of clauses in the -ve theory for different lengths
    #     nb_clauses_n = defaultdict(int)
    #
    #     for clause in get_clauses(mistle_neg_theory):
    #         nb_clauses_n[len(clause)] += 1
    # else:
    #     nb_clauses_p = None
    #     nb_clauses_n = None

    accuracy = 0
    total = 0
    nb_ties = 0
    nb_tied_examples = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        if not m_p:
            continue

        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        predicted_c_p = impute_missing_values(
            i_p, mistle_pos_theory, mistle_neg_theory, missing_attributes, metric
        )

        # if set(predicted_c_p) == set(c_p):
        #     accuracy += 1
        if len(predicted_c_p) > 1:
            nb_ties += len(predicted_c_p)
            nb_tied_examples += 1
        if c_p in predicted_c_p:
            accuracy += 1

        total += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        if not m_n:
            continue
        assert c_n == i_n | m_n
        missing_attributes = set()
        for literal in m_n:
            missing_attributes.add(abs(literal))

        predicted_c_n = impute_missing_values(
            i_n, mistle_neg_theory, mistle_pos_theory, missing_attributes, metric
        )

        # if set(predicted_c_n) == set(c_n):
        #     accuracy += 1
        if len(predicted_c_n) > 1:
            nb_ties += len(predicted_c_n)
            nb_tied_examples += 1

        if c_n in predicted_c_n:
            accuracy += 1

        total += 1

    return accuracy / total, (nb_tied_examples, nb_ties)


def split_data_into_train_test(data, train_pct=0.7, seed=1234):
    random.seed(seed)

    train_data = []
    test_data = []

    for datapoint in data:
        if random.random() < train_pct:
            train_data.append(datapoint)
        else:
            test_data.append(datapoint)

    return train_data, test_data


def split_data_into_num_folds(data, num_folds=10, seed=1234):
    random.seed(seed)

    split_data = [[] for _ in range(num_folds)]

    for datapoint in data:
        split_data[int(random.random() * num_folds)].append(datapoint)

    return split_data


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


def get_missing_data_mcar(complete_data, m):
    """
    :param complete_data: complete data
    :param m: a positive integer fixating on the exact number of values to be made missing in each row
    :return: a tuple of incomplete positives and incomplete negatives
    """
    incomplete_data = []
    missing_data = []
    for pa in complete_data:
        noisy_pa = set()
        missing_literals = set()
        for literal in pa:
            if random.random() >= m:
                noisy_pa.add(literal)
            else:
                missing_literals.add(literal)
        missing_data.append(frozenset(missing_literals))
        incomplete_data.append(frozenset(noisy_pa))

    return (incomplete_data, missing_data)


def get_artificial_missing(complete_positives, complete_negatives, M=0):
    """
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
            p = check_pa_satisfiability(
                completion, mistle_pos_theory.clauses, pa_is_complete=True
            )
            n = check_pa_satisfiability(
                completion, mistle_neg_theory.clauses, pa_is_complete=True
            )
            if p is False:
                accuracy1 += 1
                accuracy2 += 1
            if n is True:
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
            p = check_pa_satisfiability(
                completion, mistle_pos_theory.clauses, pa_is_complete=True
            )
            n = check_pa_satisfiability(
                completion, mistle_neg_theory.clauses, pa_is_complete=True
            )
            if p is True:
                accuracy1 += 1
                accuracy3 += 1
            if n is False:
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
        + " / "
        + str(total)
    )

    return accuracy1, accuracy2, accuracy3, accuracy4, total


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


def get_pct_completions_cnfalgo(
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

    pos_cnfalgo_theory = copy(modified_cnfalgo_theory)
    pos_cnfalgo_theory.add(frozenset([pos_literal]))

    neg_cnfalgo_theory = copy(modified_cnfalgo_theory)
    neg_cnfalgo_theory.add(frozenset([-pos_literal]))

    cnfalgo_nb_literals = 0
    for clause in cnfalgo_theory:
        if pos_literal in clause or -pos_literal in clause:
            cnfalgo_nb_literals += len(clause) - 1
        else:
            cnfalgo_nb_literals += len(clause)

    accuracy2 = 0
    accuracy3 = 0
    total = 0
    for c_p, i_p, m_p in zip(
        complete_test_positives, incomplete_test_positives, missing_test_positives
    ):
        assert c_p == i_p | m_p
        missing_attributes = set()
        for literal in m_p:
            missing_attributes.add(abs(literal))

        completed_pas = get_all_completions(list(i_p), missing_attributes, [])

        for completion in completed_pas:
            if check_pa_satisfiability(
                completion, pos_cnfalgo_theory, pa_is_complete=False
            ):
                accuracy3 += 1

            if (
                check_pa_satisfiability(
                    completion, neg_cnfalgo_theory, pa_is_complete=False
                )
                is False
            ):
                accuracy2 += 1
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
            if (
                check_pa_satisfiability(
                    completion, pos_cnfalgo_theory, pa_is_complete=False
                )
                is False
            ):
                accuracy2 += 1

            if check_pa_satisfiability(
                completion, neg_cnfalgo_theory, pa_is_complete=False
            ):
                accuracy3 += 1
            total += 1

    print(
        "CNFAlgo Accuracy 2 = "
        + str(accuracy2)
        + "/"
        + str(total)
        + " = "
        + str(accuracy2 / total)
    )
    print(
        "CNFAlgo Accuracy 3 = "
        + str(accuracy3)
        + "/"
        + str(total)
        + " = "
        + str(accuracy3 / total)
    )
    accuracy2 = accuracy2 / total
    accuracy3 = accuracy3 / total
    return cnfalgo_nb_clauses, cnfalgo_nb_literals, accuracy2, accuracy3


def get_pct_test_cnfalgo(
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


def avlen(l):
    # Returns the average length of elements in the list 'l'
    return sum([len(x) for x in l]) / len(l)


###############################################################################
# Exp 2.0: Split data into train/test; No 10 fold CVl; Impute missing values on the full data
###############################################################################


def plot_uci_nb_missing(
    dataset,
    M_list,
    version=1,
    dl="me",
    minsup=None,
    k=None,
    metric="length",
    train_pct=0.8,
):
    # this is the function that learns a theory without splitting the data and then tries to complete all the training examples.
    # Randomly make some values missing on whole 100% of the data. Learn theory on ‘incomplete’ 100% of data. Try to impute all missing values.

    complete_positives, complete_negatives = load_data(dataset, complete=True)

    complete_train_positives, complete_test_positives = split_data_into_train_test(
        complete_positives, train_pct, seed=seed
    )
    complete_train_negatives, complete_test_negatives = split_data_into_train_test(
        complete_negatives, train_pct, seed=seed
    )

    out_path = "Output/CNFs/" + dataset
    cnfalgo_accuracy_list = []
    cnfalgo_aug_accuracy_list = []
    mistle_accuracy_list = []
    mistle_nb_ties_list = []
    randomized_accuracy_list = []

    mistle_pos_theory, _ = Mistle(
        complete_train_negatives, complete_train_positives
    ).learn(dl_measure=dl, minsup=minsup, k=k)
    mistle_neg_theory, _ = Mistle(
        complete_train_positives, complete_train_negatives
    ).learn(dl_measure=dl, minsup=minsup, k=k)

    for m in M_list:
        (
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
        ) = get_uci_nb_missing(complete_test_positives, complete_test_negatives, m)

        accuracy, mistle_nb_ties = get_accuracy(
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
        mistle_nb_ties_list.append(mistle_nb_ties)

        nb_clauses, nb_literals, cnfalgo_accuracy, cnfalgo_aug_accuracy = complete_cnfalgo_aug(
            incomplete_test_positives,
            incomplete_test_negatives,
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
            mistle_pos_theory,
            mistle_neg_theory,
            metric=metric,
        )
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)
        cnfalgo_aug_accuracy_list.append(cnfalgo_aug_accuracy)

        mistle_accuracy_list.append(accuracy)
        randomized_accuracy_list.append(1 / (2 ** m))

    print("\nDataset\t\t\t\t: " + str(dataset))
    print("Metric\t\t\t\t: " + str(metric))
    print("Minsups\t\t\t\t: " + str(M_list))
    print("Mistle Accuracy\t\t: " + str(mistle_accuracy_list))
    print("CNFAlgo Accuracy\t: " + str(cnfalgo_accuracy_list))
    print("CNFAlgo Aug Accuracy: " + str(cnfalgo_aug_accuracy_list))
    print("Randomized Accuracy\t: " + str(randomized_accuracy_list))
    print(
        "\n# Test Examples\t\t: "
        + str(len(complete_test_positives) + len(complete_test_negatives))
    )
    print("Mistle # Ties\t\t: " + str(mistle_nb_ties_list))


dataset = sys.argv[1]
plot_uci_nb_missing(dataset=dataset, M_list=[3], k=10000, metric="length")
