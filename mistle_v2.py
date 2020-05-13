from tqdm import tqdm
from copy import copy
from pycosat import solve
from time import time
import math
import sys
from collections import Counter
from pattern_mining import compute_itemsets
import numpy as np


def print_input_data_statistics(
    name,
    positive_input_clauses,
    negative_input_clauses,
    target_class,
    negation,
    load_top_k,
    switch_signs,
):
    pos_freq = len(positive_input_clauses)
    neg_freq = len(negative_input_clauses)

    print()
    print("Input Data Statistics:")
    print("\tDataset Name \t\t\t\t\t\t\t\t\t\t\t\t\t\t\t: " + name)
    print(
        "\tAdd absent variables as negative literals in the partial assignments \t: "
        + str(negation)
    )
    if not (bool(load_top_k)):
        print(
            "\tLoad top-k negative partial assignments \t\t\t\t\t\t\t\t: "
            + str(bool(load_top_k))
        )
    else:
        print(
            "\tNumber of negative partial assignments loaded from the top of the file \t: "
            + str(load_top_k)
        )

    inconsistent_clauses = set(positive_input_clauses) & set(negative_input_clauses)
    print(
        "\tNumber of inconsistent clauses found \t\t\t\t\t\t\t\t\t: "
        + str(len(inconsistent_clauses))
    )

    pos_redundancies = len(positive_input_clauses) - len(set(positive_input_clauses))
    print(
        "\tNumber of positive redundancies found \t\t\t\t\t\t\t\t\t: "
        + str(pos_redundancies)
    )

    neg_redundancies = len(negative_input_clauses) - len(set(negative_input_clauses))
    print(
        "\tNumber of negative redundancies found \t\t\t\t\t\t\t\t\t: "
        + str(neg_redundancies)
    )

    print()

    if switch_signs:
        pos_sign = " -ve"
        neg_sign = " +ve"
    else:
        pos_sign = " +ve"
        neg_sign = " -ve"

    print("\tClass\tSign\t%age\tFrequency")
    n = pos_freq + neg_freq
    print(
        "\t"
        + str(target_class[0])
        + "\t\t"
        + pos_sign
        + "\t"
        + str(round(pos_freq * 100 / n))
        + "%\t\t"
        + str(pos_freq)
    )
    print(
        "\t"
        + str(target_class[1])
        + "\t\t"
        + neg_sign
        + "\t"
        + str(round(neg_freq * 100 / n))
        + "%\t\t"
        + str(neg_freq)
    )
    print()


def load_test():
    """
    Illustrative Toy Example used in the Paper
    :return:
    """

    positives = []
    positives.append(frozenset([1, 2]))
    positives.append(frozenset([2, 3]))
    positives.append(frozenset([2, 5]))

    negatives = []
    negatives.append(frozenset([-1, -2, -3, -4, -5]))
    negatives.append(frozenset([-1, -2, -3, -6, -7]))
    negatives.append(frozenset([-1, -3, -4]))
    negatives.append(frozenset([-2, -5, -6]))
    negatives.append(frozenset([-2, -3, -4]))
    negatives.append(frozenset([-2, -3, 4]))
    negatives.append(frozenset([-1, -3, -4]))
    negatives.append(frozenset([-2, 3]))

    return positives, negatives


def load_dtest():
    """
    An Example used to test the efficacy of the Dichotomization Operator.
    :return:
    """

    positives = []  # Dummy positives
    # positives.append(frozenset([1, -2, -3, 8, 9, 10, 11]))
    # positives.append(frozenset([-1, -2, -3, 4, 5, 6, 7]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, -6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, -5, 6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, -5, 6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, -5, -6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, -6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, -5, -6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, 5, 6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, 5, 6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, 5, -6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, -5, 6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, -5, 6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, -5, -6, 7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, 5, -6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, -4, -5, -6, -7, 8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, 9, 10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, 9, -10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, 9, -10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, -9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, -9, 10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, -9, -10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, 8, -9, -10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, 9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, 9, 10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, 9, -10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, 9, -10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, -9, 10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, -9, 10, -11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, -9, -10, 11]))
    positives.append(frozenset([1, -2, -3, 4, 5, 6, 7, -8, -9, -10, -11]))

    negatives = []
    negatives.append(frozenset([-1, -2, -3, -4]))
    negatives.append(frozenset([-1, -2, -3, -5]))
    negatives.append(frozenset([-1, -2, -3, -6]))
    negatives.append(frozenset([-1, -2, -3, -7]))
    negatives.append(frozenset([1, -2, -3, -8]))
    negatives.append(frozenset([1, -2, -3, -9]))
    negatives.append(frozenset([1, -2, -3, -10]))
    negatives.append(frozenset([1, -2, -3, -11]))

    return positives, negatives


def load_animal_taxonomy(switch_signs=False):
    a = 1
    b = 2
    c = 3
    d = 4
    e = 5
    f = 6
    g = 7
    h = 8
    i = 9
    j = 10
    k = 11
    l = 12
    m = 13
    n = 14

    global new_var_counter
    new_var_counter = 15

    p1 = frozenset([a, b, -c, -d, -e, -f, g, -h, i, j])
    p2 = frozenset([a, -b, -c, -d, -e, f, g, -h, i, j])
    p3 = frozenset([a, -b, c, -d, -e, -f, g, -h, i, j, -l, m])
    p4 = frozenset([a, -b, c, -d, -e, -f, g, -h, i, j, l, -m])

    positives = [p1, p2, p3, p4]

    n1 = frozenset([-b, c, -d, -e, -f, g, -h, i, -j, k])
    n2 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, -l, m, n])
    n3 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, l, -m, n])
    n4 = frozenset([b, -c, -d, -e, -f, g, -h, -i, -j, k])
    n5 = frozenset([-b, -c, d, -e, -f, g, -h, i, -j])
    n6 = frozenset([-b, c, -d, -e, -f, g, -h, -i, -j, -k, -l, m])
    n7 = frozenset([-b, -c, -d, e, -f, g, -h, -i, -j, -k, l, -m])

    negatives = [n1, n2, n3, n4, n5, n6, n7]

    print_input_data_statistics(
        "Animal Taxonomy",
        positives,
        negatives,
        target_class=["B", "NB"],
        negation=False,
        load_top_k=False,
        switch_signs=False,
    )
    if switch_signs:
        return negatives, positives
    else:
        return positives, negatives


