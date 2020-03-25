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


def get_description_length(theory):
    dl = sum(theory.clause_length) + theory.theory_length - 1
    for misclassified_pa in theory.errors:
        dl += len(misclassified_pa)
    dl += len(theory.errors)
    print("Description Length of theory\t:" + str(dl))
    return dl


def get_decrease_in_DL(old_clause_list, old_errors, new_clause_list, new_errors):

    old_dl = 0
    for clause in new_clause_list + new_errors:
        old_dl += len(clause) + 1
    print("Old DL\t: " + str(old_dl))

    new_dl = 0
    for clause in old_clause_list + old_errors:
        new_dl += len(clause) + 1
    print("New DL\t: " + str(new_dl))

    return new_dl - old_dl


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
            if len(itids) >= self.minsup:
                # if len(prefix) > 0:  # Only store patterns with length > 1
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
        prefix = []
        dict_id = 0

        data = self.read_data(input)
        sorted_data = sorted(data.items(), key=lambda item: len(item[1]), reverse=True)
        self.eclat(prefix, sorted_data, dict_id)

        # print("Frequent Itemsets\t:")
        # for item, clause_ids in self.freq_items.items():
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print()

        # sort freq items descending wrt change in DL (wrt W-op), length of the itemset, frequency
        self.freq_items = [(k, v) for k, v in self.freq_items.items()]

        self.freq_items.sort(
            key=lambda item: (
                len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                len(item[0]),
                len(item[1]),
            ),
            reverse=True,
        )

        # print("Frequent Itemsets\t:")
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

    def get_literal_length(self, cnf):
        """
        :param cnf: a CNF/Theory
        :return: the total number of literals in the whole theory
        """
        return sum([len(clause) for clause in cnf])

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
        self.theory.intialize(self.positives, self.negatives, 2)
        self.theory.errors = self.theory.get_violations(self.theory.positives)

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory.theory_length = self.total_negatives

        while True:
            success = self.theory.compress_theory()
            if success == "ignore_itemset":
                del self.theory.freq_items[0]
            elif not success:
                break

            # print(
            #     "--------------------------Frequent Itemsets--------------------------\t:"
            # )
            # for item, clause_ids in self.theory.freq_items:
            #     print(" {}\t: {}".format(list(item), clause_ids))
            # print("\n")

        return self.theory


