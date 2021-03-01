from mistle_v2 import *
from cnfalgo_wrapper import *
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
    incomplete_pa,
    max_theory,
    min_theory,
    missing_attributes,
    same_class_freq,
    opposite_class_freq,
    metric="count",
):
    """
    A function that predicts the most suitable completion of 'incomplete_pa' wrt a theory.
    The completion is selected out of all the candidates and it fails the most number of clauses in that theory.
    If more than one completion fail the most number of clauses in that theory, then one of them is selected at random.

    :param incomplete_pa: An incomplete partial assignment that is needed to be completed
    :param missing_attributes:
    :return: complete_pa
    """
    completed_pas = get_all_completions(
        list(incomplete_pa), copy(missing_attributes), []
    )

    if metric == "count":
        # Deprecated for now (Jan 2021)

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
                + " tied among "
                + str(len(max_indices))
                + "/"
                + str(len(completed_pas))
                + " candidates."
                # + "; Max Indices = "
                # + str(max_indices)
                # + "; # failed_clauses_list = "
                # + str(nb_failed_clauses_list)
            )

        return result, 0

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

        # TODO: First order all candidates based on:
        # nb of failed clauses in max theory (desc),
        # nb of failed clauses in min theory (asc.),

        # Calculate number of clauses in the theory for different lengths
        nb_clauses = defaultdict(int)

        for clause in get_clauses(min_theory):
            nb_clauses[len(clause)] += 1

        # For each completion of the given example, calculate number of failing clauses of different lengths
        min_indices = []
        length_metric_list = []
        nb_failed_clauses_list = []
        min_score = 1000
        for i, completed_pa in enumerate(completed_pas):
            nb_failing_clauses = defaultdict(int)
            for clause in get_clauses(min_theory):
                if solve([tuple(clause)] + [(a,) for a in completed_pa]) != "UNSAT":
                    nb_failing_clauses[len(clause)] += 1
            nb_failed_clauses_list.append(nb_failing_clauses)
            # Referred from Page 3 of 'Mining predictive kCNF expressions - Anton, Luc, Siegfried
            length_metric = 0.0
            for length, nb_clause in nb_clauses.items():
                if length in nb_failing_clauses:
                    length_metric += nb_failing_clauses[length] / (length * nb_clause)

            length_metric_list.append(length_metric)
            if length_metric < min_score:
                min_indices = [i]
                min_score = length_metric
            elif length_metric == min_score:
                min_indices.append(i)

        # print(nb_failed_clauses_list)
        print("\n" + str(min_indices))
        print(length_metric_list)
        result1 = set([frozenset(completed_pas[i]) for i in min_indices])

        if len(result1) == 1:
            return result1, 0
        elif len(result1) > 1:
            print(
                "Min Score = "
                + str(min_score)
                + " tied among "
                + str(len(min_indices))
                + "/"
                + str(len(completed_pas))
                + " candidates."
            )

        # Calculate number of clauses in the theory for different lengths
        nb_clauses = defaultdict(int)

        for clause in get_clauses(max_theory):
            nb_clauses[len(clause)] += 1

        # For each completion of the given example, calculate number of failing clauses of different lengths
        max_indices = []
        max_score = -1
        nb_failed_clauses_list = []
        length_metric_list = []
        for i, completed_pa in enumerate(completed_pas):
            if i not in min_indices:
                continue
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

            length_metric_list.append(length_metric)

            if length_metric > max_score:
                max_indices = [i]
                max_score = length_metric
            elif length_metric == max_score:
                max_indices.append(i)

        # print(nb_failed_clauses_list)
        print(max_indices)
        print(length_metric_list)
        result2 = set([frozenset(completed_pas[i]) for i in max_indices])

        if len(result2) == 1:
            print("Tie Breaking: Level 1")
            return result2, 1
        elif len(result2) > 1:
            print(
                "Max Score = "
                + str(max_score)
                + " tied among "
                + str(len(max_indices))
                + "/"
                + str(len(completed_pas))
                + " candidates."
                # + "; Max Indices = "
                # + str(max_indices)
                # + "; # failed_clauses_list = "
                # + str(nb_failed_clauses_list)
            )

        # TODO: Option1: Impute the misisng values using the most frequent value in result2

        missing_attributes_freq1 = defaultdict(int)

        for i, pa in enumerate(result2):
            for literal in pa:
                if abs(literal) in missing_attributes:
                    if literal > 0:
                        missing_attributes_freq1[abs(literal)] += 1
                    else:
                        missing_attributes_freq1[abs(literal)] -= 1

        # print(missing_attributes_freq1)

        predicted_attributes = dict()
        missing_attributes_freq2 = defaultdict(int)

        for attribute, freq in missing_attributes_freq1.items():
            if freq > 0:
                predicted_attributes[attribute] = True
            elif freq < 0:
                predicted_attributes[attribute] = False
            else:
                missing_attributes_freq2[attribute] = 0

        if len(predicted_attributes) == len(missing_attributes):
            print("Tie Breaking: Level 2")
            completed_pa = copy(set(incomplete_pa))
            for attribute, value in predicted_attributes.items():
                if value is True:
                    completed_pa.add(attribute)
                else:
                    completed_pa.add(-attribute)

            return {frozenset(completed_pa)}, 2
        else:
            print(
                "Still need to calculate most frequent values for "
                + str(missing_attributes_freq2.keys())
            )

        # TODO: Option2: Impute the misisng values using the most frequent value in same local class, either P or N

        tie_levels = []
        for attribute, freq in missing_attributes_freq2.items():
            if same_class_freq[attribute] > 0:
                # print("Tie Breaking: Level 3")
                tie_levels.append(3)
                predicted_attributes[attribute] = True
            elif same_class_freq[attribute] < 0:
                # print("Tie Breaking: Level 3")
                tie_levels.append(3)
                predicted_attributes[attribute] = False
            elif opposite_class_freq[attribute] > 0:
                # print("Tie Breaking: Level 4")
                tie_levels.append(4)
                predicted_attributes[attribute] = False
            elif opposite_class_freq[attribute] < 0:
                # print("Tie Breaking: Level 4")
                tie_levels.append(4)
                predicted_attributes[attribute] = True
            else:
                freq = 0
                for pa in result1:
                    if attribute in pa:
                        freq += 1
                    else:
                        freq -= 1

                if freq > 0:
                    # print("Tie Breaking: Level 5")
                    tie_levels.append(5)
                    predicted_attributes[attribute] = True
                elif freq < 0:
                    # print("Tie Breaking: Level 5")
                    tie_levels.append(5)
                    predicted_attributes[attribute] = False
                else:
                    # print("Tie Breaking: Level 6")
                    tie_levels.append(6)
                    predicted_attributes[attribute] = random.choice([True, False])

        assert len(predicted_attributes) == len(missing_attributes)
        completed_pa = copy(set(incomplete_pa))
        for attribute, value in predicted_attributes.items():
            if value is True:
                completed_pa.add(attribute)
            else:
                completed_pa.add(-attribute)

        print(
            "Tie Breaking Levels\t: "
            + str(tie_levels)
            + "; Predicted Values\t: "
            + str(predicted_attributes)
        )
        return {frozenset(completed_pa)}, tie_levels

        # TODO: Option3: Impute the misisng values using the most frequent value in full training data, icluding both P and N


