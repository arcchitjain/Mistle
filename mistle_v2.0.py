from tqdm import tqdm
from copy import copy
from time import time
import numpy as np
from pycosat import solve
from collections import OrderedDict


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


def load_test2():
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


def check_pa_satisfiability(pa, clauses):
    return not (
        solve([tuple(clause) for clause in clauses] + [(a,) for a in pa]) == "UNSAT"
    )


class Eclat:
    def __init__(self, minsup):
        self.minsup = minsup
        self.freq_items = dict()

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
            isupp = len(itids)
            if isupp >= self.minsup:

                self.freq_items[frozenset(prefix + [i])] = isupp
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
        prefix = []
        dict_id = 0

        data = self.read_data(input)
        sorted_data = sorted(data.items(), key=lambda item: len(item[1]), reverse=True)
        self.eclat(prefix, sorted_data, dict_id)

        # for item, support in self.freq_items.items():
        #     print(" {}\t: {}".format(list(item), round(support, 4)))
        # print()

        # sort freq items descending wrt change in DL (wrt W-op), length of the itemset, frequency
        self.freq_items = [(k, v) for k, v in self.freq_items.items()]

        self.freq_items.sort(
            key=lambda itemset: (
                itemset[1] * len(itemset[0]) - (itemset[1] + len(itemset[0]) + 1),
                len(itemset[0]),
                itemset[1],
            ),
            reverse=True,
        )

        # for item, support in self.freq_items:
        #     print(" {}\t: {}".format(list(item), round(support, 4)))
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

    def get_literal_length(self, cnf):
        """
        :param cnf: a CNF/Theory
        :return: the total number of literals in the whole theory
        """
        return sum([len(clause) for clause in cnf])

    def get_violations(self, positives, print_violations=True):
        """
        Get the number of positive partial assignments violated by the given theory.
        :param positives:
        :param print_violations:
        :return:
        """
        violated_pos = set()

        for pos in positives:
            if not check_pa_satisfiability(pos, self.theory.clauses):
                violated_pos.add(pos)

        if print_violations & len(violated_pos) > 0:
            print(
                len(violated_pos),
                " violations of positive partial assignments found for the learned theory.",
            )
        return violated_pos

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

    def learn(self):

        # Remove redundant partial assignments
        self.positives = set(self.positives)
        self.negatives = set(self.negatives)

        self.theory.new_var_counter = (
            max([abs(l) for pa in self.positives | self.negatives for l in pa]) + 1
        )

        # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
        inconsistent_pas = self.positives & self.negatives
        consistent_positives = self.positives - inconsistent_pas
        self.positives = copy(consistent_positives)
        self.negatives = self.negatives - inconsistent_pas

        # Convert the set of -ve PAs to a theory
        self.theory.intialize(self.negatives)
        self.theory.errors = self.get_violations(self.positives)

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory.theory_length = self.total_negatives

        return self.theory


class Theory:
    def __init__(self, clauses, overlap_matrix, search_index):
        self.clauses = clauses
        self.overlap_matrix = overlap_matrix
        self.search_index = search_index
        self.clause_length = []  # List of lengths of all the clauses of this theory
        self.compression = 0
        self.theory_length = len(clauses)  # Total number of clauses in the theory

        self.freq_items = dict()
        self.errors = set()

        self.operator_counter = {"W": 0, "V": 0, "S": 0, "R": 0, "T": 0}
        self.new_var_counter = None

    def intialize(self, partial_assignments):
        # Construct a theory from the partial assignments
        for pa in partial_assignments:
            self.clauses.append(frozenset([-literal for literal in pa]))
            self.clause_length.append(len(pa))

        eclat = Eclat(minsup=3)
        self.freq_items = eclat.get_Frequent_Itemsets(self.clauses)

    def get_new_var(self):
        """
        Invent a new predicate
        :return: An integer (updated counter) which has not been used as a literal before
        """
        new_var = self.new_var_counter
        self.new_var_counter += 1
        return new_var

    def __len__(self):
        return self.theory_length


if __name__ == "__main__":
    positives, negatives = load_test2()
    mistle = Mistle(positives, negatives)
    theory = mistle.learn()