def load_dataset(
    filename,
    total,
    var_range,
    target_class,
    negation=False,
    load_top_k=None,
    switch_signs=False,
    num_vars=0,
    load_tqdm=True,
    print_stats=False,
    raw_data=False,
):
    global new_var_counter
    new_var_counter = num_vars + 1

    f = open("./Data/" + filename, "r")
    name = filename.split(".")[0].split("_")[0]
    name = name[0].upper() + name[1:]

    # positive_input_clauses = set()
    # negative_input_clauses = set()

    positive_pas = []
    negative_pas = []

    if load_tqdm:
        pbar = tqdm(f, total=total)
        pbar.set_description("Reading input file for " + name)

    pos_freq = 0
    neg_freq = 0
    for line in f:
        row = str(line).replace("\n", "").strip().split(" ")
        if raw_data:
            partial_assignment = [int(literal) for literal in row[:-1]]
        else:
            partial_assignment = set()
            for i, j in enumerate(var_range):
                if str(j) in row[:-1]:
                    partial_assignment.add(i + 1)
                elif negation:
                    # Using closed world assumption to register absent variables as if they are false.
                    partial_assignment.add(-(i + 1))

        if (not switch_signs and row[-1] == target_class[0]) or (
            switch_signs and row[-1] == target_class[1]
        ):
            negative_pas.append(frozenset(partial_assignment))
            neg_freq += 1
            if load_top_k and len(negative_pas) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not switch_signs and row[-1] == target_class[1]) or (
            switch_signs and row[-1] == target_class[0]
        ):
            positive_pas.append(frozenset(partial_assignment))
            pos_freq += 1
        else:
            print("Row found without target class at the end:\n")
            print(line)

        if load_tqdm:
            pbar.update(1)

    if print_stats:
        print_input_data_statistics(
            name,
            positive_pas,
            negative_pas,
            target_class,
            negation,
            load_top_k,
            switch_signs,
        )

    return (positive_pas, negative_pas)