def get_accuracy(
    mistle_pos_theory,
    mistle_neg_theory,
    complete_positives,
    complete_negatives,
    incomplete_positives,
    incomplete_negatives,
    missing_positives,
    missing_negatives,
    pos_freq,
    neg_freq,
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

    mistle_accuracy = 0
    mistle_total = 0
    mf_same_one_accuracy = 0
    mf_same_one_total = 0
    mf_both_one_accuracy = 0
    mf_both_one_total = 0
    nb_ties = 0
    nb_tied_examples = 0
    for c_p, i_p, m_p in zip(
        complete_positives, incomplete_positives, missing_positives
    ):
        if not m_p:
            continue

        assert c_p == i_p | m_p
        missing_attributes = set()
        mf_same_one_c_p = copy(set(i_p))
        mf_both_one_c_p = copy(set(i_p))
        for literal in m_p:
            missing_attributes.add(abs(literal))

            if pos_freq[abs(literal)] > 0:
                mf_same_one_c_p.add(abs(literal))
            elif pos_freq[abs(literal)] < 0:
                mf_same_one_c_p.add(-abs(literal))

            if pos_freq[abs(literal)] - neg_freq[abs(literal)] > 0:
                mf_both_one_c_p.add(abs(literal))
            elif pos_freq[abs(literal)] - neg_freq[abs(literal)] < 0:
                mf_both_one_c_p.add(-abs(literal))

        if len(c_p) == len(mf_same_one_c_p):
            mf_same_one_total += 1
            if c_p == frozenset(mf_same_one_c_p):
                mf_same_one_accuracy += 1

        if len(c_p) == len(mf_both_one_c_p):
            mf_both_one_total += 1
            if c_p == frozenset(mf_both_one_c_p):
                mf_both_one_accuracy += 1

        predicted_c_p, tie_level = impute_missing_values(
            i_p,
            mistle_pos_theory,
            mistle_neg_theory,
            missing_attributes,
            pos_freq,
            neg_freq,
            metric,
        )

        if len(predicted_c_p) > 1:
            nb_ties += len(predicted_c_p)
            nb_tied_examples += 1
        if c_p in predicted_c_p:
            mistle_accuracy += 1
        else:
            print("INCORRECT Prediction at tie levels: " + str(tie_level))

        mistle_total += 1

    for c_n, i_n, m_n in zip(
        complete_negatives, incomplete_negatives, missing_negatives
    ):
        if not m_n:
            continue
        assert c_n == i_n | m_n
        missing_attributes = set()
        mf_same_one_c_n = copy(set(i_n))
        mf_both_one_c_n = copy(set(i_n))
        for literal in m_n:
            missing_attributes.add(abs(literal))

            if neg_freq[abs(literal)] > 0:
                mf_same_one_c_n.add(abs(literal))
            elif neg_freq[abs(literal)] < 0:
                mf_same_one_c_n.add(-abs(literal))

            if neg_freq[abs(literal)] - pos_freq[abs(literal)] > 0:
                mf_both_one_c_n.add(abs(literal))
            elif neg_freq[abs(literal)] - pos_freq[abs(literal)] < 0:
                mf_both_one_c_n.add(-abs(literal))

        if len(c_n) == len(mf_same_one_c_n):
            mf_same_one_total += 1
            if c_n == frozenset(mf_same_one_c_n):
                mf_same_one_accuracy += 1

        if len(c_n) == len(mf_both_one_c_n):
            mf_both_one_total += 1
            if c_n == frozenset(mf_both_one_c_n):
                mf_both_one_accuracy += 1

        predicted_c_n, tie_level = impute_missing_values(
            i_n,
            mistle_neg_theory,
            mistle_pos_theory,
            missing_attributes,
            neg_freq,
            pos_freq,
            metric,
        )

        if len(predicted_c_n) > 1:
            nb_ties += len(predicted_c_n)
            nb_tied_examples += 1

        if c_n in predicted_c_n:
            mistle_accuracy += 1
        else:
            print("INCORRECT Prediction at tie levels: " + str(tie_level))

        mistle_total += 1

    return (
        mistle_accuracy / mistle_total,
        (
            mf_same_one_accuracy / mf_same_one_total,
            mf_both_one_accuracy / mf_both_one_total,
            mf_same_one_total,
            mf_both_one_total,
        ),
        (nb_tied_examples, nb_ties),
    )


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


def get_freq(examples):
    freq = defaultdict(int)
    for example in examples:
        for literal in example:
            if literal < 0:
                freq[abs(literal)] -= 1
            else:
                freq[abs(literal)] += 1
    return freq


###############################################################################
# Exp 1.9: Impute missing values on the test data
###############################################################################


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


# ################################ Plot 1: Count ################################
# plot_artificial(
#     M_list=[1, 2, 3, 4, 5, 6], minsup=0.05, dl="me", version=2, metric="count"
# )
#
# ############################### Plot 1: Length ################################
# plot_artificial(
#     M_list=[1, 2, 3, 4, 5, 6], minsup=0.05, dl="me", version=1, metric="length"
# )


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
    metric="count",
    train_pct=0.8,
):
    # this is the function that learns a theory without splitting the data and then tries to complete all the training examples.
    # Randomly make some values missing on whole 100% of the data. Learn theory on ‘incomplete’ 100% of data. Try to impute all missing values.

    # complete_positives, complete_negatives = load_complete_dataset(dataset)
    complete_positives, complete_negatives = load_data(dataset, complete=True)

    complete_train_positives, complete_test_positives = split_data_into_train_test(
        complete_positives, train_pct, seed=seed
    )
    complete_train_negatives, complete_test_negatives = split_data_into_train_test(
        complete_negatives, train_pct, seed=seed
    )

    out_path = "Output/CNFs/" + dataset
    cnfalgo_accuracy_list = []
    cnfalgo_nb_ties_list = []
    mf_accuracy_list = []
    mistle_accuracy_list = []
    mistle_nb_ties_list = []
    randomized_accuracy_list = []

    mistle_pos_theory, _ = Mistle(
        complete_train_negatives, complete_train_positives
    ).learn(dl_measure=dl, minsup=minsup, k=k, lossy=True)
    mistle_neg_theory, _ = Mistle(
        complete_train_positives, complete_train_negatives
    ).learn(dl_measure=dl, minsup=minsup, k=k, lossy=True)

    pos_freq = get_freq(complete_train_positives)
    neg_freq = get_freq(complete_train_negatives)

    for m in M_list:
        (
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
        ) = get_uci_nb_missing(complete_test_positives, complete_test_negatives, m)

        mistle_accuracy, mf_accuracy, mistle_nb_ties = get_accuracy(
            mistle_pos_theory,
            mistle_neg_theory,
            complete_test_positives,
            complete_test_negatives,
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
            pos_freq,
            neg_freq,
            metric,
        )
        mistle_accuracy_list.append(mistle_accuracy)
        mf_accuracy_list.append(mf_accuracy)
        mistle_nb_ties_list.append(mistle_nb_ties)

        # nb_clauses, nb_literals, cnfalgo_accuracy, cnfalgo_nb_ties = complete_cnfalgo(
        #     incomplete_test_positives,
        #     incomplete_test_negatives,
        #     incomplete_test_positives,
        #     incomplete_test_negatives,
        #     missing_test_positives,
        #     missing_test_negatives,
        #     metric=metric,
        # )
        # cnfalgo_accuracy_list.append(cnfalgo_accuracy)
        # cnfalgo_nb_ties_list.append(cnfalgo_nb_ties)

        randomized_accuracy_list.append(1 / (2 ** m))

    print("\nDataset\t\t\t\t: " + str(dataset))
    print("Metric\t\t\t\t: " + str(metric))
    print("Minsups\t\t\t\t: " + str(M_list))
    print("MF Accuracy\t\t\t: " + str(mf_accuracy_list))
    print("Mistle Accuracy\t\t: " + str(mistle_accuracy_list))
    print("CNFAlgo Accuracy\t: " + str(cnfalgo_accuracy_list))
    print("Randomized Accuracy\t: " + str(randomized_accuracy_list))
    print(
        "\n# Test Examples\t\t: "
        + str(len(complete_test_positives) + len(complete_test_negatives))
    )
    print("Mistle # Ties\t\t: " + str(mistle_nb_ties_list))
    print("CNFAlgo # Ties\t\t: " + str(cnfalgo_nb_ties_list))


