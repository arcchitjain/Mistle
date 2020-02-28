from tqdm import tqdm
from copy import copy
from time import time
import numpy as np
from pycosat import solve
from collections import OrderedDict

np.set_printoptions(linewidth=150)


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


def load_test1():
    # a = 1, b = (2, 3, 4), c = (5, 6)
    input_cnf = [(1, 2, 3, 4), (2, 3, 4, 5, 6)]
    output_cnf = [(-1, 5, 6), (2, 3, 4, 5, 6)]

    truth_assignments = []
    for a in [-1, 1]:
        for b1 in [-2, 2]:
            for b2 in [-3, 3]:
                for b3 in [-4, 4]:
                    for c1 in [-5, 5]:
                        for c2 in [-6, 6]:
                            truth_assignments.append((a, b1, b2, b3, c1, c2))

    pos2pos = []
    neg2neg = []
    pos2neg = []
    neg2pos = []

    for ass in truth_assignments:
        i = input_cnf + [(a,) for a in ass]
        o = output_cnf + [(a,) for a in ass]

        s_i = 0 if solve(i) == "UNSAT" else 1
        s_o = 0 if solve(o) == "UNSAT" else 1

        if s_i == 1 and s_o == 1:
            pos2pos.append(ass)
        elif s_i == 0 and s_o == 0:
            neg2neg.append(ass)
        elif s_i == 1 and s_o == 0:
            pos2neg.append(ass)
        elif s_i == 0 and s_o == 1:
            neg2pos.append(ass)

        print(ass, s_i, s_o)

    print("\nPos2Pos\t:", len(pos2pos))
    for p in pos2pos:
        print(p)

    print("\nNeg2Neg\t:", len(neg2neg))
    for n in neg2neg:
        print(n)

    print("\nPos2Neg\t:", len(pos2neg))
    for p in pos2neg:
        print(p)

    print("\nNeg2Pos\t:", len(neg2pos))
    for n in neg2pos:
        print(n)

    positives = [frozenset(ass) for ass in pos2pos]
    negatives = [frozenset([-1, -2, -3, -4]), frozenset([-2, -3, -4, -5, -6])]
    # positives = [frozenset(ass) for ass in pos2pos + pos2neg]
    # negatives = [frozenset(ass) for ass in neg2neg + neg2pos]

    print("\nPositives\t:", len(positives))
    for p in positives:
        print(p)

    print("\nNegatives\t:", len(negatives))
    for n in negatives:
        print(n)

    return positives, negatives


def load_test2():
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
        partial_assignmemnt = set()
        for i, j in enumerate(var_range):
            if str(j) in row[:-1]:
                partial_assignmemnt.add(i + 1)
            elif negation:
                # Using closed world assumption to register absent variables as if they are false.
                partial_assignmemnt.add(-(i + 1))

        if (not switch_signs and row[-1] == target_class[0]) or (
            switch_signs and row[-1] == target_class[1]
        ):
            negative_pas.append(frozenset(partial_assignmemnt))
            neg_freq += 1
            if load_top_k and len(negative_pas) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not switch_signs and row[-1] == target_class[1]) or (
            switch_signs and row[-1] == target_class[0]
        ):
            positive_pas.append(frozenset(partial_assignmemnt))
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

    # # Remove redundant partial assignments
    # positive_pas = set(positive_pas)
    # negative_pas = set(negative_pas)
    #
    # # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
    # inconsistent_pas = positive_pas & negative_pas
    # positive_pas = positive_pas - inconsistent_pas
    # negative_pas = negative_pas - inconsistent_pas
    #
    # input_literal_length = get_literal_length(negative_pas)
    # input_bit_length = get_bit_length_for_theory(negative_pas)
    #
    # print(
    #     "\nInput Theory (without redundancies and inconsistencies):\n\tLiteral Length = "
    #     + str(input_literal_length)
    #     + "\n\tBit Length = "
    #     + str(input_bit_length)
    #     + "\n\tNumber of clauses = "
    #     + str(len(negative_pas))
    # )
    return (positive_pas, negative_pas)

    # # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
    # new_positive_pas = []
    # for pa in positive_pas:
    #     if pa not in negative_pas:
    #         new_positive_pas.append(pa)
    #
    # new_negative_pas = []
    # for pa in negative_pas:
    #     if pa not in positive_pas:
    #         new_negative_pas.append(pa)
    #
    # input_literal_length = get_literal_length(new_negative_pas)
    # input_bit_length = get_bit_length_for_theory(new_negative_pas)
    #
    # print(
    #     "\nInput Theory (only without inconsistencies):\n\tLiteral Length = "
    #     + str(input_literal_length)
    #     + "\n\tBit Length = "
    #     + str(input_bit_length)
    #     + "\n\tNumber of clauses = "
    #     + str(len(negative_pas))
    # )
    #
    # return (new_positive_pas, new_negative_pas)


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