def load_adult(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "adult.D97.N48842.C2.num",
        48842,
        list(range(1, 96)),
        ["96", "97"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=95,
        load_tqdm=load_tqdm,
    )


def load_breast(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "breast.D20.N699.C2.num",
        699,
        list(range(1, 19)),
        ["19", "20"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=18,
        load_tqdm=load_tqdm,
    )


def load_chess(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "chess.txt",
        3196,
        list(range(1, 74)),
        ["74", "75"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=73,
        load_tqdm=load_tqdm,
    )


def load_ionosphere(
    negation=False, load_top_k=None, switch_signs=False, load_tqdm=True
):
    return load_dataset(
        "ionosphere.D157.N351.C2.num",
        351,
        list(range(1, 156)),
        ["156", "157"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=155,
        load_tqdm=load_tqdm,
    )


def load_mushroom(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "mushroom_cp4im.txt",
        8124,
        list(range(0, 117)),
        ["0", "1"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=116,
        load_tqdm=load_tqdm,
    )


def load_pima(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "pima.D38.N768.C2.num",
        768,
        list(range(1, 37)),
        ["37", "38"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=36,
        load_tqdm=load_tqdm,
    )


def load_tictactoe(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):

    return load_dataset(
        "ticTacToe.D29.N958.C2.num",
        958,
        list(range(1, 28)),
        ["28", "29"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=27,
        load_tqdm=load_tqdm,
    )


def print_1d(frozen_set):
    l = list(frozen_set)
    abs_l = [abs(i) for i in l]
    print(np.array([x for _, x in sorted(zip(abs_l, l))]))


def print_2d(matrix):
    for row in matrix:
        if isinstance(row, np.ndarray):
            print(row)
        elif isinstance(row, frozenset):
            print_1d(row)
        else:
            print(row)
    print()


def check_pa_satisfiability(pa, clauses):
    if clauses is None or len(clauses) == 0:
        return None
    else:
        return not (
            solve([tuple(clause) for clause in clauses] + [(a,) for a in pa]) == "UNSAT"
        )


def get_alphabet_size(theory):
    alphabets = set()
    for clause in theory:
        for literal in clause:
            alphabets.add(abs(int(literal)))

    return len(alphabets)


def get_clauses(theory):
    """
    :param theory: it can be an instance if a Theory class or could just be None
    :return: a list of clauses present in the theory
    """
    if hasattr(theory, "clauses"):
        return theory.clauses
    else:
        return []


def get_errors(theory):
    """
    :param theory: it can be an instance if a Theory class or could just be None
    :return: a list of errors present in the theory
    """
    if hasattr(theory, "errors"):
        return theory.errors
    else:
        return []


def get_literal_length(clauses):
    """
    :param clauses: a CNF/Theory
    :return: the total number of literals in the whole theory
    """
    return sum([len(clause) for clause in clauses])


def get_entropy(clauses):
    """
    :param clauses: a list of clauses
    :return: the total number of bits required to represent the input theory
    """

    counts = Counter()
    total_literals = 0
    for clause in clauses:
        for literal in clause:
            counts[literal] += 1
            total_literals += 1

    entropy = 0
    for v in counts.values():
        p = float(v) / total_literals
        if p > 0.0:
            entropy -= math.log(p, 2)

    return entropy


def log_star(input):
    # From the footnote on page 5 of the paper https://hal.inria.fr/hal-02505913/document
    result = math.log(2.865064)
    n = copy(input)
    while True:
        n = math.log(n)
        if n <= 0:
            break
        else:
            result += n

    # print("Log*N( " + str(input) + ")\t= " + str(result))
    return result


def get_modified_entropy(clauses, alphabet_size):
    """
    :param clauses: a list of clauses
    :return: the total number of bits required to represent the input theory
    """
    c_entropy = 0
    if len(clauses) == 0:
        return c_entropy

    for j, clause in enumerate(clauses):

        for i, literal in enumerate(clause):
            c_entropy -= math.log(1.0 / (2 * len(clause) + 1 - 2 * i), 2)

        # c_entropy -= math.log(1.0 / (2 * len(clauses) + 1 - 2 * len(clause)), 2)
        c_entropy -= math.log(1.0 / (2 * len(clauses) + 1 - 2 * j), 2)

    c_entropy += log_star(2 * alphabet_size)
    c_entropy += log_star(len(clauses))

    return c_entropy


def get_dl(dl_measure, clauses, errors, alphabet_size):
    if dl_measure == "ll":  # Literal Length
        return get_literal_length(clauses)
    elif dl_measure == "sl":  # Symbol Length
        return (
            get_literal_length(clauses + list(errors)) + len(clauses + list(errors)) - 1
        )
    elif dl_measure == "se":  # Shanon Entropy
        return get_entropy(clauses) + get_entropy(errors)
    elif dl_measure == "me":  # Clement Entropy
        return get_modified_entropy(clauses, alphabet_size) + get_modified_entropy(
            errors, alphabet_size
        )


class Eclat:
    def __init__(self, minsup):
        self.minsup = minsup
        # self.freq_items = []
        self.freq_items = {}
        self.pruned_itemsets = set()

    def read_data(self, input):
        data = {}
        for i, pa in enumerate(input):
            for item in pa:
                if item not in data:
                    data[item] = set()
                data[item].add(i)
        return data

    def eclat(self, prefix, items, dict_id):
        while items:
            i, itids = items.pop()
            if len(itids) >= self.minsup:
                # if len(prefix) > 0:  # Only store patterns with length > 1
                # self.freq_items.append((frozenset(prefix + [i]), itids))

                if frozenset(prefix) in self.freq_items and len(
                    self.freq_items[frozenset(prefix)]
                ) <= len(itids):
                    self.pruned_itemsets.add(frozenset(prefix))
                    del self.freq_items[frozenset(prefix)]
                # elif frozenset(prefix + [i]) not in self.pruned_itemsets:
                if frozenset(prefix + [i]) in self.pruned_itemsets:
                    old_itids = self.freq_items[frozenset(prefix + [i])]
                    print("Old len(itids)\t: " + str(len(old_itids)))
                    print("New len(itids)\t: " + str(len(itids)))
                    print("Intersection len(itids)\t: " + str(itids & old_itids))

                self.freq_items[frozenset(prefix + [i])] = itids

                suffix = []
                for j, ojtids in items:
                    jtids = itids & ojtids
                    if len(jtids) >= self.minsup:
                        suffix.append((j, jtids))

                dict_id += 1
                self.eclat(
                    prefix + [i],
                    sorted(suffix, key=lambda item: len(item[1]), reverse=True),
                    dict_id,
                )

    def get_Frequent_Itemsets(self, input):

        start_time = time()
        prefix = []
        dict_id = 0

        data = self.read_data(input)
        sorted_data = sorted(data.items(), key=lambda item: len(item[1]), reverse=True)
        self.eclat(prefix, sorted_data, dict_id)
        # self.freq_items = list(self.freq_items.items())

        # print("Frequent Itemsets\t:")
        # for item, clause_ids in self.freq_items.items():
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print()

        # sort freq items descending wrt change in DL (wrt W-op), length of the itemset, frequency

        # self.freq_items.sort(
        #     key=lambda item: (
        #         len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
        #         len(item[0]),
        #         len(item[1]),
        #     ),
        #     reverse=True,
        # )

        print(
            str(len(self.freq_items))
            + " Frequent Itemsets are mined in "
            + str(time() - start_time)
            + " seconds."
        )
        # for item, clause_ids in self.freq_items:
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print()

        return self.freq_items


class Mistle:
    def __init__(self, positives, negatives):
        self.positives = positives
        self.negatives = negatives

        self.total_positives = len(positives)
        self.total_negatives = len(negatives)

        self.theory = Theory(clauses=[], overlap_matrix=[], search_index=0)
        self.beam = None
        self.initial_dl = None

    def check_clause_validity(self, input_clause1, input_clause2, output_clauses):
        """
        :param clauses: List of new clauses that can be added to the theory
        :return:
            True    if the new theory is consistent with the data
            False   otherwise
        """

        new_theory = copy(self.theory.clauses)
        new_theory.remove(input_clause1)
        new_theory.remove(input_clause2)
        new_theory += list(output_clauses)

        uncovered_positives = set()

        for pa in self.positives:
            if not check_pa_satisfiability(pa, new_theory):
                uncovered_positives.add(pa)

        return uncovered_positives

    def learn(
        self,
        dl_measure,
        minsup=None,
        k=None,
        lossy=True,
        prune=True,
        mining_steps=None,
        permitted_operators=None,
    ):
        """

        :param dl_measure: it can take one
        :param minsup:
        :param k:
        :param lossy:
        :param prune:
        :param mining_steps:
        :param permitted_operators:
        :return:
        """

        if permitted_operators is None:
            # Assume, by default, that each operator is permitted to compress the theory.
            permitted_operators = {
                "D": True,
                "W": True,
                "V": True,
                "S": True,
                "R": True,
                "T": True,
            }

        # This line of code assumes that variable numbers start from 1:
        # initial_alphabet_size = max(
        #     [abs(literal) for pa in self.positives + self.negatives for literal in pa]
        # )

        initial_alphabet_size = get_alphabet_size(self.positives + self.negatives)
        self.initial_dl = get_dl(
            dl_measure, self.positives + self.negatives, [], initial_alphabet_size
        )

        # Remove redundant partial assignments
        self.positives = set(self.positives)
        self.negatives = set(self.negatives)

        # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
        inconsistent_pas = self.positives & self.negatives
        consistent_positives = self.positives - inconsistent_pas
        self.positives = copy(consistent_positives)
        self.negatives = self.negatives - inconsistent_pas

        # Convert the set of -ve PAs to a theory
        if minsup is None and k is None:
            minsup = 1
        success = self.theory.intialize(
            self.positives, self.negatives, dl_measure, initial_alphabet_size, minsup, k
        )
        if not success:
            # This is the case when no frequent itemsets get mined or when the input data is empty.
            # Return empty theory with 0 compression
            return None, 0

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory.theory_length = self.total_negatives

        prev_clauses = []
        mining_count = 0
        while True:
            while True:
                success = self.theory.compress(lossy, permitted_operators)
                if success == "ignore_itemset":
                    del self.theory.freq_items[0]
                elif not success:
                    break
                if success is True:
                    neg_violations = self.theory.get_violations(
                        self.theory.clauses, self.negatives, True, "-"
                    )
                    assert len(neg_violations) == 0

            if self.theory.final_alphabet_size != get_alphabet_size(
                self.theory.clauses
            ):
                self.theory.final_alphabet_size = get_alphabet_size(self.theory.clauses)

            neg_violations = self.theory.get_violations(
                self.theory.clauses, self.negatives, True, "-"
            )

            assert len(neg_violations) == 0

            # Unpack W operator if one of its non-definition clauses got truncated and the application is no longer useful.
            self.theory.resolve()

            if self.theory.clauses == prev_clauses:
                break
            else:
                prev_clauses = self.theory.clauses

            mining_count += 1

            if mining_steps is None or (
                mining_steps is not None and mining_steps > mining_count
            ):
                self.theory.freq_items = self.theory.get_frequent_itemsets(
                    self.theory.clauses, minsup=1
                )
            else:
                # Break the loop and don't compress further if maximum number of mining steps reached.
                break

        # print(
        #     "--------------------------Frequent Itemsets--------------------------\t:"
        # )
        # for item, clause_ids in self.theory.freq_items:
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print("\n")

        if prune:
            # print("Theory before pruning\t:")
            # print_2d(self.theory.clauses)

            self.theory.prune()

        final_dl = get_dl(
            dl_measure,
            self.theory.clauses,
            self.theory.errors,
            self.theory.final_alphabet_size,
        )

        compression = (self.initial_dl - final_dl) / float(self.initial_dl)
        print("Initial DL\t\t\t\t: " + str(self.initial_dl))
        print("Final DL\t\t\t\t: " + str(final_dl))
        print(
            "Compression (wrt "
            + str(self.theory.dl_measure)
            + ")\t: "
            + str(compression)
        )
        print("Operator Counters\t\t: " + str(self.theory.operator_counter))

        return self.theory, compression


class Theory:
    def __init__(self, clauses, overlap_matrix, search_index):
        self.clauses = clauses
        self.unpacked_clauses = dict()
        self.unpacked_w = dict()
        self.overlap_matrix = overlap_matrix
        self.search_index = search_index
        self.theory_length = len(clauses)  # Total number of clauses in the theory
        self.invented_predicate_definition = dict()
        self.freq_items = []
        self.errors = set()
        self.positives = set()
        self.negatives = set()
        self.minsup = None
        self.k = None
        self.dl = None
        self.dl_measure = None
        self.operator_counter = {"D": 0, "W": 0, "V": 0, "S": 0, "R": 0, "T": 0}
        self.new_var_counter = None
        self.initial_alphabet_size = None
        self.final_alphabet_size = None
        self.prunable_invented_literals = set()

    def intialize(
        self, positives, negatives, dl_measure, alphabet_size, minsup=None, k=None
    ):

        if (
            len(negatives) == 0
        ):  # We still allow a theory to be learned and compressed if there are some negatives and no positives.
            return False

        # Construct a theory from the partial assignments
        for pa in negatives:
            self.clauses.append(frozenset([-literal for literal in pa]))

        if minsup is None:
            self.minsup = None
        elif minsup >= 1:
            self.minsup = int(minsup)
        elif minsup < 1:
            self.minsup = minsup * len(self.clauses)

        self.k = k

        if minsup is not None:
            self.freq_items = self.get_frequent_itemsets(self.clauses, self.minsup)
        elif k is not None:
            self.freq_items, minsup = self.get_frequent_itemsets_topk(self.clauses, k)
            self.minsup = minsup

        if len(self.freq_items) == 0:
            return False

        self.initial_alphabet_size = alphabet_size
        self.final_alphabet_size = alphabet_size
        self.new_var_counter = alphabet_size + 1
        self.positives = positives
        self.negatives = negatives
        self.errors = self.get_violations(self.clauses, positives)
        self.dl = get_dl(
            dl_measure, list(positives | negatives), [], self.new_var_counter - 1
        )
        print("DL of initial theory\t: " + str(self.dl))
        self.dl_measure = dl_measure
        return True

    def is_invented_literal(self, literal):
        """
        # Returns true if the |literal| > largest variable in the data
        Returns true if the literal is invented only due to the 'W' operator and not due to 'D' operator
        :param literal:
        :return:
        """

        if (literal >= -self.initial_alphabet_size) and (
            literal <= self.initial_alphabet_size
        ):
            return False
        else:
            return abs(literal) in self.invented_predicate_definition

    def is_definition_clause(self, clause, invented_literals=None):
        """
        Returns true if a clause is serving as a definition for an invented predicate.
        :param clause: The clause which is to be checked if it is a definition clause
        :param invented_literals: Check if the clause is a definition clause for only these literals
        :return:
        """
        unpacked_clause = self.unpack_clause(clause)
        for literal in unpacked_clause:
            if (
                literal in self.invented_predicate_definition
                and unpacked_clause
                == self.unpack_clause(
                    frozenset(
                        [literal] + list(self.invented_predicate_definition[literal])
                    )
                )
            ):
                if invented_literals is None:
                    return True
                elif literal in invented_literals:
                    return True
        return False

    def get_invented_literals(self, clause):
        invented_literals = set()
        for literal in clause:
            if self.is_invented_literal(literal):
                invented_literals.add(abs(literal))
        return invented_literals

    def get_frequent_itemsets_topk(self, clauses, k, decrement_factor=2):
        """
        :param clauses: all the transactions
        :param k: maximum itemsets to be returned
        :param decrement_factor: a float > 1 by which the minimum support threshold needs to be decreased
        :return: Return top-k closed frequent itemsets, a minsup value that is guaranteed to mine at least k closed frequent itemsets
        """

        start_time_lcm = time()
        if decrement_factor <= 1:
            decrement_factor = 2
        minsup = int(len(clauses) / decrement_factor)

        while minsup >= 1:
            freq_items_lcm = compute_itemsets(clauses, minsup / len(clauses), "LCM")
            if len(freq_items_lcm) > k:
                break
            else:
                minsup = int(minsup / decrement_factor)

        total_time_lcm = time() - start_time_lcm
        print("Length of freq items by LCM\t: " + str(len(freq_items_lcm)))
        print("Time of freq items by LCM\t: " + str(total_time_lcm))

        start_time_eclat = time()
        eclat = Eclat(minsup=minsup)
        freq_items_eclat = eclat.get_Frequent_Itemsets(clauses)
        total_time_eclat = time() - start_time_eclat
        print("Length of freq items by Eclat\t: " + str(len(freq_items_eclat)))
        print("Time of freq items by Eclat\t: " + str(total_time_eclat))

        assert len(freq_items_eclat) >= len(freq_items_lcm)

        result = []
        if len(freq_items_eclat) == len(freq_items_lcm):
            result = list(freq_items_eclat.items())
        else:
            for i, (itemset, frequency) in enumerate(freq_items_lcm):
                item = frozenset(itemset)
                if item not in freq_items_eclat:
                    print("Itemset not found\t: " + str(item))
                else:
                    result.append((item, freq_items_eclat[item]))

        result.sort(
            key=lambda item: (
                len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                len(item[0]),
                len(item[1]),
            ),
            reverse=True,
        )

        # Only return top-k closed frequent itemsets
        if len(result) > k:
            result = result[:k]

        return result, minsup

    def get_frequent_itemsets(self, clauses, minsup):

        start_time1 = time()
        freq_items1 = compute_itemsets(clauses, minsup / len(clauses), "LCM")
        total_time1 = time() - start_time1
        print("Length of freq items 1\t: " + str(len(freq_items1)))
        print("Time of freq items 1\t: " + str(total_time1))

        start_time2 = time()
        eclat = Eclat(minsup=minsup)
        freq_items2 = eclat.get_Frequent_Itemsets(clauses)
        total_time2 = time() - start_time2
        print("Length of freq items 2\t: " + str(len(freq_items2)))
        print("Time of freq items 2\t: " + str(total_time2))

        assert len(freq_items2) >= len(freq_items1)

        result = []
        if len(freq_items2) == len(freq_items1):
            result = list(freq_items2.items())
        else:
            for i, (itemset, frequency) in enumerate(freq_items1):
                item = frozenset(itemset)
                if item not in freq_items2:
                    print("Itemset not found\t: " + str(item))
                else:
                    result.append((item, freq_items2[item]))

        result.sort(
            key=lambda item: (
                len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                len(item[0]),
                len(item[1]),
            ),
            reverse=True,
        )

        return result

    def get_new_var(self):
        """
        Invent a new predicate
        :return: An integer (updated counter) which has not been used as a literal before
        """
        new_var = self.new_var_counter
        self.new_var_counter += 1
        self.final_alphabet_size += 1
        return new_var

    def get_violations(
        self, clauses, partial_assignments, print_violations=True, sign="+"
    ):
        """
        Get the number of partial assignments violated by the given theory.
        """
        if len(clauses) == 0 or len(partial_assignments) == 0:
            return set()

        violated_pas = set()

        for pa in partial_assignments:
            if (sign == "+" and not check_pa_satisfiability(pa, clauses)) or (
                sign == "-" and check_pa_satisfiability(pa, clauses)
            ):
                violated_pas.add(pa)

        if print_violations and len(violated_pas) > 0:
            print(
                len(violated_pas),
                " violations of "
                + sign
                + "ve partial assignments found for the learned theory.",
            )
        return violated_pas

    def __len__(self):
        return self.theory_length

    def get_compression(self, input_clauses, op, output_clauses):

        new_theory = copy(self.clauses)
        for clause in input_clauses:
            new_theory.remove(clause)
        if op in {"D", "W"}:
            for clause in output_clauses:
                substituted_clause = []
                for literal in clause:
                    if literal == "#":
                        substituted_clause.append(self.new_var_counter)
                    elif literal == "-#":
                        substituted_clause.append(-self.new_var_counter)
                    else:
                        substituted_clause.append(literal)
                new_theory.append(substituted_clause)
        else:
            new_theory += list(output_clauses)

        uncovered_positives = set()
        if op in {"D", "V", "T"}:
            for pa in self.positives:
                if not check_pa_satisfiability(pa, new_theory):
                    uncovered_positives.add(pa)

        if op in {"D", "W"}:
            return (
                uncovered_positives,
                # The alphabet size for D-op and W-op is incremented by 1 as they invent a new literal, that is represented temporarily by "#" here.
                get_dl(
                    self.dl_measure,
                    new_theory,
                    list(self.errors | uncovered_positives),
                    self.new_var_counter,
                ),
            )
        else:
            return (
                uncovered_positives,
                get_dl(
                    self.dl_measure,
                    new_theory,
                    list(self.errors | uncovered_positives),
                    self.new_var_counter - 1,
                ),
            )

    def select_best_operator(self, input_clause_list, possible_operations):
        success = "ignore_itemset"
        operator_precedence = ["S", "R", "W", "D", "V", "T", None]
        min_dl = self.dl
        best_operator = None
        new_errors = None
        output_clause_list = None
        possible_entropies = []
        for op, output_clauses in possible_operations:
            # errors, compression = self.get_compression(
            #     input_clause_list, op, output_clauses
            # )
            # if compression > max_compression or (
            #     compression == max_compression
            #     and operator_precedence.index(op)
            #     < operator_precedence.index(best_operator)
            # ):

            errors, dl = self.get_compression(input_clause_list, op, output_clauses)
            possible_entropies.append((op, dl))
            if dl < min_dl or (
                dl == min_dl
                and operator_precedence.index(op)
                < operator_precedence.index(best_operator)
            ):
                min_dl = dl
                best_operator = op
                new_errors = errors
                output_clause_list = list(output_clauses)
                success = True

        if min_dl == self.dl:
            # print(
            #     "Ignoring itemset\t: Initial DL = "
            #     + str(self.dl)
            #     + "\t; Possible DLs\t: "
            #     + str(possible_entropies)
            # )
            success = "ignore_itemset"

        return (
            success,
            best_operator,
            input_clause_list,
            output_clause_list,
            min_dl,
            new_errors,
        )

    def apply_best_operator(
        self, best_operator, input_clause_list, output_clause_list, min_dl, new_errors
    ):

        # Update old_theory
        self.operator_counter[best_operator] += 1
        if best_operator in {"D", "W"}:
            new_var = self.get_new_var()
            for i, clause in enumerate(output_clause_list):
                if "#" in clause:
                    output_clause_list[i].remove("#")
                    output_clause_list[i].add(new_var)
                else:
                    assert "-#" in clause
                    output_clause_list[i].remove("-#")
                    output_clause_list[i].add(-new_var)
                output_clause_list[i] = frozenset(output_clause_list[i])
            subclause = self.invented_predicate_definition["#"]
            if best_operator == "W":
                assert (
                    new_var not in self.invented_predicate_definition
                )  # DEBUG: Remove once debugging is complete
                self.invented_predicate_definition[new_var] = subclause

        if "#" in self.invented_predicate_definition:
            del self.invented_predicate_definition["#"]

        self.errors |= new_errors
        self.positives -= new_errors
        # self.total_compression += max_compression
        self.theory_length += len(output_clause_list) - len(input_clause_list)

        pruned_indices = set(self.freq_items[0][1])

        if best_operator == "T":
            # Check if any unpacked clauses are subsumed by the output of the T operator
            # invented_literal = None
            for i, clause in enumerate(self.clauses):
                if i in pruned_indices:
                    continue
                unpacked_clause = self.unpack_clause(clause)
                # if self.is_definition_clause(clause):
                #     invented_literal = None
                #     for literal in clause:
                #         if literal > self.initial_alphabet_size:
                #             # We assert that there is only one positive invented literal in the clause
                #             invented_literal = literal
                #             break
                #     assert invented_literal is not None
                #
                # # This code assumes that the definition clause will be contiguously followed by its non-definition clauses
                # # ex: [2, 3, -8, 9, -4], [-5, -6, -9, 7], [1, -6, -9, 7], [-7, -5, -1, -9]
                # # The first clause is a definition clause as it contains 9 (alphabet size was 8).
                # # The rest are non-definition clauses as they contain -9. These are assumed to come right after their definition clause.
                # if invented_literal is not None and -invented_literal in clause:
                #     # This makes sure that if we prune a definition clause, we also prune all the clauses that were using that invented literal.
                #     pruned_indices.add(i)
                # elif invented_literal not in clause:
                #     invented_literal = None

                if output_clause_list[0].issubset(unpacked_clause):
                    pruned_indices.add(i)
                    if self.is_definition_clause(clause):
                        self.operator_counter["W"] -= 1
                        print(
                            "Line 1326: Decreasing final alphabet size due to "
                            + str(clause)
                        )
                        self.final_alphabet_size -= 1

                    self.prunable_invented_literals |= self.get_invented_literals(
                        clause
                    )

        pruned_indices = list(pruned_indices)

        # Delete indices that are in pruned indices and decrease indices of other clauses correspondingly
        buffer = sorted(pruned_indices)
        # print("Initial Buffer\t: " + str(buffer))
        decrement = 0
        replace_dict = {}
        for i in range(len(self.clauses)):
            if len(buffer) > 0 and i == buffer[0]:
                del buffer[0]
                decrement += 1
            else:
                replace_dict[i] = i - decrement

        # print("Final Buffer\t: " + str(buffer))
        # print("Replace Dict\t: " + str(replace_dict))

        # Replace the indices of clauses wrt the replacement dictionary
        increment = len(replace_dict)
        # new_freq_items = []
        del self.freq_items[0]
        prune_itemsets = []
        for i, (itemset, clause_ids) in enumerate(self.freq_items):

            new_clause_ids = set()
            # Repalce ids of old existing clauses
            for clause_id in clause_ids:
                if clause_id in replace_dict:
                    new_clause_ids.add(replace_dict[clause_id])

            # Add ids of new clause covering this itemset
            for j, clause in enumerate(output_clause_list):
                if itemset.issubset(clause):
                    new_clause_ids.add(j + increment)

            if len(new_clause_ids) >= 2:
                self.freq_items[i] = (itemset, new_clause_ids)
            else:
                prune_itemsets.append(i)

        prune_itemsets.sort(reverse=True)
        for i in prune_itemsets:
            del self.freq_items[i]

        self.freq_items.sort(
            key=lambda item: (
                len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                len(item[0]),
                len(item[1]),
            ),
            reverse=True,
        )

        print(
            str(len(self.freq_items))
            + "\titemsets left after "
            + best_operator
            + " operation \t: "
            + str(output_clause_list)
        )
        # print("New Frequent Itemsets\t:")
        # for item, clause_ids in self.freq_items:
        #     print(" {}\t: {}".format(list(item), len(clause_ids)))
        # print()

        pruned_indices.sort(reverse=True)
        for i in pruned_indices:
            del self.clauses[i]

        for clause in output_clause_list:
            self.clauses.append(clause)

        # assert min_entropy == get_entropy(self.clauses) + get_entropy(self.errors)
        # self.entropy = min_entropy
        self.dl = min_dl

    def compress(self, lossy=True, permitted_operators=None):
        # TODO: Make it more efficient once it is complete. Reduce the number of iterations on input_clause_list/residue

        if len(self.freq_items) == 0:
            return False
        subclause, clause_ids = self.freq_items[0]
        if len(clause_ids) == 1:
            return "ignore_itemset"

        input_clause_list = []  # A list of input clauses (to be compressed)
        residue = []  # A list of input clauses without the shared subclause
        possible_operations = []

        for clause_id in clause_ids:
            clause = self.clauses[clause_id]
            input_clause_list.append(clause)
            residue.append(clause - subclause)

        if (
            subclause in self.unpacked_w
            and set(input_clause_list) == self.unpacked_w[subclause]
        ):
            return "ignore_itemset"

        # Check if S-operator is applicable
        if permitted_operators["S"] is True:
            for clause in input_clause_list:
                if clause == subclause:
                    # S-operator is applicable
                    _, dl = self.get_compression(input_clause_list, "S", [subclause])
                    self.apply_best_operator(
                        "S", input_clause_list, [subclause], dl, set()
                    )
                    return True

        # Check if R-operator is applicable
        if permitted_operators["R"] is True:
            r_applicable = False
            # r_residue = [tuple(clause) for clause in residue] + [tuple(-literal) for literal in subclause]
            # Technically, if we assume that each atom can already appear once in a clause, then it suffices to check the
            # unsatisfiability of [tuple(clause) for clause in residue] as each of the literal in the subclause is absent in the residue.
            r_residue = [tuple(clause) for clause in residue]
            if solve(r_residue) == "UNSAT":
                # R-operator is applicable (R-operator is a special case when applying T-operator is lossless.
                # Example: If input is [(a,b), (a,c), (a,-b,-c)], then the output is [(a)].
                # Here, then input is logically equivalent to the output (Lossless Truncation).
                _, dl = self.get_compression(input_clause_list, "R", [subclause])
                self.apply_best_operator("R", input_clause_list, [subclause], dl, set())
                return True

        if permitted_operators["T"] is True and lossy:
            possible_operations.append(("T", [subclause]))

        # Check if V-operator is applicable
        v_literals = []
        if permitted_operators["V"] is True and lossy:
            for clause in residue:
                if len(clause) == 1:
                    v_literals.append(next(iter(clause)))
                    break

        if v_literals:
            for v_literal in v_literals:
                v_output = []
                for clause in residue:
                    if len(clause) == 1 and v_literal == next(iter(clause)):
                        new_clause = set(copy(subclause))
                        new_clause.add(v_literal)
                        v_output.append(frozenset(new_clause))
                    else:
                        new_clause = set(copy(clause))
                        if v_literal not in new_clause:
                            new_clause.add(-v_literal)
                            v_output.append(frozenset(new_clause))
                possible_operations.append(("V", v_output))

        # Check if D-operator is applicable
        # D-operator is only applicable if V operator and S operators are not applicable
        if permitted_operators["D"] is True and lossy and not v_literals:
            d_literals = []
            for literal in residue[0]:
                d_literals.append(abs(literal))

            assert len(d_literals) == len(
                set(d_literals)
            )  # DEBUG: Remove once debugging od D_op is complete

            for clause in residue[1:]:
                remove_d_literals = []
                for literal in d_literals:
                    if literal not in clause and -literal not in clause:
                        remove_d_literals.append(literal)
                for literal in remove_d_literals:
                    d_literals.remove(literal)

                if len(d_literals) == 0:
                    break

            # D-operator is applicable on the set of literals contained in d_literals.
            # It is not applicable if d_literals is empty.

            if d_literals:
                for d_literal in d_literals:
                    clause1 = set(copy(subclause))
                    clause1.add(d_literal)
                    clause1.add("#")

                    clause2 = set(copy(subclause))
                    clause2.add(-d_literal)
                    clause2.add("-#")

                    d_output = [clause1, clause2]

                    for clause in residue:
                        clause3 = set(copy(clause))
                        assert (
                            d_literal in clause or -d_literal in clause
                        )  # DEBUG: Remove once debugging od D_op is complete
                        if d_literal in clause:
                            clause3.remove(d_literal)
                            clause3.add("-#")
                        elif -d_literal in clause:
                            clause3.remove(-d_literal)
                            clause3.add("#")
                        d_output.append(clause3)

                    possible_operations.append(("D", d_output))

        # Consider W-operator
        if permitted_operators["W"] is True:
            # Use '#' as a special newly invented variable.
            # If/once W-operator is accepted for compression, it will be replaced by a self.new_var_counter()
            new_clause = set(copy(subclause))
            new_clause.add("#")
            w_output = [new_clause]
            self.invented_predicate_definition["#"] = subclause

            for clause in residue:
                new_clause = set(clause)
                new_clause.add("-#")
                w_output.append(new_clause)

            possible_operations.append(("W", w_output))

        (
            success,
            best_operator,
            input_clause_list,
            output_clause_list,
            min_dl,
            new_errors,
        ) = self.select_best_operator(input_clause_list, possible_operations)
        if success is True:
            self.apply_best_operator(
                best_operator, input_clause_list, output_clause_list, min_dl, new_errors
            )

        return success

    def unpack_clause(self, clause, invented_literals=None):
        """
        Substitute the invented literals with their definitions in a clause
        :param clause: The clause which is to be unpacked or resolved
        :param invented_literals: Replace only these invented literal while unpacking and leave others as is
        :return:
        """

        if invented_literals is not None:
            unpacked_clause = set()
            for literal in clause:
                if -literal in invented_literals:
                    # unpacked_clause |= self.invented_predicate_definition[-literal]
                    unpacked_clause |= self.unpack_clause(
                        self.invented_predicate_definition[-literal],
                        invented_literals - {-literal},
                    )
                else:
                    unpacked_clause.add(literal)
            return frozenset(unpacked_clause)

        if clause in self.unpacked_clauses:
            return self.unpacked_clauses[clause]

        unpacked_clause = set()
        for literal in clause:
            if literal < 0 and -literal in self.invented_predicate_definition:
                definition = self.invented_predicate_definition[-literal]
                unpacked_clause |= definition
            else:
                unpacked_clause.add(literal)

        # Cache the unpacked clause
        self.unpacked_clauses[clause] = frozenset(unpacked_clause)

        return frozenset(unpacked_clause)

    def resolve(self):
        """
        Unpack W operator if one of its non-definition clauses got truncated and the application is no longer useful.
        Ex: Consider a case when Mistle invents the literal '12' and defines it as 6 or -8.
            [6, -8, 12], [-1, -2, -3, 5, -12], [4, -11, -12], [-7, -12]

            But now if the second clause gets deleted as it is subsumed by [-1, -2], the following clauses are left:
            [6, -8, 12], [4, -11, -12], [-7, -12]

            And now, it makes sense for us to undo the 'W' operator (as it is no longer compressing much) to get:
            [4, -11, 6, -8], [-7, 6, -8]

            The invented literals to check for are stored in the set, self.prunable_invented_literals
        """
        if not self.prunable_invented_literals:
            return

        # ALl the clauses in self.clauses are divided into 2 parts:
        # 1. imprunable_clauses: clauses that do not contain any pruned invented literals
        # 2. prunable_clauses: clauses that contain any pruned invented literals which may be unpacked later
        prunable_clauses = []
        imprunable_clauses = []
        for clause in self.clauses:
            literal_found = False
            for literal in clause:
                if abs(literal) in self.prunable_invented_literals:
                    prunable_clauses.append(clause)
                    literal_found = True
                    break
            if literal_found is False:
                imprunable_clauses.append(clause)

        unpack_literals = set()
        self.prunable_invented_literals = list(self.prunable_invented_literals)
        self.prunable_invented_literals.sort(reverse=True)
        for literal in self.prunable_invented_literals:
            temp_theory = copy(imprunable_clauses)
            unpack_clauses = []
            pack_clauses = []
            for clause in prunable_clauses:
                if -literal in clause:
                    pack_clauses.append(clause)
                    unpack_clauses.append(self.unpack_clause(clause, {literal}))
                elif literal in clause:
                    pack_clauses.append(clause)

            temp_theory += pack_clauses
            temp_theory += unpack_clauses
            unpack_dl = get_dl(
                self.dl_measure,
                temp_theory,
                list(self.errors),
                self.new_var_counter - 2,
            )

            if unpack_dl < self.dl:
                # It is better to keep the unpacked clause for this particular W operator
                # output_clauses += unpack_clauses
                unpack_literals.add(literal)
                print(
                    "Unpacking W operator applied earlier using "
                    + str(literal)
                    + " on "
                    + str(self.invented_predicate_definition[literal])
                )
                self.unpacked_w[literal] = copy(
                    self.invented_predicate_definition[literal]
                )
                # TODO: what if there is a "D" operator involved that needs to be unpacked
                self.operator_counter["W"] -= 1
                self.final_alphabet_size -= 1

            # else:
            #     # It is better to keep the packed clause for this particular W operator
            #     output_clauses += pack_clauses

        self.prunable_invented_literals = set()

        if len(unpack_literals) == 0:
            return

        pruned_clauses = []
        for clause in prunable_clauses:
            if not self.is_definition_clause(clause, unpack_literals):
                unpacked_pruned_clause = self.unpack_clause(clause, unpack_literals)
                pruned_clauses.append(unpacked_pruned_clause)

        for literal in unpack_literals:
            del self.invented_predicate_definition[literal]

        # assert len(pruned_clauses) == len(prunable_clauses) - len(unpack_literals)
        # print("Possible clauses:\n")
        # print_2d(possible_clauses)
        # print("Output clauses:\n")
        # print_2d(output_clauses)
        new_clauses = imprunable_clauses + pruned_clauses
        self.dl = get_dl(
            self.dl_measure, self.clauses, list(self.errors), self.final_alphabet_size
        )

        removal_indices = set()
        for i, clause in enumerate(new_clauses):
            for literal in unpack_literals:
                if literal in clause or -literal in clause:
                    removal_indices.add(i)
        removal_indices = list(removal_indices)
        removal_indices.sort(reverse=True)

        for i in removal_indices:
            del new_clauses[i]

        neg_violations = self.get_violations(new_clauses, self.negatives, True, "-")
        if len(neg_violations) == 0:
            self.clauses = new_clauses

        if self.final_alphabet_size != get_alphabet_size(self.clauses):
            self.final_alphabet_size = get_alphabet_size(self.clauses)

    def prune(self):

        # Don't check for pruning, those clauses that were unchanged from when the theory was initialized
        initial_clauses = set()
        for pa in self.negatives:
            initial_clauses.add(frozenset([-literal for literal in pa]))

        old_clauses = copy(self.clauses)

        pruned_clause_ids = set()
        clauses_set = copy(set(self.clauses))

        assert len(self.get_violations(clauses_set, self.positives)) == 0
        neg_violations = len(
            self.get_violations(clauses_set, self.negatives, True, "-")
        )
        assert neg_violations == 0

        for i, clause in enumerate(old_clauses):
            if clause in initial_clauses:
                continue
            violations = len(
                self.get_violations(
                    copy(clauses_set) - {clause}, self.negatives, False, sign="-"
                )
            )
            if violations == neg_violations:
                pruned_clause_ids.add(i)
                clauses_set.remove(clause)

        # print("Pruned IDs\t: " + str(pruned_clause_ids))

        self.clauses = []
        for i, clause in enumerate(old_clauses):
            if i not in pruned_clause_ids:
                self.clauses.append(clause)

        print(str(len(pruned_clause_ids)) + " clauses are pruned.")

        assert set(self.clauses) == clauses_set

        self.final_alphabet_size = get_alphabet_size(self.clauses)
        # if len(pruned_clause_ids) != 0:
        #     print("Pruned Theory\t:")
        #     print_2d(self.theory.clauses)


if __name__ == "__main__":
    # filename = sys.argv[1]
    # nb_rows = int(sys.argv[2])
    # nb_vars = int(sys.argv[3])
    # minsup = int(sys.argv[4])
    #
    # positives, negatives = load_dataset(
    #     filename,
    #     nb_rows,
    #     list(range(1, nb_vars + 1)),
    #     [str(nb_vars + 1), str(nb_vars + 2)],
    #     negation=False,
    #     load_top_k=None,
    #     switch_signs=False,
    #     num_vars=100,
    #     load_tqdm=True,
    #     raw_data=True,
    # )
    # start_time = time()
    # mistle = Mistle(positives, negatives)
    # theory, compression = mistle.learn(minsup=minsup, dl_measure="me")
    # print("Total time\t\t\t\t: " + str(time() - start_time) + " seconds.")
    # if theory is not None:
    #     print("Final theory has " + str(len(theory.clauses)) + " clauses.")
    # else:
    #     print("Empty theory learned.")

    from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
    import random
    import numpy as np

    seed = 0
    random.seed(seed)
    np.random.seed(seed)

    start_time = time()

    th = GeneratedTheory([[1, -4], [2, 5], [6, -7, -8]])
    generator = TheoryNoisyGeneratorOnDataset(th, 400, 0.01)
    positives, negatives = generator.generate_dataset()
    # positives, negatives = load_dtest()
    mistle = Mistle(positives, negatives)
    theory, compression = mistle.learn(dl_measure="me", k=100)
    print("Total time\t\t\t\t: " + str(time() - start_time) + " seconds.")
    if theory is not None:
        print("Final theory has " + str(len(theory.clauses)) + " clauses.")
        # for c in theory.clauses:
        #     print(list(c))
        print_2d(theory.clauses)
    else:
        print("Empty theory learned.")