dataset = sys.argv[1]
plot_uci_nb_missing(dataset=dataset, M_list=[3], k=10000, metric="length")


###############################################################################
# Exp 2.1: Impute missing values on the test data
###############################################################################


def plot_exp2_1(
    dataset,
    missingness_parameters,
    dl,
    version,
    minsup=None,
    k=None,
    metric="count",
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

    split_positives = split_data_into_num_folds(
        complete_positives, num_folds, seed=seed
    )
    split_negatives = split_data_into_num_folds(
        complete_negatives, num_folds, seed=seed
    )

    cnfalgo_accuracy_list = []
    cnfalgo_clauses_list = []
    cnfalgo_literals_list = []
    mistle_clauses_list = []
    mistle_literals_list = []
    mistle_accuracy_list = []
    randomized_accuracy_list = []
    train_data_literals_list = []
    po_parameters = []

    for m in missingness_parameters:

        po_parameters.append(1 - m)
        fold_cnfalgo_accuracy_list = []
        fold_cnfalgo_clauses_list = []
        fold_cnfalgo_literals_list = []
        fold_mistle_clauses_list = []
        fold_mistle_literals_list = []
        fold_mistle_accuracy_list = []
        fold_train_data_literals_list = []
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
            fold_train_data_literals_list.append(nb_literls)

            (incomplete_test_positives, missing_test_positives) = get_missing_data_mcar(
                complete_test_positives, m
            )
            (incomplete_test_negatives, missing_test_negatives) = get_missing_data_mcar(
                complete_test_negatives, m
            )

            fra = 0
            total = 0
            for missing_example in missing_test_positives + missing_test_negatives:
                if len(missing_example) != 0:
                    fra += 1 / (2 ** len(missing_example))
                    total += 1

            fold_randomized_accuracy_list.append(fra / total)

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
            fold_mistle_accuracy_list.append(
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
                suppress_output=True,
            )
            fold_cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
            fold_cnfalgo_literals_list.append(cnfalgo_nb_literals)
            fold_cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        cnfalgo_accuracy_list.append(sum(fold_cnfalgo_accuracy_list) / num_folds)
        cnfalgo_clauses_list.append(sum(fold_cnfalgo_clauses_list) / num_folds)
        cnfalgo_literals_list.append(sum(fold_cnfalgo_literals_list) / num_folds)
        mistle_accuracy_list.append(sum(fold_mistle_accuracy_list) / num_folds)
        mistle_clauses_list.append(sum(fold_mistle_clauses_list) / num_folds)
        mistle_literals_list.append(sum(fold_mistle_literals_list) / num_folds)

        train_data_literals_list.append(sum(fold_train_data_literals_list) / num_folds)
        # randomized_accuracy_list.append(1 / (2 ** (m * total_vars)))
        randomized_accuracy_list.append(sum(fold_randomized_accuracy_list) / num_folds)

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )

    print("mistle_accuracy_list = " + str(mistle_accuracy_list))
    print("mistle_clauses_list = " + str(mistle_clauses_list))
    print("mistle_literals_list = " + str(mistle_literals_list))

    print("cnfalgo_accuracy_list = " + str(cnfalgo_accuracy_list))
    print("cnfalgo_clauses_list = " + str(cnfalgo_clauses_list))
    print("cnfalgo_literals_list = " + str(cnfalgo_literals_list))

    print("train_data_literals_list = " + str(train_data_literals_list))
    print("randomized_accuracy_list = " + str(randomized_accuracy_list))

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("Completion Accuracy")

    plt.plot(po_parameters, mistle_accuracy_list, marker="o", label="Mistle")
    plt.plot(po_parameters, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
    plt.plot(
        po_parameters, randomized_accuracy_list, marker="o", label="Random Completion"
    )

    plt.ylim(bottom=0, top=1)
    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_plot1_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("# of clauses in learned theory")

    plt.plot(po_parameters, mistle_clauses_list, marker="o", label="Mistle")
    plt.plot(po_parameters, cnfalgo_clauses_list, marker="o", label="CNF-cc")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_plot2_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("# of literals")

    plt.plot(po_parameters, mistle_literals_list, marker="o", label="Mistle")
    plt.plot(po_parameters, cnfalgo_literals_list, marker="o", label="CNF-cc")
    plt.plot(po_parameters, train_data_literals_list, marker="o", label="Train Data")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.1_plot3_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


# Sampled Variables		:[2, 7, 9, 10, 12, 13, 14, 16, 17, 21, 25, 27]
# mistle_accuracy_list = [0.4575484246274099, 0.3067038951580178, 0.1761600930892958, 0.0946598954285173, 0.046581495907257034]
# mistle_clauses_list = [37.9, 37.9, 37.9, 37.9, 37.9]
# mistle_literals_list = [222.5, 222.5, 222.5, 222.5, 222.5]
# cnfalgo_accuracy_list = [0.39942352006282517, 0.3526399447641536, 0.23788644917720828, 0.11374246268460957, 0.05186113067518916]
# cnfalgo_clauses_list = [6916.4, 6916.4, 6916.4, 6916.4, 6916.4]
# cnfalgo_literals_list = [34028.0, 34028.0, 34028.0, 34028.0, 34028.0]
# train_data_literals_list = [10346.4, 10346.4, 10346.4, 10346.4, 10346.4]
# randomized_accuracy_list = [0.3494154720258318, 0.23018823352345055, 0.13326191157653297, 0.06517439229514303, 0.031845899928442535]
# plot_exp2_1(
#     dataset="tictactoe",
#     missingness_parameters=[0.1, 0.2, 0.3, 0.4, 0.5],
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=10,
#     metric="count",
#     sample_vars=12,
#     sample_rows=None,
# )

###############################################################################
# Exp 2.2: %age of completions of test examples that are corrrectly satisfiable
###############################################################################


def plot_exp2_2(
    dataset, missingness_parameters, dl, version, minsup=None, k=None, sample_rows=None
):

    complete_positives, complete_negatives = load_data(dataset, complete=True)
    dataset = dataset.split(".")[0]

    if sample_rows is not None:
        complete_positives = random.sample(complete_positives, sample_rows)
        complete_negatives = random.sample(complete_negatives, sample_rows)

    num_folds = 10
    split_positives = split_data_into_num_folds(
        complete_positives, num_folds, seed=seed
    )
    split_negatives = split_data_into_num_folds(
        complete_negatives, num_folds, seed=seed
    )

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
    train_data_literals_list = []
    po_parameters = []

    fold_cnfalgo_accuracy_list = np.empty((num_folds, len(missingness_parameters)))
    fold_cnfalgo_clauses_list = np.empty((num_folds, len(missingness_parameters)))
    fold_cnfalgo_literals_list = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_clauses_list = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_literals_list = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_accuracy_list1 = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_accuracy_list2 = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_accuracy_list3 = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_accuracy_list4 = np.empty((num_folds, len(missingness_parameters)))
    fold_mistle_total_tests = np.empty((num_folds, len(missingness_parameters)))
    fold_train_data_literals_list = np.empty((num_folds, len(missingness_parameters)))
    fold_randomized_accuracy_list = np.empty((num_folds, len(missingness_parameters)))

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
        fold_train_data_literals_list[fold][:] = [nb_literls] * len(
            missingness_parameters
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
        fold_mistle_clauses_list[fold][:] = [nb_clauses] * len(missingness_parameters)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        fold_mistle_literals_list[fold][:] = [mistle_nb_literals] * len(
            missingness_parameters
        )

        cnfalgo_theory = get_cnfalgo_theory(
            complete_train_positives, complete_train_negatives, suppress_output=True
        )

        for i, m in enumerate(missingness_parameters):

            (incomplete_test_positives, missing_test_positives) = get_missing_data_mcar(
                complete_test_positives, m
            )
            (incomplete_test_negatives, missing_test_negatives) = get_missing_data_mcar(
                complete_test_negatives, m
            )

            fra = 0
            total = 0
            for missing_example in missing_test_positives + missing_test_negatives:
                if len(missing_example) != 0:
                    fra += 1 / (2 ** len(missing_example))
                    total += 1
            fold_randomized_accuracy_list[fold][i] = fra / total

            a1, a2, a3, a4, total = get_pct_satisfiable_completions(
                mistle_pos_theory,
                mistle_neg_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )
            fold_mistle_accuracy_list1[fold][i] = a1
            fold_mistle_accuracy_list2[fold][i] = a2
            fold_mistle_accuracy_list3[fold][i] = a3
            fold_mistle_accuracy_list4[fold][i] = a4
            fold_mistle_total_tests[fold][i] = total

            cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy, cnfalgo_accuracy3 = get_pct_completions_cnfalgo(
                cnfalgo_theory,
                complete_test_positives,
                complete_test_negatives,
                incomplete_test_positives,
                incomplete_test_negatives,
                missing_test_positives,
                missing_test_negatives,
            )

            fold_cnfalgo_clauses_list[fold][i] = cnfalgo_nb_clauses
            fold_cnfalgo_literals_list[fold][i] = cnfalgo_nb_literals
            fold_cnfalgo_accuracy_list[fold][i] = cnfalgo_accuracy

        # print("fold_mistle_accuracy_list1 = " + str(fold_mistle_accuracy_list1))
        # print("fold_mistle_accuracy_list2 = " + str(fold_mistle_accuracy_list2))
        # print("fold_mistle_accuracy_list3 = " + str(fold_mistle_accuracy_list3))
        # print("fold_mistle_accuracy_list4 = " + str(fold_mistle_accuracy_list4))
        # print("fold_mistle_clauses_list = " + str(fold_mistle_clauses_list))
        # print("fold_mistle_literals_list = " + str(fold_mistle_literals_list))
        # print("fold_cnfalgo_accuracy_list = " + str(fold_cnfalgo_accuracy_list))
        # print("fold_cnfalgo_clauses_list = " + str(fold_cnfalgo_clauses_list))
        # print("fold_cnfalgo_literals_list = " + str(fold_cnfalgo_literals_list))
        # print("fold_train_data_literals_list = " + str(fold_train_data_literals_list))
        # print("fold_randomized_accuracy_list = " + str(fold_randomized_accuracy_list))

    print("\nfold_mistle_accuracy_list1 = " + str(fold_mistle_accuracy_list1))
    print("fold_mistle_accuracy_list2 = " + str(fold_mistle_accuracy_list2))
    print("fold_mistle_accuracy_list3 = " + str(fold_mistle_accuracy_list3))
    print("fold_mistle_accuracy_list4 = " + str(fold_mistle_accuracy_list4))
    print("fold_mistle_clauses_list = " + str(fold_mistle_clauses_list))
    print("fold_mistle_literals_list = " + str(fold_mistle_literals_list))
    print("fold_cnfalgo_accuracy_list = " + str(fold_cnfalgo_accuracy_list))
    print("fold_cnfalgo_clauses_list = " + str(fold_cnfalgo_clauses_list))
    print("fold_cnfalgo_literals_list = " + str(fold_cnfalgo_literals_list))
    print("fold_train_data_literals_list = " + str(fold_train_data_literals_list))
    print("fold_randomized_accuracy_list = " + str(fold_randomized_accuracy_list))

    for i, m in enumerate(missingness_parameters):
        po_parameters.append(1 - m)

        cnfalgo_accuracy_list.append(sum(fold_cnfalgo_accuracy_list[:][i]) / num_folds)
        cnfalgo_clauses_list.append(sum(fold_cnfalgo_clauses_list[:][i]) / num_folds)
        cnfalgo_literals_list.append(sum(fold_cnfalgo_literals_list[:][i]) / num_folds)

        mistle_accuracy_list1.append(sum(fold_mistle_accuracy_list1[:][i]) / num_folds)
        mistle_accuracy_list2.append(sum(fold_mistle_accuracy_list2[:][i]) / num_folds)
        mistle_accuracy_list3.append(sum(fold_mistle_accuracy_list3[:][i]) / num_folds)
        mistle_accuracy_list4.append(sum(fold_mistle_accuracy_list4[:][i]) / num_folds)

        mistle_clauses_list.append(sum(fold_mistle_clauses_list[:][i]) / num_folds)
        mistle_literals_list.append(sum(fold_mistle_literals_list[:][i]) / num_folds)

        mistle_total_tests.append(sum(fold_mistle_total_tests[:][i]) / num_folds)

    train_data_literals_list.append(sum(fold_train_data_literals_list) / num_folds)
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
    print("randomized_accuracy_list = " + str(randomized_accuracy_list))

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("Completion Accuracy")

    plt.plot(po_parameters, mistle_accuracy_list1, marker="o", label="M1")
    plt.plot(po_parameters, mistle_accuracy_list2, marker="o", label="M2")
    plt.plot(po_parameters, mistle_accuracy_list3, marker="o", label="M3")
    plt.plot(po_parameters, mistle_accuracy_list4, marker="o", label="M4")
    plt.plot(po_parameters, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
    plt.plot(
        po_parameters, randomized_accuracy_list, marker="o", label="Random Completion"
    )

    plt.ylim(bottom=0, top=1)
    plt.legend()
    plt.savefig(
        "Experiments/exp2.2_plot1_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("# of clauses in learned theory")

    plt.plot(po_parameters, mistle_clauses_list, marker="o", label="Mistle")
    plt.plot(po_parameters, cnfalgo_clauses_list, marker="o", label="CNF-cc")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.2_plot2_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.xlabel("Partial Observability Parameter")
    plt.ylabel("# of literals")

    plt.plot(po_parameters, mistle_literals_list, marker="o", label="Mistle")
    plt.plot(po_parameters, cnfalgo_literals_list, marker="o", label="CNF-cc")
    plt.plot(po_parameters, train_data_literals_list, marker="o", label="Train Data")

    plt.legend()
    plt.savefig(
        "Experiments/exp2.2_plot3_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


# Sampled Variables		:[2, 7, 9, 10, 12, 13, 14, 16, 17, 21, 25, 27]
# mistle_accuracy_list1 = [0.7131123396479725, 0.6417820410320259, 0.634364404787649, 0.6070103123512263, 0.6358006847893657]
# mistle_accuracy_list2 = [0.6566931869568483, 0.5659245453083045, 0.48214150653394805, 0.40035810301975133, 0.37318775179134056]
# mistle_accuracy_list3 = [0.7083985600104221, 0.6947009283186425, 0.7096297159211238, 0.7420750008995574, 0.7576925217842754]
# mistle_accuracy_list4 = [0.6519794073192979, 0.6188434325949213, 0.5574068176674227, 0.5354227915680825, 0.4950795887862502]
# mistle_clauses_list = [37.9, 37.9, 37.9, 37.9, 37.9]
# mistle_literals_list = [222.5, 222.5, 222.5, 222.5, 222.5]
# cnfalgo_accuracy_list = [0.5144429128901429, 0.4034604312693341, 0.3035038638314072, 0.22273699148413484, 0.1892233525360675]
# cnfalgo_clauses_list = [6916.4, 6916.4, 6916.4, 6916.4, 6916.4]
# cnfalgo_literals_list = [30101.2, 30101.2, 30101.2, 30101.2, 30101.2]
# train_data_literals_list = [10346.4, 10346.4, 10346.4, 10346.4, 10346.4]
# randomized_accuracy_list = [0.35676026507964764, 0.2318938852666804, 0.13733124834369984, 0.06748979068397692, 0.028722446861102684]
# plot_exp2_2(
#     dataset="tictactoe",
#     # missingness_parameters=[0.1, 0.2, 0.3, 0.4, 0.5],
#     missingness_parameters=[0.1],
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=10,
#     metric="count",
#     sample_vars=None,
#     sample_rows=None,
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
# train_data_literals_list = [23279.4, 23279.4, 23279.4, 23279.4, 23279.4]
# randomized_accuracy_list = [0.33418195, 0.20357312, 0.10831783, 0.05446911, 0.02636418]
#
# plot_exp2_2(
#     dataset="tictactoe",
#     missingness_parameters=[0.05, 0.1, 0.15, 0.2, 0.25],
#     # missingness_parameters=[0.05],
#     minsup=0.2,
#     # k=20000,
#     dl="me",
#     version=11,
#     metric="count",
#     sample_vars=None,
#     sample_rows=None,
# )


def plot_exp2_2_without_num_folds(
    dataset,
    missingness_parameters=[0.10],
    train_pct=0.8,
    minsup=None,
    load_complete=False,
):

    complete_positives, complete_negatives = load_data(dataset, complete=load_complete)

    complete_train_positives, complete_test_positives = split_data_into_train_test(
        complete_positives, train_pct, seed=seed
    )
    complete_train_negatives, complete_test_negatives = split_data_into_train_test(
        complete_negatives, train_pct, seed=seed
    )

    if minsup is None:
        # minsup = get_topk_minsup(
        #     complete_train_positives + complete_train_negatives,
        #     current_minsup=0.99,
        #     decrement_factor=5,
        #     suppress_output=False,
        # )
        minsup = minsup_dict[dataset]

    cnfalgo_accuracy_list2 = []
    cnfalgo_accuracy_list3 = []
    cnfalgo_clauses_list = []
    cnfalgo_literals_list = []

    # mistle_clauses_list = []
    # mistle_literals_list = []
    mistle_accuracy_list1 = []
    mistle_accuracy_list2 = []
    mistle_accuracy_list3 = []
    mistle_accuracy_list4 = []

    randomized_accuracy_list = []
    # train_data_literals_list = []

    nb_literls = 0
    for p_in in complete_train_positives + complete_train_negatives:
        nb_literls += len(p_in)
    train_data_literals_list = [nb_literls] * len(missingness_parameters)

    start = time()
    mistle_neg_theory, _ = Mistle(
        complete_train_positives, complete_train_negatives
    ).learn(minsup=minsup)
    mistle_pos_theory, _ = Mistle(
        complete_train_negatives, complete_train_positives
    ).learn(minsup=minsup)
    mistle_runtime = time() - start

    nb_clauses = 0
    clauses = []
    if mistle_pos_theory is not None:
        nb_clauses += len(mistle_pos_theory.clauses)
        clauses += mistle_pos_theory.clauses
        # print("mistle_pos_theory.clauses = " + str(mistle_pos_theory.clauses))
    else:
        print("Mistle_pos_theory is None")
    if mistle_neg_theory is not None:
        nb_clauses += len(mistle_neg_theory.clauses)
        clauses += mistle_neg_theory.clauses
        # print("mistle_neg_theory.clauses = " + str(mistle_neg_theory.clauses))
    else:
        print("Mistle_neg_theory is None")
    mistle_clauses_list = [nb_clauses] * len(missingness_parameters)

    mistle_nb_literals = 0
    for clause in clauses:
        mistle_nb_literals += len(clause)
    mistle_literals_list = [mistle_nb_literals] * len(missingness_parameters)

    start = time()
    cnfalgo_theory = get_cnfalgo_theory(
        complete_train_positives, complete_train_negatives, suppress_output=True
    )
    cnfalgo_runtime = time() - start

    for i, m in enumerate(missingness_parameters):

        print("\nDataset\t: " + dataset + ";\tMissingness Parameter\t: " + str(m))
        (incomplete_test_positives, missing_test_positives) = get_missing_data_mcar(
            complete_test_positives, m
        )
        (incomplete_test_negatives, missing_test_negatives) = get_missing_data_mcar(
            complete_test_negatives, m
        )

        fra = 0
        total = 0
        for missing_example in missing_test_positives + missing_test_negatives:
            if len(missing_example) != 0:
                fra += 1 / (2 ** len(missing_example))
                total += 1
        randomized_accuracy_list.append(fra / total)

        a1, a2, a3, a4, _ = get_pct_satisfiable_completions(
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

        cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy2, cnfalgo_accuracy3 = get_pct_completions_cnfalgo(
            cnfalgo_theory,
            complete_test_positives,
            complete_test_negatives,
            incomplete_test_positives,
            incomplete_test_negatives,
            missing_test_positives,
            missing_test_negatives,
        )

        cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_accuracy_list2.append(cnfalgo_accuracy2)
        cnfalgo_accuracy_list3.append(cnfalgo_accuracy3)

    print("\n\ndataset = " + dataset + " with minsup = " + str(minsup))
    print("missingness_parameters = " + str(missingness_parameters))
    print("mistle_accuracy_list1 = " + str(mistle_accuracy_list1))
    print("mistle_accuracy_list2 = " + str(mistle_accuracy_list2))
    print("mistle_accuracy_list3 = " + str(mistle_accuracy_list3))
    print("mistle_accuracy_list4 = " + str(mistle_accuracy_list4))
    print("mistle_clauses_list = " + str(mistle_clauses_list))
    print("mistle_literals_list = " + str(mistle_literals_list))

    print("cnfalgo_accuracy_list2 = " + str(cnfalgo_accuracy_list2))
    print("cnfalgo_accuracy_list3 = " + str(cnfalgo_accuracy_list3))
    print("cnfalgo_clauses_list = " + str(cnfalgo_clauses_list))
    print("cnfalgo_literals_list = " + str(cnfalgo_literals_list))

    print("train_data_literals_list = " + str(train_data_literals_list))
    print("randomized_accuracy_list = " + str(randomized_accuracy_list))

    print("mistle runtime = " + str(mistle_runtime))
    print("cnfalgo runtime = " + str(cnfalgo_runtime))


# dataset = "ticTacToe.dat"
# missingness_parameters = [0.05]
# mistle_accuracy_list1 = [0.7474093264248705]
# mistle_accuracy_list2 = [0.8199481865284974]
# mistle_accuracy_list3 = [0.49352331606217614]
# mistle_accuracy_list4 = [0.5660621761658031]
# mistle_clauses_list = [51]
# mistle_literals_list = [922]
# cnfalgo_accuracy_list = [0.0012547051442910915]
# cnfalgo_clauses_list = [645884]
# cnfalgo_literals_list = [3058030]
# train_data_literals_list = [20385]
# randomized_accuracy_list = [0.327639751552795]

# missingness_parameters = [0.05, 0.15, 0.25]
# mistle_accuracy_list1 = [0.7097156398104265, 0.6907324757154109, 0.6558425063505504]
# mistle_accuracy_list2 = [0.7535545023696683, 0.6944079810974009, 0.5989839119390347]
# mistle_accuracy_list3 = [0.5355450236966824, 0.6509582567603045, 0.6950465707027943]
# mistle_accuracy_list4 = [0.5793838862559242, 0.6546337621422945, 0.6381879762912785]
# mistle_clauses_list = [51, 51, 51]
# mistle_literals_list = [922, 922, 922]
# cnfalgo_accuracy_list = [0.0, 0.000525003281270508, 0.00029635588107661856]
# cnfalgo_clauses_list = [645884, 645884, 645884]
# cnfalgo_literals_list = [3058030, 3058030, 3058030]
# train_data_literals_list = [20385, 20385, 20385]
# randomized_accuracy_list = [0.33349609375, 0.1101377876243781, 0.02028059251237624]

# dataset = sys.argv[1]
# plot_exp2_2_without_num_folds(dataset=dataset)


###############################################################################
# Exp 2.3: %age of test examples that are corrrectly satisfiable
###############################################################################


def plot_exp2_3(dataset, dl="me", version=1, minsup=None, k=None):

    # complete_positives, complete_negatives = load_complete_dataset(
    #     dataset, sample=sample_vars
    # )
    #
    # if sample_rows is not None:
    #     complete_positives = random.sample(complete_positives, sample_rows)
    #     complete_negatives = random.sample(complete_negatives, sample_rows)

    complete_positives, complete_negatives = load_data(dataset, complete=True)
    num_folds = 10

    split_positives = split_data_into_num_folds(
        complete_positives, num_folds, seed=seed
    )
    split_negatives = split_data_into_num_folds(
        complete_negatives, num_folds, seed=seed
    )

    fold_cnfalgo_accuracy_list = []
    fold_cnfalgo_clauses_list = []
    fold_cnfalgo_literals_list = []

    fold_mistle_clauses_list = []
    fold_mistle_literals_list = []
    fold_mistle_accuracy_list1 = []
    fold_mistle_accuracy_list2 = []
    fold_mistle_accuracy_list3 = []
    fold_mistle_accuracy_list4 = []
    fold_mistle_accuracy_comp_list = []

    fold_train_data_literals_list = []

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
        fold_train_data_literals_list.append(nb_literls)

        mistle_minus_theory, comp_minus = Mistle(
            complete_train_positives, complete_train_negatives
        ).learn(dl_measure=dl, minsup=minsup, k=k)
        mistle_plus_theory, comp_plus = Mistle(
            complete_train_negatives, complete_train_positives
        ).learn(dl_measure=dl, minsup=minsup, k=k)

        a1, a2, a3, a4, total = check_test_completions(
            mistle_minus_theory,
            mistle_plus_theory,
            complete_test_positives,
            complete_test_negatives,
        )
        fold_mistle_accuracy_list1.append(a1)
        fold_mistle_accuracy_list2.append(a2)
        fold_mistle_accuracy_list3.append(a3)
        fold_mistle_accuracy_list4.append(a4)

        nb_clauses = 0
        clauses = []
        if comp_plus > comp_minus:
            nb_clauses += len(mistle_plus_theory.clauses)
            clauses += mistle_plus_theory.clauses
            mistle_comp_accuracy = a4
        else:
            nb_clauses += len(mistle_minus_theory.clauses)
            clauses += mistle_minus_theory.clauses
            mistle_comp_accuracy = a1

        fold_mistle_accuracy_comp_list.append(mistle_comp_accuracy)

        # if mistle_pos_theory is not None:
        #     nb_clauses += len(mistle_pos_theory.clauses)
        #     clauses += mistle_pos_theory.clauses
        # if mistle_neg_theory is not None:
        #     nb_clauses += len(mistle_neg_theory.clauses)
        #     clauses += mistle_neg_theory.clauses

        fold_mistle_clauses_list.append(nb_clauses)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        fold_mistle_literals_list.append(mistle_nb_literals)

        cnfalgo_theory = get_cnfalgo_theory(
            complete_train_positives, complete_train_negatives
        )

        fold_cnfalgo_clauses, fold_cnfalgo_literals, fold_cnfalgo_accuracy = get_pct_test_cnfalgo(
            cnfalgo_theory, complete_test_positives, complete_test_negatives
        )
        fold_cnfalgo_clauses_list.append(fold_cnfalgo_clauses)
        fold_cnfalgo_literals_list.append(fold_cnfalgo_literals)
        fold_cnfalgo_accuracy_list.append(fold_cnfalgo_accuracy)

    train_data_literals = sum(fold_train_data_literals_list) / num_folds

    mistle_clauses = sum(fold_mistle_clauses_list) / num_folds
    mistle_literals = sum(fold_mistle_literals_list) / num_folds

    mistle_accuracy1 = sum(fold_mistle_accuracy_list1) / num_folds
    mistle_accuracy2 = sum(fold_mistle_accuracy_list2) / num_folds
    mistle_accuracy3 = sum(fold_mistle_accuracy_list3) / num_folds
    mistle_accuracy4 = sum(fold_mistle_accuracy_list4) / num_folds
    mistle_comp_accuracy = sum(fold_mistle_accuracy_comp_list) / num_folds

    cnfalgo_clauses = sum(fold_cnfalgo_clauses_list) / num_folds
    cnfalgo_literals = sum(fold_cnfalgo_literals_list) / num_folds
    cnfalgo_accuracy = sum(fold_cnfalgo_accuracy_list) / num_folds

    print(
        "\nSampled Variables\t\t:"
        + str(sorted([abs(x) for x in complete_positives[0]]))
    )

    print("mistle_clauses = " + str(mistle_clauses))
    print("mistle_literals = " + str(mistle_literals))
    print("mistle_accuracy1 = " + str(mistle_accuracy1))
    print("mistle_accuracy2 = " + str(mistle_accuracy2))
    print("mistle_accuracy3 = " + str(mistle_accuracy3))
    print("mistle_accuracy4 = " + str(mistle_accuracy4))
    print("mistle_comp_accuracy = " + str(mistle_comp_accuracy))

    print("\ncnfalgo_accuracy = " + str(cnfalgo_accuracy))
    print("cnfalgo_clauses = " + str(cnfalgo_clauses))
    print("cnfalgo_literals = " + str(cnfalgo_literals))

    print("train_data_literals = " + str(train_data_literals))

    # plt.figure()
    # plt.ylabel("Completion Accuracy")
    #
    # plt.bar(
    #     ["M1", "M2", "M3", "M4", "CNF-cc"],
    #     [
    #         mistle_accuracy1,
    #         mistle_accuracy2,
    #         mistle_accuracy3,
    #         mistle_accuracy4,
    #         cnfalgo_accuracy,
    #     ],
    # )
    # plt.ylim(bottom=0, top=1)
    # plt.savefig(
    #     "Experiments/exp2.3_plot1_" + dataset + "_v" + str(version) + ".pdf",
    #     bbox_inches="tight",
    # )
    # plt.show()
    # plt.close()
    #
    # plt.figure()
    # plt.ylabel("# of clauses in learned theory")
    # plt.bar(["Mistle", "CNF-cc"], [mistle_clauses, cnfalgo_clauses])
    # plt.savefig(
    #     "Experiments/exp2.3_plot2_" + dataset + "_v" + str(version) + ".pdf",
    #     bbox_inches="tight",
    # )
    # plt.show()
    # plt.close()

    # plt.figure()
    # plt.ylabel("# of literals")
    # plt.bar(
    #     ["Mistle", "CNF-cc", "Train Data"],
    #     [mistle_literals, cnfalgo_literals, train_data_literals],
    # )
    # plt.savefig(
    #     "Experiments/exp2.3_plot3_" + dataset + "_v" + str(version) + ".pdf",
    #     bbox_inches="tight",
    # )
    # plt.show()
    # plt.close()


# dataset = sys.argv[1]
# plot_exp2_3(dataset=dataset, k=5000)

# Sampled Variables		:[2, 7, 9, 10, 12, 13, 14, 16, 17, 21, 25, 27]
# mistle_accuracy_list2 = [0.7420315225852726, 0.7420315225852726, 0.7420315225852726, 0.7420315225852726, 0.7420315225852726]
# mistle_accuracy_list2 = [0.8703735849953027, 0.8703735849953027, 0.8703735849953027, 0.8703735849953027, 0.8703735849953027]
# mistle_accuracy_list3 = [0.641900001717907, 0.641900001717907, 0.641900001717907, 0.641900001717907, 0.641900001717907]
# mistle_accuracy_list4 = [0.7702420641279369, 0.7702420641279369, 0.7702420641279369, 0.7702420641279369, 0.7702420641279369]
# mistle_clauses_list = [38.5, 38.5, 38.5, 38.5, 38.5]
# mistle_literals_list = [225.8, 225.8, 225.8, 225.8, 225.8]
# cnfalgo_accuracy_list = [0.8806884650113839, 0.8806884650113839, 0.8806884650113839, 0.8806884650113839, 0.8806884650113839]
# cnfalgo_clauses_list = [6916.4, 6916.4, 6916.4, 6916.4, 6916.4]
# cnfalgo_literals_list = [30101.2, 30101.2, 30101.2, 30101.2, 30101.2]
# train_data_literals_list = [10346.4, 10346.4, 10346.4, 10346.4, 10346.4]

# Sampled Variables		:[2, 7, 9, 10, 12, 13, 14, 16, 17, 21, 25, 27]
# mistle_accuracy1 = 0.7420315225852726
# mistle_accuracy2 = 0.8703735849953027
# mistle_accuracy3 = 0.641900001717907
# mistle_accuracy4 = 0.7702420641279369
# mistle_clauses = 38.5
# mistle_literals = 225.8
# cnfalgo_accuracy = 0.8806884650113839
# cnfalgo_clauses = 6916.4
# cnfalgo_literals = 30101.2
# train_data_literals = 10346.4
# plot_exp2_3(
#     dataset="tictactoe",
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=10,
#     sample_vars=12,
#     sample_rows=None,
# )

###############################################################################
# Exp 2.4: Dont complete dataset; %age of test examples that are corrrectly satisfiable
###############################################################################


def plot_exp2_4(dataset, dl, version, minsup=None, k=None):
    positives, negatives = globals()["load_" + dataset]()

    num_folds = 10
    split_positives = split_data_into_num_folds(positives, num_folds)
    split_negatives = split_data_into_num_folds(negatives, num_folds)

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

    train_data_literals_list = []

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

        nb_literls = 0
        for p_in in train_positives + train_negatives:
            nb_literls += len(p_in)
        train_data_literals_list.append(nb_literls)

        mistle_neg_theory, _ = Mistle(train_positives, train_negatives).learn(
            dl_measure=dl, minsup=minsup, k=k
        )
        mistle_pos_theory, _ = Mistle(train_negatives, train_positives).learn(
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

        mistle_clauses_list.append(nb_clauses)

        mistle_nb_literals = 0
        for clause in clauses:
            mistle_nb_literals += len(clause)
        mistle_literals_list.append(mistle_nb_literals)

        a1, a2, a3, a4, total = check_test_completions(
            mistle_pos_theory, mistle_neg_theory, test_positives, test_negatives
        )
        mistle_total_tests.append(total)

        cnfalgo_theory = get_cnfalgo_theory(train_positives, train_negatives)

        cnfalgo_nb_clauses, cnfalgo_nb_literals, cnfalgo_accuracy = get_pct_test_cnfalgo(
            cnfalgo_theory, test_positives, test_negatives
        )

        cnfalgo_clauses_list.append(cnfalgo_nb_clauses)
        cnfalgo_literals_list.append(cnfalgo_nb_literals)
        cnfalgo_accuracy_list.append(cnfalgo_accuracy)

        mistle_accuracy_list1.append(a1)
        mistle_accuracy_list2.append(a2)
        mistle_accuracy_list3.append(a3)
        mistle_accuracy_list4.append(a4)

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

    mistle_clauses = sum(mistle_clauses_list) / num_folds
    mistle_literals = sum(mistle_literals_list) / num_folds

    mistle_accuracy1 = sum(mistle_accuracy_list1) / num_folds
    mistle_accuracy2 = sum(mistle_accuracy_list2) / num_folds
    mistle_accuracy3 = sum(mistle_accuracy_list3) / num_folds
    mistle_accuracy4 = sum(mistle_accuracy_list4) / num_folds

    cnfalgo_clauses = sum(cnfalgo_clauses_list) / num_folds
    cnfalgo_literals = sum(cnfalgo_literals_list) / num_folds
    cnfalgo_accuracy = sum(cnfalgo_accuracy_list) / num_folds

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
        "Experiments/exp2.4_plot1_" + dataset + "_v" + str(version) + ".pdf",
        # bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.title(dataset)
    plt.ylabel("# of clauses in learned theory")

    plt.bar(["Mistle", "CNF-cc"], [mistle_clauses, cnfalgo_clauses])
    plt.savefig(
        "Experiments/exp2.4_plot2_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()

    plt.figure()
    plt.title(dataset)
    plt.ylabel("# of literals")

    plt.bar(["Mistle", "CNF-cc"], [mistle_literals, cnfalgo_literals])

    # plt.legend()
    plt.savefig(
        "Experiments/exp2.4_plot3_" + dataset + "_v" + str(version) + ".pdf",
        bbox_inches="tight",
    )
    plt.show()
    plt.close()


# mistle_accuracy_list1 = [0.970873786407767, 0.8902439024390244, 0.872093023255814, 0.8762886597938144, 0.9444444444444444, 0.8602150537634409, 0.88, 0.8255813953488372, 0.8631578947368421, 0.8888888888888888]
# mistle_accuracy_list2 = [0.9805825242718447, 0.9878048780487805, 0.9534883720930233, 0.979381443298969, 1.0, 0.967741935483871, 0.99, 0.9534883720930233, 0.9473684210526315, 0.9814814814814815]
# mistle_accuracy_list3 = [0.9902912621359223, 0.9024390243902439, 0.9186046511627907, 0.8969072164948454, 0.9444444444444444, 0.8924731182795699, 0.89, 0.872093023255814, 0.9157894736842105, 0.9074074074074074]
# mistle_accuracy_list4 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
# mistle_clauses_list = [20, 45, 43, 41, 36, 36, 33, 33, 36, 39]
# mistle_literals_list = [70, 166, 163, 153, 131, 135, 123, 134, 138, 146]
# cnfalgo_accuracy_list = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
# cnfalgo_clauses_list = [22700, 22860, 22405, 23338, 23640, 23093, 22693, 22395, 23123, 23022]
# cnfalgo_literals_list = [103467, 104200, 102091, 106431, 107960, 105294, 103426, 102132, 105434, 105074]
# train_data_literals_list = [7695, 7884, 7848, 7749, 7650, 7785, 7722, 7848, 7767, 7650]
# plot_exp2_4(
#     dataset="tictactoe",
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=10,
# )


# mistle_accuracy_list1 = [0.3037974683544304, 0.3230769230769231, 0.375, 0.3466666666666667, 0.34146341463414637, 0.4050632911392405, 0.37349397590361444, 0.3088235294117647, 0.6046511627906976, 0.3563218390804598]
# mistle_accuracy_list2 = [0.8987341772151899, 0.8307692307692308, 0.96875, 0.9066666666666666, 0.926829268292683, 0.9367088607594937, 0.8433734939759037, 0.9264705882352942, 0.6511627906976745, 0.8160919540229885]
# mistle_accuracy_list3 = [0.1518987341772152, 0.2153846153846154, 0.125, 0.10666666666666667, 0.18292682926829268, 0.12658227848101267, 0.26506024096385544, 0.11764705882352941, 0.627906976744186, 0.25287356321839083]
# mistle_accuracy_list4 = [0.7468354430379747, 0.7230769230769231, 0.71875, 0.6666666666666666, 0.7682926829268293, 0.6582278481012658, 0.7349397590361446, 0.7352941176470589, 0.6744186046511628, 0.7126436781609196]
# mistle_clauses_list = [32, 42, 34, 41, 31, 33, 38, 40, 42, 41]
# mistle_literals_list = [135, 181, 138, 175, 132, 133, 163, 168, 154, 169]
# cnfalgo_accuracy_list = [0.8354430379746836, 0.8153846153846154, 0.90625, 0.9066666666666666, 0.8536585365853658, 0.8987341772151899, 0.7951807228915663, 0.9117647058823529, 0.7441860465116279, 0.8390804597701149]
# cnfalgo_clauses_list = [4874, 5493, 5737, 5990, 5751, 5662, 5469, 5251, 5001, 5713]
# cnfalgo_literals_list = [17116, 19700, 20655, 21691, 20810, 20332, 19707, 18744, 17566, 20504]
# train_data_literals_list = [5512, 5624, 5632, 5544, 5488, 5512, 5480, 5600, 5456, 5448]
# plot_exp2_4(
#     dataset="pima",
#     minsup=1,
#     # k=20000,
#     dl="me",
#     version=10,
# )