def get_subclauses(clause1, clause2):
    clause_a = copy(set(clause1 - clause2))
    clause_b = copy(set(clause1 & clause2))
    clause_c = copy(set(clause2 - clause1))
    return clause_a, clause_b, clause_c


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


def check_pa_satisfiability(pa, theory):

    theory_cnf = [tuple(clause) for clause in theory]

    cnf = theory_cnf + [(a,) for a in pa]

    return not (solve(cnf) == "UNSAT")


def convert_to_clause(partial_assignment):
    """
    Negate the terms in the input frozenset to convert them from a partial assignment to a clause.
    Example:
        input: frozenset({-1*v1, -1*v2, -1*v3, -1*v4, -1*v5, -1*v6, -1*v7, -1*v8})
        output: frozenset({v1, v2, v3, v4, v5, v6, v7, v8})
    :param partial_assignment: Every term in the frozenset is in a conjunction with one another
    :return: clause: Every term in the frozenset is in a disjunction with one another
    """
    clause = set()
    for term in partial_assignment:
        clause.add(-term)
    return frozenset(clause)


class Mistle:
    def __init__(self, positives, negatives):

        self.initial_positives = copy(positives)
        self.initial_negatives = copy(negatives)

        self.positives = positives
        self.negatives = negatives

        self.total_positives = len(positives)
        self.total_negatives = len(negatives)
        self.theory_length = len(negatives)

        self.operator_counter = {"W": 0, "V": 0, "S": 0, "R": 0}
        self.invented_predicate_definition = OrderedDict()

        self.theory = []
        self.clause_length = []
        self.overlap_matrix = []

        self.new_var_counter = 0
        for pa in positives + negatives:
            self.new_var_counter = max(self.new_var_counter, max([abs(l) for l in pa]))
        self.new_var_counter += 1
        # print("new_var_counter\t:", self.new_var_counter)

        self.search_index = 0
        self.update_dict = {}
        self.update_threshold = 1

    def get_new_var(self):
        """
        Invent a new predicate
        :return: An integer (updated counter) which has not been used as a literal before
        """
        new_var = self.new_var_counter
        self.new_var_counter += 1
        return new_var

    def convert_to_theory(self, partial_assignments):
        """
        Negate the disjunctive set of partial assignments to get a conjunctive set of clauses (theory)
        :param partial_assignments: List/Set of frozensets of terms where each frozenset is a partial assignment
        :return: List of frozensets of terms where each frozenset is a clause
        """
        for pa in partial_assignments:
            self.theory.append(convert_to_clause(pa))
            self.clause_length.append(len(pa))

        return self.theory

    def sort_theory(self):

        # # Numpy Implementation
        # self.clause_length = np.array(self.clause_length)
        # index = self.clause_length.argsort()[::-1]
        # self.clause_length = self.clause_length[index]

        # Non-numpy Implementation
        index = list(range(self.theory_length))
        index.sort(key=self.clause_length.__getitem__, reverse=True)
        self.clause_length = [self.clause_length[i] for i in index]

        self.theory = [self.theory[i] for i in index]

    def get_literal_length(self, cnf=None):
        """
        :param theory:
        :return: the total number of literals in the whole theory
        """
        if cnf is None:
            cnf = self.theory

        length = 0
        for clause in cnf:
            length += len(clause)

        return length

    def check_clause_validity(self, input_clause1, input_clause2, output_clauses):
        """
        :param clauses: List of new clauses that can be added to the theory
        :return:
            True    if the new theory is consistent with the data
            False   otherwise
        """

        new_theory = copy(self.theory)
        new_theory.remove(input_clause1)
        new_theory.remove(input_clause2)
        new_theory += list(output_clauses)

        for pa in self.positives:
            if not check_pa_satisfiability(pa, new_theory):
                return False

        for pa in self.negatives:
            if check_pa_satisfiability(pa, new_theory):
                return False

        return True

    def check_clause_validity1(self, clause1, clause2):
        """
        :param clause1: 1st clause for V-operator
        :param clause2: 2nd clause for V-operator
        :return:
            True    if all positives are valid in the SAT Theory
            False   if at least one of the positives is not True in that theory
        """

        clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
        clause_a = tuple(clause_a)
        clause_b = tuple(clause_b)
        clause_c = tuple(clause_c)
        l_a = len(clause_a)
        l_b = len(clause_b)
        l_c = len(clause_c)

        if (l_a == 1 or l_c == 1) and l_b > 1:

            if l_a == 1:
                # V-operator is applied on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
                # To get (a; b1; b2; b3), (c1; c2; c3; -a)

                # Create a CNF to represent the datapoints moving from positive to negative after V-operator
                # Positive Loss = a, (b1; b2; b3), -c1, -c2, -c3
                pos_cnf = [clause_a, clause_b]
                for c in clause_c:
                    pos_cnf.append((-c,))

                common_part = [-clause_a[0]] + [c for c in clause_c]
                pos_cnf_c = []
                for i, b in enumerate(clause_b):
                    component = copy(common_part) + [-b]
                    pos_cnf_c.append(component)

                for pos_pa in self.positives:
                    # if (
                    #     check_pa_satisfiability(pos_pa, [clause_a])
                    #     and check_pa_satisfiability(pos_pa, [clause_b])
                    #     and not check_pa_satisfiability(pos_pa, [clause_c])
                    # ):
                    if not check_pa_satisfiability(pos_pa, pos_cnf_c):
                        # if check_pa_satisfiability(pos_pa, pos_cnf):
                        # Loss of a positive partial assignment encountered.
                        return False

            elif len(clause_c) == 1:
                # V-operator is applied on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
                # To get (a1; a2; a3; -c), (b1; b2; b3; c)

                # Create a CNF to represent the datapoints moving from positive to negative after V-operator
                # Positive Loss = -a1, -a2, -a3, (b1; b2; b3), c
                pos_cnf = [clause_c, clause_b]
                for a in clause_a:
                    pos_cnf.append((-a,))

                common_part = [-clause_c[0]] + [a for a in clause_a]
                pos_cnf_c = []
                for i, b in enumerate(clause_b):
                    component = copy(common_part) + [-b]
                    pos_cnf_c.append(component)

                for pos_pa in self.positives:
                    # if (
                    #     not check_pa_satisfiability(pos_pa, [clause_a])
                    #     and check_pa_satisfiability(pos_pa, [clause_b])
                    #     and check_pa_satisfiability(pos_pa, [clause_c])
                    # ):
                    if not check_pa_satisfiability(pos_pa, pos_cnf_c):
                        # if check_pa_satisfiability(pos_pa, pos_cnf):
                        # Loss of a positive partial assignment encountered.
                        return False

            return True

        else:
            return True

    def count_violations(
        self, positives=None, negatives=None, sign=None, print_violations=True
    ):

        start_time = time()
        pos_counter = 0
        neg_counter = 0
        violated_pos = []
        violated_neg = []

        if positives is None:
            positives = self.initial_positives

        if negatives is None:
            negatives = self.initial_negatives

        if sign is None or sign == "+":

            for pos in positives:
                if not check_pa_satisfiability(pos, self.theory):
                    violated_pos.append(pos)
                    pos_counter += 1

            if pos_counter > 0:
                print(
                    pos_counter,
                    " violations of positive partial assignments found for the learned theory.",
                )
                # if print_violations:
                #     print_2d(violated_pos)
                #     print()

        elif sign is None or sign == "-":

            for neg in negatives:
                if check_pa_satisfiability(neg, self.theory):
                    violated_neg.append(neg)
                    neg_counter += 1

            if neg_counter > 0:
                print(
                    neg_counter,
                    " violations of negative partial assignments found for the learned theory.",
                )
                # if print_violations:
                #     print_2d(violated_neg)
                #     print()

        # print(
        #     "Time taken in counting violations (Sign = "
        #     + str(sign)
        #     + ") = "
        #     + str(time() - start_time)
        #     + " seconds"
        # )
        return pos_counter + neg_counter, violated_pos, violated_neg

    def get_overlap_matrix(self):
        """
        Construct a triangular matrix that stores overlap of ith and jth clause at (i,j) position
        :return: A triangular matrix that stores overlap of ith and jth clause at (i,j) position
        """

        n = len(self.theory)
        for i, clause1 in enumerate(self.theory):
            self.overlap_matrix.append([0] * n)
            for j, clause2 in enumerate(self.theory[i + 1 :]):
                self.overlap_matrix[i][i + j + 1] = len(clause1 & clause2)

        # # Numpy Implementation
        # return np.array(self.overlap_matrix)

        # Non-numpy Implementation
        return self.overlap_matrix

    def select_clauses(self, prev_overlap_size):
        """
        Select clauses for next compression step WITH a compression matrix
        :param prev_overlap_size: Previous Overlap Size
        :return:
            max_overlap_indices: The indices of 2 clauses that have the maximum overlap size
            max_overlap_size: The maximum overlap size
        """

        max_overlap_size = 0
        max_overlap_indices = (0, 0)
        # Only search the overlap matrix (After self.search_index)
        for i, row in enumerate(self.overlap_matrix[self.search_index :]):
            for j, overlap in enumerate(row[self.search_index + i + 1 :]):
                if overlap > max_overlap_size:
                    max_overlap_size = overlap
                    max_overlap_indices = (
                        self.search_index + i,
                        self.search_index + i + 1 + j,
                    )
                    if prev_overlap_size and max_overlap_size >= prev_overlap_size:
                        break

        if prev_overlap_size and max_overlap_size < prev_overlap_size:
            # Search the initial part of the matrix too (Before self.search_index)

            max_overlap_size1 = 0
            max_overlap_indices1 = (0, 0)
            for i, row in enumerate(self.overlap_matrix[: self.search_index]):
                for j, overlap in enumerate(row[i + 1 : self.search_index]):
                    if overlap > max_overlap_size1:
                        max_overlap_size1 = overlap
                        max_overlap_indices1 = (i, i + 1 + j)

            if max_overlap_size1 >= max_overlap_size:
                max_overlap_indices = max_overlap_indices1
                max_overlap_size = max_overlap_size1

        self.search_index = max_overlap_indices[0]

        return max_overlap_indices, max_overlap_size

    def delete_clauses(self, indices):

        # # Numpy Implementation
        # self.clause_length = np.delete(self.clause_length, indices)
        # self.overlap_matrix = np.delete(self.overlap_matrix, indices[1], 0)
        # self.overlap_matrix = np.delete(self.overlap_matrix, indices[0], 0)
        # self.overlap_matrix = np.delete(self.overlap_matrix, indices[1], 1)
        # self.overlap_matrix = np.delete(self.overlap_matrix, indices[0], 1)

        # Non-numpy implementation
        del self.clause_length[indices[1]]
        del self.clause_length[indices[0]]

        del self.overlap_matrix[indices[1]]
        del self.overlap_matrix[indices[0]]
        for i in range(self.theory_length - 2):
            del self.overlap_matrix[i][indices[1]]
            del self.overlap_matrix[i][indices[0]]

        del self.theory[indices[1]]
        del self.theory[indices[0]]

        self.theory_length -= 2

    def insert_clause(self, clause):

        # # Numpy Implementation
        # index = np.searchsorted(-self.clause_length, -len(clause), side="right")
        #
        # # Insert zero rows and columns in overlap_matrix at index
        # self.overlap_matrix = np.insert(
        #     self.overlap_matrix, index, np.zeros(len(self.overlap_matrix)), 0
        # )
        # self.overlap_matrix = np.insert(
        #     self.overlap_matrix, index, np.zeros(len(self.overlap_matrix)), 1
        # )
        #
        # self.theory[index:index] = [clause]
        #
        # for i, clause2 in enumerate(self.theory):
        #     if i == index:
        #         continue
        #     overlap = len(clause & clause2)
        #     if i < index:
        #         self.overlap_matrix[i][index] = overlap
        #     else:
        #         self.overlap_matrix[index][i] = overlap
        #
        # self.clause_length = np.insert(self.clause_length, index, len(clause))

        # Non-numpy implementation
        i = int(self.theory_length / 2)
        start = 0
        end = self.theory_length
        index_found = False
        index = None
        l = len(clause)
        while i < self.theory_length:
            if l <= self.clause_length[-1]:
                # Insert element at the end of the list
                index = self.theory_length
                index_found = True
                break
            elif l >= self.clause_length[0]:
                # Insert element around the beginning at the place when the length drops right after
                for j, cl in enumerate(self.clause_length):
                    if cl < l:
                        index = j
                        break
                index_found = True
                break
            elif self.clause_length[i] == l:
                for j, cl in enumerate(self.clause_length[i + 1 :]):
                    if cl < l:
                        index = j + i + 1
                        break
                index_found = True
                break
            elif i + 1 < len(self.clause_length) and self.clause_length[i + 1] == l:
                for j, cl in enumerate(self.clause_length[i + 2 :]):
                    if cl < l:
                        index = j + i + 2
                        break
                index_found = True
                break
            elif (
                i + 1 < len(self.clause_length)
                and self.clause_length[i] > l > self.clause_length[i + 1]
            ):
                # Insert element at/before (i + 1) if
                # clause_length[i] >= len(clause) >= clause_length[i + 1]
                index = i + 1
                index_found = True
                break
            elif l > self.clause_length[i]:
                end = i
                i = int((i + start) / 2)
            elif l < self.clause_length[i]:
                start = i
                i = int((end + i) / 2)
            else:
                i += 1

        if index_found is False:
            # This is the case where a clause is being inserted to an empty theory
            index = 0

        new_overlap_row = [0] * (self.theory_length + 1)
        self.overlap_matrix[index:index] = [new_overlap_row]

        self.theory[index:index] = [clause]

        for j, clause2 in enumerate(self.theory):
            if j == index:
                continue

            overlap = len(clause & clause2)

            if j < index:
                self.overlap_matrix[j][index:index] = [overlap]
                self.overlap_matrix[index][j] = 0
            elif j > index:
                self.overlap_matrix[j][index:index] = [0]
                self.overlap_matrix[index][j] = overlap

        self.clause_length[index:index] = [len(clause)]

        if index < self.search_index:
            self.search_index = index
        # else:
        #     # print(np.array(self.clause_length))
        #     prev_l = self.clause_length[0]
        #     for l in self.clause_length[1:]:
        #         if l < prev_l:
        #             prev_l = l
        #         elif l > prev_l:
        #             print(np.array(self.clause_length))
        #             break

        # self.search_index = min(self.search_index, index)
        self.theory_length += 1

    def update_clause(self, new_var, clause):
        """
        Update the theory to replace every occurrence of 'clause' by 'new_var'
        :param new_var:
        :param clause:
        :return:
        """
        self.update_dict[new_var] = clause

        if len(self.update_dict) == self.update_threshold:
            clause_list = []
            new_var_list = []
            clause_lengths = []
            for k, v in self.update_dict.items():
                new_var_list.append(k)
                clause_list.append(v)
                clause_lengths.append(len(v))

            index = list(range(self.update_threshold))
            index.sort(key=clause_lengths.__getitem__, reverse=True)

            clause_list = [clause_list[i] for i in index]
            new_var_list = [new_var_list[i] for i in index]

            delete_indices = []
            updated_clauses = []
            for i, clause in enumerate(self.theory):

                for new_var, clause_def in zip(new_var_list, clause_list):
                    if clause_def.issubset(clause):
                        new_clause = set(clause) - clause_def
                        new_clause.add(-new_var)

                        updated_clauses.append(new_clause)
                        delete_indices.append(i)
                        break

            if len(delete_indices) > 0:
                print("Other clause found in theory that has the subset.")

            # Delete the old clauses from the theory
            delete_indices = delete_indices[::-1]
            for index in delete_indices:
                del self.clause_length[index]

                del self.overlap_matrix[index]
                for i in range(self.theory_length - 1):
                    del self.overlap_matrix[i][index]

                del self.theory[index]

                self.theory_length -= 1

            # Insert the updated clauses in the theory
            for clause in updated_clauses:
                self.insert_clause(clause)

            self.update_dict = {}

    def compress_pairwise(self, clause1, clause2, lossless=False):
        """
        Compress clause1 and clause2
        :param clause1:
        :param clause2:
        :param lossless: Apply V-operator only if lossless=False
        :return:
            A list of the new clauses obtained after compression
            The decrease in literal length achieved during compression
            A Boolean variable that is:
                True    if a lossless compression step (W-operator, Subsumption) took place
                False   if a lossy compression step (V-operator) took place
                None    if no operator is applied
        """

        clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
        resolution_applicable = False
        if (
            len(clause_a) == 1
            and len(clause_c) == 1
            and copy(clause_a).pop() == -copy(clause_c).pop()
        ):
            resolution_applicable = True

        if len(clause_a) == 0:
            # Apply subsumption operator on (b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            self.operator_counter["S"] += 1
            return {clause1}, len(clause_b), True

        elif len(clause_c) == 0:
            # Apply subsumption operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3)
            self.operator_counter["S"] += 1
            return {clause2}, len(clause_b), True

        elif resolution_applicable:
            # Apply complementation operator on (a; b1; b2; b3), (b1; b2; b3; -a)
            # Return (b1; b2; b3)
            self.operator_counter["R"] += 1
            return {frozenset(clause_b)}, len(clause_b) + 2, True

        elif len(clause_b) == 0:
            # Cannot compress (a1; a2; a3), (c1; c2; c3)
            print_1d(clause1)
            print_1d(clause2)
            print("No Overlap Found")
            return {clause1, clause2}, 0, None

        elif not lossless and len(clause_a) == 1 and len(clause_b) > 1:
            # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            # Return (a; b1; b2; b3), (c1; c2; c3; -a)
            a = clause_a.pop()
            clause_c.add(-a)
            self.operator_counter["V"] += 1
            return {clause1, frozenset(clause_c)}, len(clause_b) - 1, False

        elif not lossless and len(clause_c) == 1 and len(clause_b) > 1:
            # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
            # Return (a1; a2; a3; -c), (b1; b2; b3; c)
            c = clause_c.pop()
            clause_a.add(-c)
            self.operator_counter["V"] += 1
            return {frozenset(clause_a), clause2}, len(clause_b) - 1, False

        elif len(clause_b) < 3:
            # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + "
            # literals.")
            return {clause1, clause2}, 0, None

        else:
            # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
            # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)

            # Invent a new predicate if the definition of a predicate is new.
            if clause_b not in self.invented_predicate_definition.values():
                new_var = self.get_new_var()
                clause_a.add(-new_var)
                self.invented_predicate_definition[new_var] = copy(clause_b)
                clause_b.add(new_var)
                clause_c.add(-new_var)

                # Update other clauses in the theory that contain 'clause_b'
                # start_time = time()
                self.update_clause(new_var, clause_b)
                # print(time() - start_time)
            else:
                for var, clause in self.invented_predicate_definition.items():
                    if clause == clause_b:
                        clause_a.add(-var)
                        clause_b.add(var)
                        clause_c.add(-var)

            # new_var = self.get_new_var()
            # clause_a.add(-new_var)
            # self.invented_predicate_definition[new_var] = copy(clause_b)
            # clause_b.add(new_var)
            # clause_c.add(-new_var)

            self.operator_counter["W"] += 1

            return (
                {frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)},
                len(clause_b) - 3,
                True,
            )

    def learn(self, lossless=False, description_measure="ll"):

        start_time = time()

        input_literal_length = self.get_literal_length(self.negatives)
        input_length = -1
        output_length = -1
        # input_bit_length = get_bit_length_for_theory(negatives)

        print(
            "\nInput Theory (with redundancies and inconsistencies):\n\tLiteral Length = "
            + str(input_literal_length)
            # + "\n\tBit Length = "
            # + str(input_bit_length)
            + "\n\tNumber of clauses = "
            + str(self.total_negatives)
        )

        # Remove redundant partial assignments
        self.positives = set(self.positives)
        self.negatives = set(self.negatives)

        # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
        inconsistent_pas = self.positives & self.negatives
        self.positives = self.positives - inconsistent_pas
        self.negatives = self.negatives - inconsistent_pas

        print(
            "\nInput Theory (without redundancies and inconsistencies):\n\tLiteral Length = "
            + str(self.get_literal_length(self.negatives))
            # + "\n\tBit Length = "
            # + str(get_bit_length_for_theory(negatives))
            + "\n\tNumber of clauses = "
            + str(len(self.negatives))
        )

        if description_measure == "ll":
            input_length = input_literal_length
        # elif description_measure == "se":
        #     input_length = input_bit_length

        # Convert the set of -ve PAs to a theory
        self.theory = self.convert_to_theory(self.negatives)

        initial_violations, positive_violations, negative_violations = self.count_violations(
            self.positives, self.negatives, sign="+", print_violations=False
        )

        if len(positive_violations) > 0:
            # Ignore Positive Violations while calculating the loss
            self.positives = copy(self.positives - set(positive_violations))

            # initial_violations, positive_violations, negative_violations = count_violations(
            #     positives, negatives, theory, sign="+"
            # )

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory_length = self.total_negatives

        # print_2d(theory)
        self.sort_theory()

        self.overlap_matrix = self.get_overlap_matrix()

        overlap_size = None
        # search_index = 0
        compression_counter = 0
        catch_it = 0
        pos2neg = 0
        neg2pos = 0

        pbar = tqdm(total=int(1.1 * self.theory_length + 100))
        pbar.set_description("Compressing Clauses")
        while True:

            prev_overlap_size = overlap_size
            max_overlap_indices, overlap_size = self.select_clauses(prev_overlap_size)

            # print("Max overlapping indices = " + str(max_overlap_indices))
            # print(
            #     "[" + str(compression_counter) + "]",
            #     str(prev_overlap_size) + " --> " + str(overlap_size),
            #     self.search_index,
            # )

            if overlap_size < 2:
                print("Cannot compress anymore: Max overlap size < 2.")
                break

            clause1 = self.theory[max_overlap_indices[0]]
            clause2 = self.theory[max_overlap_indices[1]]

            (
                compressed_clauses,
                compression_size,
                is_lossless,
            ) = self.compress_pairwise(clause1, clause2, lossless=lossless)
            # print(compression_counter, compressed_clauses, compression_size, is_lossless)
            # print(overlap_size, compression_size, str(operator_counter))
            if overlap_size - compression_size > 2:
                print(
                    "------------------------------- Compressing is not efficient! -------------------------------"
                )
                print("Length of clause 1\t:", len(clause1))
                print_1d(clause1)
                print("Length of clause 2\t:", len(clause2))
                print_1d(clause2)
                print("Overlap = " + str(len(clause1 & clause2)))
                clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
                print(
                    "len(a) = "
                    + str(len(clause_a))
                    + ";\tlen(b) = "
                    + str(len(clause_b))
                    + ";\tlen(c) = "
                    + str(len(clause_c))
                )

                print_2d(self.theory)
                # print_2d(overlap_matrix)

            compression_counter += 1

            if len(compressed_clauses) == 2 and compression_size == 0:
                # Theory cannot be compressed any further
                # overlap_matrix[max_overlap_indices[0]][max_overlap_indices[1]] = -1
                # continue
                # Initialize search index
                print("Cannot compress the theory any further.")
                break
            elif is_lossless:
                # V - Operator is not applied
                # Continue compressing the theory
                # Delete the clauses used for last compression step
                self.delete_clauses(max_overlap_indices)

                # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
                # theory |= compressed_clauses
                for clause in compressed_clauses:
                    self.insert_clause(clause)

            else:
                clause_validity_slow = self.check_clause_validity(
                    clause1, clause2, compressed_clauses
                )
                # clause_validity_fast = self.check_clause_validity1(clause1, clause2)
                #
                # if clause_validity_fast != clause_validity_slow:
                #     catch_it += 1
                #     if clause_validity_fast:
                #         neg2pos += 1
                #     else:
                #         pos2neg += 1
                #     # clause_validity_fast = self.check_clause_validity1(clause1, clause2)

                if clause_validity_slow:
                    self.delete_clauses(max_overlap_indices)

                    for clause in compressed_clauses:
                        self.insert_clause(clause)

                elif not clause_validity_slow:
                    # Perform a lossless step here, i.e., Apply W-operator instead of V-operator
                    self.operator_counter["V"] -= 1

                    (
                        compressed_clauses,
                        compression_size,
                        is_lossless,
                    ) = self.compress_pairwise(clause1, clause2, lossless=True)

                    assert is_lossless is True

                    self.delete_clauses(max_overlap_indices)

                    for clause in compressed_clauses:
                        self.insert_clause(clause)

            # print_2d(self.theory)
            # print_2d(self.overlap_matrix)
            pbar.update(1)

        pbar.close()
        output_literal_length = self.get_literal_length()
        # output_bit_length = get_bit_length_for_theory(theory)

        if description_measure == "ll":
            output_length = output_literal_length
        # elif description_measure == "se":
        #     output_length = output_bit_length
        compression = round((1 - output_length / input_length) * 100, 2)

        print(
            "\nResultant Theory:\n\tLiteral Length = "
            + str(output_literal_length)
            # + "\n\tBit Length = "
            # + str(output_bit_length)
            + "\n\tLength of Theory = "
            + str(self.theory_length)
            + "\n\t% Compression (For measure: "
            + str(description_measure)
            + ") = "
            + str(compression)
            + "%"
            + "\n\tOperator Count = "
            + str(self.operator_counter)
            # + "\n\t# Contradictions = "
            # + str(catch_it)
            # + " ("
            # + str(pos2neg)
            # + ", "
            # + str(neg2pos)
            # + ")"
            + "\n\tTime Taken = "
            + str(time() - start_time)
            + " seconds"
            # + "\n\tOverlap Matrix = "
        )
        # print_2d(self.overlap_matrix)
        # print()
        # print_2d(self.theory)

        count, p, n = self.count_violations(self.positives, self.negatives)

        return self.theory, compression, self.invented_predicate_definition


if __name__ == "__main__":

    # # positives, negatives = load_animal_taxonomy()
    # # positives, negatives = load_ionosphere()
    # positives, negatives = load_chess(switch_signs=True)
    # # positives, negatives = load_tictactoe()
    # # positives, negatives = load_chess()
    # # positives, negatives = load_mushroom()
    # mistle = Mistle(positives, negatives)
    # theory = mistle.learn()

    # positives, negatives = load_test1()
    # mistle = Mistle(positives, negatives)
    # theory = mistle.learn()

    # positives, negatives = load_ionosphere()
    # positives, negatives = load_tictactoe()
    # positives, negatives = load_chess(switch_signs=True)
    # positives, negatives = load_adult()
    positives, negatives = load_ionosphere()
    mistle = Mistle(positives, negatives)
    theory = mistle.learn()