class Theory:
    def __init__(self, clauses, overlap_matrix, search_index):
        self.clauses = clauses
        self.overlap_matrix = overlap_matrix
        self.search_index = search_index
        self.clause_length = []  # List of lengths of all the clauses of this theory
        self.total_compression = 0
        self.theory_length = len(clauses)  # Total number of clauses in the theory

        self.freq_items = dict()
        self.errors = set()
        self.positives = set()
        self.minsup = None

        self.operator_counter = {"W": 0, "V": 0, "S": 0, "R": 0, "T": 0}
        self.new_var_counter = None

    def intialize(self, positives, negatives, minsup):
        # Construct a theory from the partial assignments
        for pa in negatives:
            self.clauses.append(frozenset([-literal for literal in pa]))
            self.clause_length.append(len(pa))

        self.minsup = minsup
        eclat = Eclat(minsup=minsup)
        self.freq_items = eclat.get_Frequent_Itemsets(self.clauses)
        self.positives = positives

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

    def get_violations(self, positives, print_violations=True):
        """
        Get the number of positive partial assignments violated by the given theory.
        :param positives:
        :param print_violations:
        :return:
        """
        violated_pos = set()

        for pos in positives:
            if not check_pa_satisfiability(pos, self.clauses):
                violated_pos.add(pos)

        if print_violations & len(violated_pos) > 0:
            print(
                len(violated_pos),
                " violations of positive partial assignments found for the learned theory.",
            )
        return violated_pos

    def get_compression(self, input_clauses, op, output_clauses):

        # dl = sum(theory.clause_length) + theory.theory_length - 1
        # for misclassified_pa in theory.errors:
        #     dl += len(misclassified_pa)
        # dl += len(theory.errors)
        # print("Description Length of theory\t:" + str(dl))

        new_theory = copy(self.clauses)
        for clause in input_clauses:
            new_theory.remove(clause)
        new_theory += list(output_clauses)

        uncovered_positives = set()
        compression = 0

        if op in {"V", "T"}:
            for pa in self.positives:
                if not check_pa_satisfiability(pa, new_theory):
                    uncovered_positives.add(pa)
                    compression -= len(pa) + 1

        for clause in input_clauses:
            compression += len(clause) + 1

        for clause in output_clauses:
            compression -= len(clause) + 1

        return uncovered_positives, compression

    def apply_best_operator(self, input_clause_list, possible_operations):
        success = "ignore_itemset"
        operator_precedence = ["S", "R", "W", "V", "T", None]
        max_compression = 0
        best_operator = None
        new_errors = None
        output_clause_list = None
        for op, output_clauses in possible_operations:
            errors, compression = self.get_compression(
                input_clause_list, op, output_clauses
            )
            if compression > max_compression or (
                compression == max_compression
                and operator_precedence.index(op)
                < operator_precedence.index(best_operator)
            ):
                max_compression = compression
                best_operator = op
                new_errors = errors
                output_clause_list = list(output_clauses)
                success = True

        if success is True:
            # Update old_theory
            self.operator_counter[best_operator] += 1
            if best_operator == "W":
                new_var = self.get_new_var()
                for i, clause in enumerate(output_clause_list):
                    if "#" in clause:
                        output_clause_list[i].remove("#")
                        output_clause_list[i].add(new_var)
                    else:
                        output_clause_list[i].remove("-#")
                        output_clause_list[i].add(-new_var)
                    output_clause_list[i] = frozenset(output_clause_list[i])

            self.errors |= new_errors
            self.positives -= new_errors
            self.total_compression += max_compression
            self.theory_length += len(output_clause_list) - len(input_clause_list)

            pruned_indices = list(self.freq_items[0][1])

            # Delete indices that are in pruned indices and decrease indices of other clauses correspondingly
            buffer = copy(pruned_indices)
            # print("Initial Buffer\t: " + str(buffer))
            decrement = 0
            replace_dict = {}
            for i in range(len(self.clauses)):
                if i < buffer[0]:
                    replace_dict[i] = i - decrement
                elif i == buffer[0]:
                    del buffer[0]
                    decrement += 1
                    if len(buffer) == 0:
                        break

            # print("Final Buffer\t: " + str(buffer))
            # print("Replace Dict\t: " + str(replace_dict))

            # Replace the indices of clauses wrt the replacement dictionary
            increment = len(replace_dict)
            new_freq_items = []
            for itemset, clause_ids in self.freq_items[1:]:

                new_clause_ids = set()
                # Repalce ids of old existing clauses
                for clause_id in clause_ids:
                    if clause_id in replace_dict:
                        new_clause_ids.add(replace_dict[clause_id])

                # Add ids of new clause covering this itemset
                for i, clause in enumerate(output_clause_list):
                    if itemset.issubset(clause):
                        new_clause_ids.add(i + increment)

                if len(new_clause_ids) >= self.minsup:
                    new_freq_items.append((itemset, new_clause_ids))

            # print("Old Clauses\t: " + str(input_clause_list))
            # print("New Clauses\t: " + str(output_clause_list))
            # print("New Frequent Itemsets\t:")
            # for item, clause_ids in new_freq_items:
            #     print(" {}\t: {}".format(list(item), clause_ids))
            # print()

            self.freq_items = new_freq_items

            pruned_indices.sort(reverse=True)
            for i in pruned_indices:
                del self.clauses[i]
                del self.clause_length[i]

            for clause in output_clause_list:
                self.clauses.append(clause)
                self.clause_length.append(len(clause))

        return success

    def compress_theory(self):
        # TODO: Make it more efficient once it is complete. Reduce the number of iterations on old_clause_list/residue
        if len(self.freq_items) == 0:
            return False
        subclause, clause_ids = self.freq_items[0]
        if len(clause_ids) == 1:
            return "ignore_itemset"

        old_clause_list = []  # A list of input clauses (to be compressed)
        residue = []  # A list of input clauses without the shared subclause
        possible_operations = []

        for clause_id in clause_ids:
            clause = self.clauses[clause_id]
            old_clause_list.append(clause)
            residue.append(clause - subclause)

        # Check if S-operator is applicable
        for clause in old_clause_list:
            if clause == subclause:
                # S-operator is applicable
                possible_operations.append(("S", [subclause]))
                # self.operator_counter["S"] += 1
                # return True

        possible_operations.append(("T", [subclause]))

        # Check if R-operator is applicable
        r_residue = [tuple(clause) for clause in residue]
        if solve(r_residue) == "UNSAT":
            # R-operator is applicable (R-operator is a special case when applying T-operator is lossless.
            # Example: If input is [(a,b), (a,c), (a,-b,-c)], then the output is [(a)].
            # Here, then input is logically equivalent to the output (Lossless Truncation).
            possible_operations.append(("R", [subclause]))
            # return True

        # Check if V-operator is applicable
        v_literals = []
        for clause in residue:
            if len(clause) == 1:
                v_literals.append(next(iter(clause)))
                break

        if v_literals:
            for v_literal in v_literals:
                v_output = []
                for clause in residue:
                    if len(clause) == 1 and v_literal == next(iter(clause)):
                        new_clause = set(subclause)
                        new_clause.add(v_literal)
                        v_output.append(frozenset(new_clause))
                    else:
                        new_clause = set(clause)
                        new_clause.add(-v_literal)
                        v_output.append(frozenset(new_clause))
                possible_operations.append(("V", v_output))

        # Consider W-operator

        # Use '#' as a special newly invented variable.
        # If/once W-operator is accepted for compression, it will be replaced by a self.new_var_counter()
        new_clause = set(subclause)
        new_clause.add("#")
        w_output = [new_clause]

        for clause in residue:
            new_clause = set(clause)
            new_clause.add("-#")
            w_output.append(new_clause)

        possible_operations.append(("W", w_output))

        return self.apply_best_operator(old_clause_list, possible_operations)


if __name__ == "__main__":
    positives, negatives = load_test2()
    mistle = Mistle(positives, negatives)
    theory = mistle.learn()
    print("Final Theory\t: " + str(theory.clauses))
    initial_dl = -1
    for pa in positives + negatives:
        initial_dl += 1
        initial_dl += len(pa)

    final_dl = get_description_length(theory)
    compression2 = (initial_dl - final_dl) / float(initial_dl)
    print("Initial DL\t: " + str(initial_dl))
    print("Final DL\t: " + str(final_dl))
    print("Compression (wrt Description Length)\t: " + str(compression2))
